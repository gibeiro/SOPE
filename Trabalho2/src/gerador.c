#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include "structs.h"

static pthread_mutex_t * volatile mutex;
static size_t id = 0;

void* vehicle_thread(void* var);

int main(int argc, char *argv[]){

	if (argc != 3) {
		printf("Utilizacao: %s <T_GERACAO> <U_RELOGIO>\n", argv[0]);
		return 1;
	}

	if (atoi(argv[1]) <= 0) {
		printf("Parametro invalido: %s\n",argv[1]);
		return 2;
	}

	if(atoi(argv[2]) <= 0){
		printf("Parametro invalido: %s\n",argv[2]);
		return 3;
	}

	key_t key = ftok("parque",0);
	int shmid = shmget(key,0,0);
	if(shmid == -1){
		perror("shmget()");
		exit(EXIT_FAILURE);
	}

	mutex = (pthread_mutex_t*) shmat(shmid,NULL,0);
	if(mutex == (pthread_mutex_t*)-1){
		perror("shmat()");
		exit(EXIT_FAILURE);
	}

	time_t end = time(NULL) + (time_t) atoi(argv[1]);
	clock_t ticks = (clock_t) atoi(argv[2]);

	srand(time(NULL));

	thread_param param;

	while(time(NULL) < end){

		param.i = rand() % 4;
		param.v.duration = ticks * (clock_t) ((rand() % 2) + 1);
		param.v.id = id++;

		if(pthread_create(
				&param.tid,
				NULL,
				vehicle_thread,
				memcpy(malloc(sizeof(param)),&param,sizeof(param)))
				!= 0){
			perror("pthread_create()");
			exit(EXIT_FAILURE);
		}

	}

	return 0;
}

void* vehicle_thread(void* var){

	thread_param *param = (thread_param*)var;

	pthread_detach(param->tid);

	int fd = open(fifo[param->i],O_WRONLY|O_NONBLOCK, 0666);

	if(fd != -1){

		if(pthread_mutex_lock(&mutex[param->i]) != 0){
			perror("pthread_mutex_lock()");
			exit(EXIT_FAILURE);
		}
		if( write(fd,&param->v,sizeof(param->v)) == -1){
			perror("write()");
			exit(EXIT_FAILURE);
		}

		if(pthread_mutex_lock(&mutex[param->i]) != 0){
			perror("pthread_mutex_lock()");
			exit(EXIT_FAILURE);
		}

		if(close(fd) != 0){
			perror("close()");
			exit(EXIT_FAILURE);
		}
	}

	if(fd == -1){
		//parque fechado
	}

	free(var);

	return NULL;

}
