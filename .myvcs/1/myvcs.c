#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <dirent.h>
#include <sys/types.h>
#include <errno.h>

int current = 0;
int highest = 0;

void initialize()
{
	FILE* info_file;
	char current_buf[5];
	char highest_buf[5];

	if ((info_file = fopen(".myvcs/.info", "r")) == NULL) {
		fprintf(stderr, "[ERROR] fopen(%s): %s\n", ".myvcs/.info", strerror(errno));
		exit(-1);
	}

	fgets(current_buf, 5, info_file);
	fgets(highest_buf, 5, info_file);

	current = atoi(current_buf);
	highest = atoi(highest_buf);

	fclose(info_file);
}

void save_info()
{
	FILE* info_file;

	if ((info_file = fopen(".myvcs/.info", "w")) == NULL) {
		fprintf(stderr, "[ERROR] fopen(%s): %s\n", ".myvcs/.info", strerror(errno));
		exit(-1);
	}

	fprintf(info_file, "%d\n", current);
	fprintf(info_file, "%d\n", highest);

	fclose(info_file);
}

void copy_all(char* dir_from, char* dir_to)
{
	FILE* dest;
	FILE* src;

	char filename_buf[1024];
	DIR* dp;
	struct dirent* ep;
	int c;

	if((dp = opendir(dir_from)) == NULL) {
		fprintf(stderr, "[ERROR] open_dir(): %s\n", strerror(errno));
		return;
	}

	while ((ep = readdir(dp)) != NULL) {
		if ((ep->d_name[0] == '.') || (ep->d_name[strlen(ep->d_name)-1] == '/'))
			continue;

		snprintf(filename_buf, 1024, "%s/%s", dir_from, ep->d_name);
		if ((src = fopen(filename_buf, "r")) == NULL) {
			fprintf(stderr, "[ERROR] fopen(%s): %s\n", filename_buf, strerror(errno));
			return;
		}
		
		snprintf(filename_buf, 1024, "%s/%s", dir_to, ep->d_name);
		if ((dest = fopen(filename_buf, "w")) == NULL) {
			fprintf(stderr, "[ERROR] fopen(%s): %s\n", filename_buf, strerror(errno));
			return;
		}
		
		while ((c = fgetc(src)) != EOF)
			fputc(c, dest);
		
		fclose(src);
		fclose(dest);
	}
}

void clear_directory(char* dir)
{
	char filename[1024];
	DIR* dp;
	struct dirent* ep;

	if((dp = opendir(dir)) == NULL) {
		fprintf(stderr, "[ERROR] open_dir(%s): %s\n", dir, strerror(errno));
		return;
	}

	while ((ep = readdir(dp)) != NULL) {
		if ((ep->d_name[0] == '.') || (ep->d_name[strlen(ep->d_name)-1] == '/'))
			continue;
		snprintf(filename, 1024, "%s/%s", dir, ep->d_name);
		unlink(ep->d_name);
	}
}

void snap(char** argv)
{
	FILE* metafile;

	time_t rawtime;
	struct tm * timeinfo;

	int new_id = highest + 1;
	char new_dir[50];
	char filename_buf[100];
	char cwd[1024];

	snprintf(new_dir, 50, ".myvcs/%d", new_id);

	if(mkdir(new_dir, 0777) == -1) {
		fprintf(stderr, "[ERROR] mkdir(): %s\n", strerror(errno));
		return;
	}

	snprintf(filename_buf, 100, "%s/.metafile", new_dir);

	if ((metafile = fopen(filename_buf, "w")) == NULL) {
		fprintf(stderr, "[ERROR] fopen(): %s\n", strerror(errno));
		return;
	}

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	/* save time snap was made */
	fprintf(metafile, "%s", asctime(timeinfo));
	/* save parent id */
	fprintf(metafile, "%d\n", current);

	current = new_id;
	highest = new_id;

	if (argv[2] != NULL) 
		fprintf(metafile, "%s\n", argv[2]);

	copy_all("./", new_dir);
}

void checkout(int id)
{
	char checkout_dir[50];

	snprintf(checkout_dir, 50, ".myvcs/%d", id);

	clear_directory("./");
	copy_all(checkout_dir, "./");

	current = id;
}

void print_log(int id)
{
	FILE* metadata;
	char filename[40];
	char date[40];
	char parent[10];
	char message[1024];
	int c;
	int i;
	int parent_id;

	snprintf(filename, 40, ".myvcs/%d/.metafile", id);

	if ((metadata = fopen(filename, "r")) == NULL) {
		fprintf(stderr, "[ERROR] fopen(%s): %s\n", filename, strerror(errno));
		return;
	}

	/* get date */
	fgets(date, 40, metadata);
        /* parent */
	fgets(parent, 10, metadata);

	/* message */
	i = 0;
	while ((c = fgetc(metadata)) != EOF)
		message[i++] = c;
	message[i] = '\0';

	printf("date: %smessage: %s\n", date, message);
	
	parent_id = atoi(parent);
	if (parent_id != 0)
		print_log(parent_id);

	fclose(metadata);
}

int main(int argc, char** argv)
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <command>\n", argv[0]);
		exit(0);
	}

	initialize();

	if (strcmp(argv[1], "snap") == 0)
		snap(argv);
	
	else if (strcmp(argv[1], "current") == 0)
		printf("current: %d\n", current);
	
	else if (strcmp(argv[1], "checkout") == 0) {
		if (argc < 3) {
			fprintf(stderr, "checkout requires id argument\n");
			exit(0);
		}
		checkout(atoi(argv[2]));
	}

	else if (strcmp(argv[1], "log") == 0)
		print_log(current);

	else
		printf("Command unrecognized.\n");

	save_info();

	return 0;
}


