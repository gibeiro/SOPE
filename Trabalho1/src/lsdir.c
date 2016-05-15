#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

int main(int argc, char *argv[])
{

	DIR *dir;
	FILE *txt;
	struct dirent *dentry;
	struct stat stat_entry;

	if (argc != 3) {
		printf("Usage: %s <dir_path> <file_path>\n", argv[0]);
		return 1;
	}

	if ((dir = opendir(argv[1])) == NULL) {
		perror(argv[1]);
		return 2;
	}

	if((txt = fopen(argv[2],"a")) == NULL){
		perror(argv[2]);
		return 3;
	}

	chdir(argv[1]);

	while( (dentry = readdir(dir)) != NULL){

		if(dentry->d_name[0] == '.')
			continue;

		stat(dentry->d_name, &stat_entry);

		if (S_ISREG(stat_entry.st_mode))
			fprintf(
					txt,
					"%s\t%d\t%d\t%d\t%s\n",
					dentry->d_name,
					stat_entry.st_mode,
					(int)stat_entry.st_size,
					(int)stat_entry.st_mtime,
					argv[1]
						 );

	}

	fclose(txt);
	closedir(dir);

	return 0;
}