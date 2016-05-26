#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include "structs.h"

static pthread_mutex_t *mutex;
static size_t id = 0;
static const char log[] = "gerador.log";
static int log_fd;
static pthread_mutex_t log_mutex;
static clock_t start;
//static struct tms st_cpu;

void* vehicle_thread(void* var);

int main(int argc, char *argv[]){

	start = clock();

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

	//start = times(&st_cpu);

	log_fd = open(log,O_WRONLY|O_CREAT,0666);
	if(log_fd == -1){
		perror("open()");
		exit(EXIT_FAILURE);
	}

	if(write(
			log_fd,
			"t(ticks)\t;\tid_viat\t;\tdestin\t;\tt_estacion\t;\tt_vida\t;\tobserv\n",
			59
	) == -1){
		perror("write()");
		exit(EXIT_FAILURE);
	}


	key_t key = ftok("sope2",0);
	int shmid = shmget(key,4*sizeof(pthread_mutex_t),0666);
	if(shmid == -1){
		perror("shmget()");
		exit(EXIT_FAILURE);
	}

	mutex = (pthread_mutex_t*) shmat(shmid,NULL,0);

	time_t end = time(NULL) + (time_t) atoi(argv[1]);
	clock_t ticks = (clock_t) atoi(argv[2]);

	srand(time(NULL));

	thread_param *param;

	while(time(NULL) < end){

		param = malloc(sizeof(param));
		param->i = rand() % 4;
		param->v.duration = ticks * (clock_t) ((rand() % 2) + 1);
		param->v.id = id++;

		if((param->tid = pthread_create(
				&param->t,
				NULL,
				vehicle_thread,
				param
		))
				!= 0){
			perror("pthread_create()");
			exit(EXIT_FAILURE);
		}

	}

	if(close(log_fd) != 0){
		perror("close()");
		exit(EXIT_FAILURE);
	}

	return 0;
}

void* vehicle_thread(void* var){

	clock_t v_start = clock();

	thread_param *param = (thread_param*)var;

	pthread_detach(pthread_self());

	printf("vehicle_thread()\n");
	vehicle_info(&param->v);

	char fifo_priv[FIFO_NAME_BUFFER_SIZE];
	sprintf(fifo_priv,"fifo%d",param->v.id);

	if(mkfifo(fifo_priv, O_RDWR) != 0){
		perror("mkfifo()");
		exit(EXIT_FAILURE);
	}

	if(pthread_mutex_lock(&mutex[param->i]) != 0){
		perror("pthread_mutex_lock()");
		exit(EXIT_FAILURE);
	}

	int fd_w = open(fifo[param->i],O_WRONLY|O_NONBLOCK, 0666);

	if(fd_w != -1){

		if( write(fd_w,&param->v,sizeof(param->v)) == -1){
			perror("write()");
			exit(EXIT_FAILURE);
		}

		if(close(fd_w) != 0){
			perror("close()");
			exit(EXIT_FAILURE);
		}
	}

	if(fd_w == -1){
		//parque fechado
		printf("parque fechado\n");
	}

	if(pthread_mutex_unlock(&mutex[param->i]) != 0){
		perror("pthread_mutex_unlock()");
		exit(EXIT_FAILURE);
	}

	int fd_r = open(fifo_priv,O_RDONLY,0666);
	if(fd_r == -1){
		perror("open()");
		exit(EXIT_FAILURE);
	}

	char message[MAX_MESSAGE_SIZE];
	if(read(fd_r,message,sizeof(message)) == -1){
		perror("read()");
		exit(EXIT_FAILURE);
	}

	char log_line[LOG_LINE_MAX_SIZE];
	char destin[1];
	switch(param->i){
	case N:
		*destin = 'N';
		break;
	case S:
		*destin = 'S';
		break;
	case E:
		*destin = 'E';
		break;
	case O:
		*destin = 'O';
		break;

	}

	if(strcmp(message, "cheio!") == 0){

		clock_t current_tick = clock();
		sprintf(
				log_line,
				"%llu\t;\t%d\t;\t%s\t;\t%d\t;\t?\t;\t%s\n",
				(unsigned long long) (current_tick - start),
				param->v.id,
				destin,
				(int)param->v.duration,
				message

		);

		if(pthread_mutex_lock(&log_mutex) != 0){
			perror("pthread_mutex_lock()");
			exit(EXIT_FAILURE);
		}

		if(write(log_fd,log_line,strlen(log_line)) == -1){
			perror("write()");
			exit(EXIT_FAILURE);
		}

		if(pthread_mutex_unlock(&log_mutex) != 0){
			perror("pthread_mutex_lock()");
			exit(EXIT_FAILURE);
		}

	}
	else if(strcmp(message, "entrada") == 0){

		clock_t current_tick = clock();
		sprintf(
				log_line,
				"%llu\t;\t%d\t;\t%s\t;\t%d\t;\t%s\t;\t%s\n",
				(unsigned long long) (current_tick - start),
				param->v.id,
				destin,
				(int)param->v.duration,
				"?",
				message

		);

		if(pthread_mutex_lock(&log_mutex) != 0){
			perror("pthread_mutex_lock()");
			exit(EXIT_FAILURE);
		}

		if(write(log_fd,log_line,strlen(log_line)) == -1){
			perror("write()");
			exit(EXIT_FAILURE);
		}

		if(pthread_mutex_unlock(&log_mutex) != 0){
			perror("pthread_mutex_lock()");
			exit(EXIT_FAILURE);
		}

		if(read(fd_r,message,(size_t)MAX_MESSAGE_SIZE) == -1){
			perror("read()");
			exit(EXIT_FAILURE);
		}

		memset(log_line,0,LOG_LINE_MAX_SIZE);

		current_tick = clock();
		sprintf(
				log_line,
				"%llu\t;\t%d\t;\t%s\t;\t%d\t;\t%llu\t;\t%s\n",
				(unsigned long long) (current_tick - start),
				param->v.id,
				destin,
				(int)param->v.duration,
				(unsigned long long) (current_tick - v_start),
				message

		);

		if(pthread_mutex_lock(&log_mutex) != 0){
			perror("pthread_mutex_lock()");
			exit(EXIT_FAILURE);
		}

		if(write(log_fd,log_line,strlen(log_line)) == -1){
			perror("write()");
			exit(EXIT_FAILURE);
		}

		if(pthread_mutex_unlock(&log_mutex) != 0){
			perror("pthread_mutex_lock()");
			exit(EXIT_FAILURE);
		}

	}



	if(close(fd_r) != 0){
		perror("close()");
		exit(EXIT_FAILURE);
	}
	unlink(fifo_priv);

	free(var);

	return NULL;

}
