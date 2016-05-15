#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#define LINE_SIZE 128


int compareFiles(const char* dir1, const char* dir2){

	FILE *fp1 = fopen(dir1,"r");
	FILE *fp2 = fopen(dir2,"r");

	if(fp1 == NULL || fp2 == NULL)
		return -1;

	int ch1 = getc(fp1);
	int ch2 = getc(fp2);

	while((ch1 != EOF) && (ch2 != EOF)){

		if(ch1 != ch2)
			return 1;

		ch1 = getc(fp1);
		ch2 = getc(fp2);
	}

	return 0;
}

int main(int argc, char* argv[]){

	pid_t pid;
	int status;

	if (argc != 2) {
		printf("Usage: %s <dir_path>\n", argv[0]);
		return 1;
	}

	if ((opendir(argv[1])) == NULL) {
		perror(argv[1]);
		return 2;
	}

	pid = fork();

	if(pid < 0)
		exit(1);

	if(pid == 0)
		execlp("rlsdir","rlsdir",argv[1],"/tmp/filesaux.txt",NULL);

	else
		wait(&status);

	int flaux, fldes;

	flaux = open("/tmp/filesaux.txt", O_RDONLY);
	fldes = open("/tmp/files.txt", O_RDWR|O_CREAT|O_TRUNC,0600);

	pid = fork();

	if(pid < 0)
		exit(1);
	if(pid == 0){
		dup2(flaux, STDIN_FILENO);
		dup2(fldes, STDOUT_FILENO);
		execlp("sort","sort", NULL);
	}
	else
		wait(&status);

	FILE *hlinks = fopen( strcat(argv[1],"/hlinks.txt")  ,"a");
	FILE *files = fopen("/tmp/files.txt","r");
	char line1[LINE_SIZE] = "";
	char line2[LINE_SIZE] = "";
	char line1_tmp[LINE_SIZE] = "";
	char line2_tmp[LINE_SIZE] = "";
	char *name1 = NULL;
	char *name2 = NULL;
	char *size1 = NULL;
	char *size2 = NULL;
	char *mode1 = NULL;
	char *mode2 = NULL;
	//char *time1 = NULL;
	//char *time2 = NULL;
	char *dir1 = NULL;
	char *dir2 = NULL;

	if(fgets(line1,LINE_SIZE,files) == NULL)
		exit(1);

	while(fgets(line2,LINE_SIZE,files) != NULL){

		strcpy(line1_tmp, line1);
		name1 = strtok(line1_tmp, "\t");
		mode1 = strtok(NULL, "\t");
		size1 = strtok(NULL, "\t");
		strtok(NULL, "\t");
		dir1 = strtok(NULL, "\t");
		dir1 = strtok(dir1, "\n");

		strcpy(line2_tmp, line2);
		name2 = strtok(line2_tmp, "\t");
		mode2 = strtok(NULL, "\t");
		size2 = strtok(NULL, "\t");
		strtok(NULL, "\t");
		dir2 = strtok(NULL, "\t");
		dir2 = strtok(dir2, "\n");


		if(
				strcmp(name1,name2) == 0 &&
				strcmp(mode1,mode2) == 0 &&
				strcmp(size1,size2) == 0 &&
				compareFiles(
						strcat(dir1,strcat("/",name1)),
						strcat(dir2,strcat("/",name2))) == 0
		){

			remove( strcat(dir2,strcat("/",name2)) );			
			link( strcat(dir1,strcat("/",name1)), strcat(dir2,strcat("/",name2)) );
			fprintf(hlinks, "%s linked to file %s",strcat(dir2,strcat("/",name2)),strcat(dir1,strcat("/",name1)));
			
		}



		else
			strcpy(line1,line2);


	}

	fclose(hlinks);
	fclose(files);

	return 0;

}
