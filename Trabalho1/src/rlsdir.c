#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

#define PATH_STRING_SIZE 128

int main(int argc, char* argv[]){

	DIR *dir;
	struct dirent *dentry;
	struct stat stat_entry;
	pid_t pid;
	int status;

	char bin_path[PATH_STRING_SIZE];
	memset(bin_path,0,sizeof(bin_path));
	getcwd(bin_path,sizeof(bin_path));
	strcat(bin_path,"/bin/lsdir");

	char path[PATH_STRING_SIZE];
	memset(path,0,sizeof(path));
	strcpy(path,argv[1]);
	if( path[ strlen(path) - 1 ] != '/')
		strcat(path,"/");

	if (argc != 3) {
		printf("Usage: %s <dir_path> <file_path>\n", argv[0]);
		return 1;
	}

	if((fopen(argv[2],"a")) == NULL){
		perror(argv[2]);
		return 3;
	}

	loop:

	if ((dir = opendir(path)) == NULL) {
		perror(path);
		return 2;
	}

	chdir(path);

	pid = fork();

	if(pid < 0)
		exit(1);


	if(pid == 0)
		execlp("lsdir", "lsdir", path, argv[2], NULL);

	else
		wait(&status);

	while( (dentry = readdir(dir)) != NULL ){

		if(dentry->d_name[0] == '.')
			continue;

		stat(dentry->d_name, &stat_entry);

		if (S_ISDIR(stat_entry.st_mode)){

			pid = fork();

			if(pid < 0)
				exit(1);


			if(pid == 0){
				if( path[ strlen(path) - 1 ] != '/')
					strcat(path,"/");
				strcat(path,dentry->d_name);
				goto loop;
			}

			if(pid > 0)
				wait(&status);

		}

	}

	closedir(dir);

	return 0;

}