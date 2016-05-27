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

static pthread_mutex_t *mutex[NR_ENTR];
static size_t id = 0;
static const char log[] = "gerador.log";
static int log_fd;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
static clock_t start;
static int shmid = 0;

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


	shmid = shm_open("/sope",O_RDWR,0666);
	if(shmid == -1){
		perror("shmget()");
		exit(EXIT_FAILURE);
	}

	if(ftruncate(shmid,4*sizeof(pthread_mutex_t)) < 0){
		perror("ftruncate()");
		exit(EXIT_FAILURE);
	}

	char *shm =	(char*) mmap(
			0,
			4*sizeof(pthread_mutex_t),
			PROT_READ|PROT_WRITE,
			MAP_SHARED,
			shmid,
			0);

	if(shm == (char*) MAP_FAILED){
		perror("mmap()");
		exit(EXIT_FAILURE);
	}
	int i = 0;
	for(i = 0; i < NR_ENTR;i++)
		mutex[i] = (pthread_mutex_t*)shm+sizeof(pthread_mutex_t)*i;

	time_t end = time(NULL) + (time_t) atoi(argv[1]);
	clock_t ticks = (clock_t) atoi(argv[2]);

	srand(time(NULL));

	thread_param param;

	const int prob[] = {0,0,0,0,0,1,1,1,2,2};
	while(time(NULL) < end){

		param.i = rand() % 4;
		param.v.duration = ticks * (clock_t) ((rand() % 10) + 1);
		param.v.id = id++;
		thread_param *tmp = malloc(sizeof(pthread_mutex_t));
		memcpy(tmp,&param,sizeof(pthread_mutex_t));

		if((tmp->tid = pthread_create(
				&tmp->t,
				NULL,
				vehicle_thread,
				tmp
		))
				!= 0){
			//perror("pthread_create()");
			free(tmp);
			continue;
		}

		clock_t end = clock() +  prob[rand()%10]*ticks;

		while(clock() < end);

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

	if(pthread_detach(pthread_self()) != 0){
		perror("pthread_detach()");
		exit(EXIT_FAILURE);
	}

	printf("\nvehicle_thread()\n");
	printf("Entrance %d\n", (int)param->i);
		vehicle_info(&param->v);


	char fifo_priv[FIFO_NAME_BUFFER_SIZE];
	sprintf(fifo_priv,"fifo%d",param->v.id);
	if(mkfifo(fifo_priv, O_RDWR) != 0){
		perror("mkfifo()");
		exit(EXIT_FAILURE);
	}

	char log_line[LOG_LINE_MAX_SIZE];
	char destin[1];
	clock_t current_tick;
	switch(param->i){
	case N:
		destin[0] = 'N';
		break;
	case S:
		destin[0] = 'S';
		break;
	case E:
		destin[0] = 'E';
		break;
	case O:
		destin[0] = 'O';
		break;

	}

	printf("Opening entrance FIFO ...");
	int fd_w = open(fifo[param->i],O_WRONLY|O_NONBLOCK, 0666);

	if(fd_w != -1){
		printf(" Done!\n");
		if(pthread_mutex_lock(mutex[param->i]) != 0){
			//perror("pthread_mutex_lock()");
			//exit(EXIT_FAILURE);
		}

		printf("Writting ...");
		if( write(fd_w,&param->v,sizeof(param->v)) == -1){
			perror("write()");
			exit(EXIT_FAILURE);
		}

		if(pthread_mutex_unlock(mutex[param->i]) != 0){
			//perror("pthread_mutex_unlock()");
			//exit(EXIT_FAILURE);
		}

		if(close(fd_w) != 0){
			perror("close()");
			exit(EXIT_FAILURE);
		}
		printf(" Done!\n");
	}

	if(fd_w == -1){
		printf(" Closed!\n");
		current_tick = clock();
		memset(log_line,0,sizeof(log_line));
		sprintf(
				log_line,
				"%d\t;\t%d\t;\t%s\t;\t%d\t;\t?\t;\tencerrado\n",
				(int) (current_tick - start),
				param->v.id,
				destin,
				(int)param->v.duration

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

		printf(vehicle_thread end\n\n");

		unlink(fifo_priv);

		free(var);
		return NULL;
	}


	printf("Opening private FIFO ...");
	int fd_r = open(fifo_priv,O_RDONLY, 0666);
	if(fd_r == -1){
		close(fd_r);
		printf(" Closed!\n");
		unlink(fifo_priv);
		free(var);
		return NULL;
	}
	printf(" Done!\n");

	printf("Reading private FIFO ...");
	char message[MAX_MESSAGE_SIZE];
	if(read(fd_r,message,sizeof(message)) == -1){
		perror("read()");
		exit(EXIT_FAILURE);
	}
	printf(" Done!\n");


	if(strcmp(message, "cheio!") == 0){

		current_tick = clock();
		memset(log_line,0,sizeof(log_line));
		sprintf(
				log_line,
				"%d\t;\t%d\t;\t%s\t;\t%d\t;\t?\t;\t%s\n",
				(int) (current_tick - start),
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

		current_tick = clock();
		memset(log_line,0,sizeof(log_line));
		sprintf(
				log_line,
				"%d\t;\t%d\t;\t%s\t;\t%d\t;\t?\t;\t%s\n",
				(int) (current_tick - start),
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

		if(read(fd_r,message,(size_t)MAX_MESSAGE_SIZE) == -1){
			perror("read()");
			exit(EXIT_FAILURE);
		}

		memset(log_line,0,LOG_LINE_MAX_SIZE);

		current_tick = clock();
		memset(log_line,0,sizeof(log_line));
		sprintf(
				log_line,
				"%d\t;\t%d\t;\t%s\t;\t%d\t;\t%d\t;\t%s\n",
				(int) (current_tick - start),
				param->v.id,
				destin,
				(int)param->v.duration,
				(int) (current_tick - v_start),
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
	else if(strcmp(message, "encerrado") == 0){

		current_tick = clock();
		memset(log_line,0,sizeof(log_line));
		sprintf(
				log_line,
				"%d\t;\t%d\t;\t%s\t;\t%d\t;\t?\t;\t%s\n",
				(int) (current_tick - start),
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


	if(close(fd_r) != 0){
		perror("close()");
		exit(EXIT_FAILURE);
	}

	printf("vehicle_thread() end\n\n");

	unlink(fifo_priv);

	free(var);

	return NULL;

}
