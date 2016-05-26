#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include "structs.h"

static pthread_mutex_t *mutex[NR_ENTR];
static volatile size_t empty_spots;
static const char log[] = "parque.log";
static int log_fd;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t empty_spots_mutex = PTHREAD_MUTEX_INITIALIZER;
static clock_t start;
static int shmid = 0;

void *ctrl_thread(void *var);
void *arrum_thread(void *var);
void quit();

int main(int argc, char *argv[]){

	//	Variaveis locais
	size_t i;

	start = clock();

	//	Validar parametros
	if (argc != 3) {
		printf("Utilizacao: %s <N_LUGARES> <T_ABERTURA>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	if (atoi(argv[1]) <= 0) {
		printf("Parametro invalido: %s\n",argv[1]);
		exit(EXIT_FAILURE);
	}
	if(atoi(argv[2]) <= 0){
		printf("Parametro invalido: %s\n",argv[2]);
		exit(EXIT_FAILURE);
	}


	// Inicializar var global
	empty_spots = atoi(argv[1]);
	log_fd = open(log,O_WRONLY|O_CREAT,0666);
	if(log_fd == -1){
		perror("open()");
		exit(EXIT_FAILURE);
	}

	if(write(
			log_fd,
			"t(ticks)\t;\tnlug\t;\tid_viat\t;\tobserv\n",
			35
	) == -1){
		perror("write()");
		exit(EXIT_FAILURE);
	}

	//	Permitir partilha de
	//	mutexes em diferentes
	//	processos
	pthread_mutexattr_t mattr;
	if(pthread_mutexattr_setpshared(&mattr,PTHREAD_PROCESS_SHARED) != 0){
		perror("pthread_mutexattr_setpshared()");
		exit(EXIT_FAILURE);
	}

	shmid = shm_open("/sope",O_CREAT|O_RDWR,0600);
	if(shmid == -1){
		perror("shmget()");
		exit(EXIT_FAILURE);
	}

	if(ftruncate(shmid,4*sizeof(pthread_mutex_t)) < 0){
		perror("ftruncate()");
		quit();
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
		quit();
		exit(EXIT_FAILURE);
	}

	for(i = 0; i < NR_ENTR;i++){

		if(pthread_mutex_init((pthread_mutex_t*)shm+sizeof(pthread_mutex_t)*i,&mattr) != 0){
			perror("pthread_mutex_init()");
			exit(EXIT_FAILURE);
		}
		mutex[i] = (pthread_mutex_t*)shm+sizeof(pthread_mutex_t)*i;

	}
	printf("mutex test\n");

	printf("pthread_mutex_lock(0) = %d\npthread_mutex_unlock(0) = %d\n",pthread_mutex_lock(mutex[0]),pthread_mutex_unlock(mutex[0]));

	//	Partilhar mutexes
	/*
	key_t key = ftok("sope2",0);
	shmid = shmget(key,4*sizeof(pthread_mutex_t),IPC_EXCL |IPC_CREAT | 0666);
	if(shmid == -1){
		perror("shmget()");
		exit(EXIT_FAILURE);
	}

	mutex = (pthread_mutex_t*) shmat(shmid,NULL,0);

	for(i = 0; i < NR_ENTR;i++){
		pthread_mutex_t tmp_mutex;
		pthread_mutex_init(&tmp_mutex,&mattr);
		mutex[i] = tmp_mutex;
	}
	 */
	//	Criar FIFOs
	for(i = 0; i < NR_ENTR; i++)
		if(mkfifo(fifo[i], O_RDWR) != 0){
			perror("mkfifo()");
			quit();
			exit(EXIT_FAILURE);
		}


	//	Criar threads dos controladores
	pthread_t tid[NR_ENTR];
	for(i = 0; i < NR_ENTR; i++){
		pthread_t tid_tmp;
		if(pthread_create(&tid_tmp, NULL, ctrl_thread, &entrance[i]) != 0){
			perror("pthread_create()");
			quit();
			exit(EXIT_FAILURE);
		}
		tid[i] = tid_tmp;
	}



	//	Abrir FIFOs
	int fd[NR_ENTR];
	for(i = 0; i < NR_ENTR; i++){
		int fd_tmp;
		if( (fd_tmp = open(fifo[i], O_WRONLY, 0666)) == -1){
			perror("open()");
			quit();
			exit(EXIT_FAILURE);
		}
		fd[i] = fd_tmp;
	}



	//	Esperar tempo de funcionamento
	//	do parque
	sleep(atoi(argv[2]));



	//	Fechar parque
	vehicle stop = last_vehicle();

	printf("Done! Closing park ...\n");
	for(i = 0; i < NR_ENTR; i++){
		if(pthread_mutex_lock(mutex[i]) != 0){
			perror("pthread_mutex_lock()");
			quit();
			exit(EXIT_FAILURE);
		}
		if(write(fd[i],&stop,sizeof(stop)) == -1){
			perror("write()");
			quit();
			exit(EXIT_FAILURE);
		}
	}



	//	Fechar FIFOs
	printf("Done! Closing FIFOs...\n");
	for(i = 0; i < NR_ENTR; i++)
		if(close(fd[i]) != 0){
			perror("close()");
			quit();
			exit(EXIT_FAILURE);
		}



	//	Esperar que os controladores
	//	terminem
	printf("Done! Waiting for threads to close...\n");

	for(i = 0; i < NR_ENTR; i++){

		if(pthread_join(tid[i], NULL) != 0){
			perror("pthread_join()");
			quit();
			exit(EXIT_FAILURE);
		}

		if(pthread_mutex_unlock(mutex[i]) != 0){
			perror("pthread_mutex_unlock()");
			quit();
			exit(EXIT_FAILURE);
		}
	}



	quit();

	printf("Done!\n");
	return 0;
}

void *ctrl_thread(void *var){

	thread_param param;

	param.i = *(size_t*) var;
	_Bool closed = 0;

	printf("ctrl_thread(\"%s\")\n",fifo[param.i]);


	//	Abrir o FIFO
	int fd = open(fifo[param.i], O_RDONLY, 0666);
	if(fd == -1){
		perror("open()");
		quit();
		exit(EXIT_FAILURE);
	}	


	//	Ler veiculos do FIFO
	//	ate o parque fechar

	while(read(fd,&param.v,sizeof(vehicle)) != 0){

		if(closed){

			if(pthread_mutex_lock(&log_mutex) != 0){
				perror("pthread_mutex_lock()");
				quit();
				exit(EXIT_FAILURE);
			}

			clock_t current_tick = clock();
			char log_line[LOG_LINE_MAX_SIZE];

			sprintf(log_line,
					"%lluu\t;\t%d\t;\t%d\t;\tencerrado\n",
					(unsigned long long) (current_tick - start),
					(int)empty_spots,
					param.v.id);

			if(write(log_fd,log_line,strlen(log_line)) == -1){
				perror("write()");
				quit();
				exit(EXIT_FAILURE);
			}

			if(pthread_mutex_lock(&log_mutex) != 0){
				perror("pthread_mutex_lock()");
				quit();
				exit(EXIT_FAILURE);
			}

			continue;
		}

		if(param.v.id == -1){
			closed = 1;
			printf("Entrance %d is closed.\n",(int)param.i);
		}


		if((param.tid = pthread_create(
				&param.t,
				NULL,
				arrum_thread,
				memcpy(malloc(sizeof(param)),&param,sizeof(param))))
				!= 0){
			perror("pthread_create()");
			quit();
			exit(EXIT_FAILURE);
		}

	}


	//	Fechar FIFO
	if(close(fd) != 0){
		perror("close()");
		quit();
		exit(EXIT_FAILURE);
	}


	return NULL;

}

void *arrum_thread(void *var){

	printf("arrum_thread()\n");

	thread_param *param = (thread_param*) var;

	//	Detach thread
	if(pthread_detach(pthread_self()) != 0){
		perror("pthread_detach()");
		quit();
		exit(EXIT_FAILURE);
	}	

	printf("Entrada %d\n", (int)param->i);
	vehicle_info(&param->v);

	char fifo_priv[FIFO_NAME_BUFFER_SIZE];
	sprintf(fifo_priv,"fifo%d",param->v.id);

	int fd_w = open(fifo_priv,O_WRONLY|O_NONBLOCK, 0666);
	if(fd_w == -1){
		perror("open()");
		quit();
		exit(EXIT_FAILURE);
	}

	char log_line[LOG_LINE_MAX_SIZE];

	//	Caso hajam lugares livres
	if(empty_spots > (size_t)0){

		if(pthread_mutex_lock(&log_mutex) != 0){
			perror("pthread_mutex_lock()");
			quit();
			exit(EXIT_FAILURE);
		}

		clock_t current_tick = clock();

		sprintf(log_line,
				"%llu\t;\t%d\t;\t%d\t;\testacionamento\n",
				(unsigned long long) (current_tick - start),
				(int) empty_spots,
				param->v.id
		);
		if(write(log_fd,log_line,strlen(log_line)) == -1){
			perror("write()");
			quit();
			exit(EXIT_FAILURE);
		}

		if(write(fd_w,"entrada",7) == -1){
			perror("write()");
			exit(EXIT_FAILURE);
		}

		//	Decrementar lugares livres
		if(pthread_mutex_lock(&empty_spots_mutex) != 0){
			perror("pthread_mutex_unlock()");
			quit();
			exit(EXIT_FAILURE);
		}

		empty_spots--;

		if(pthread_mutex_unlock(&empty_spots_mutex) != 0){
			perror("pthread_mutex_unlock()");
			quit();
			exit(EXIT_FAILURE);
		}

		clock_t end = clock() + param->v.duration;


		//	Esperar saida do carro
		while(clock() < end);


		//	Incrementar lugares livres
		if(pthread_mutex_lock(&empty_spots_mutex) != 0){
			perror("pthread_mutex_unlock()");
			quit();
			exit(EXIT_FAILURE);
		}

		empty_spots++;

		if(pthread_mutex_unlock(&empty_spots_mutex) != 0){
			perror("pthread_mutex_unlock()");
			quit();
			exit(EXIT_FAILURE);
		}

		if(write(fd_w,"saida",5) == -1){
			perror("write()");
			quit();
			exit(EXIT_FAILURE);
		}

		if(pthread_mutex_lock(&log_mutex) != 0){
			perror("pthread_mutex_lock()");
			quit();
			exit(EXIT_FAILURE);
		}

		current_tick = clock();
		memset(log_line,0,LOG_LINE_MAX_SIZE);
		sprintf(log_line,
				"%llu\t;\t%d\t;\t%d\t;\tsaida\n",
				(unsigned long long) (current_tick - start),
				(int) empty_spots,
				param->v.id
		);
		if(write(log_fd,log_line,strlen(log_line)) == -1){
			perror("write()");
			quit();
			exit(EXIT_FAILURE);
		}
	}
	else{

		if(pthread_mutex_lock(&log_mutex) != 0){
			perror("pthread_mutex_lock()");
			quit();
			exit(EXIT_FAILURE);
		}

		clock_t current_tick = clock();
		sprintf(log_line,
				"%llu\t;\t%d\t;\t%d\t;\tcheio\n",
				(unsigned long long) (current_tick - start),
				(int) empty_spots,
				param->v.id
		);
		if(write(log_fd,log_line,strlen(log_line)) == -1){
			quit();
			perror("write()");
			exit(EXIT_FAILURE);
		}

		if(write(fd_w,"cheio!",6) == -1){
			quit();
			perror("write()");
			exit(EXIT_FAILURE);
		}
	}

	if(close(fd_w) != 0){
		perror("close()");
		quit();
		exit(EXIT_FAILURE);
	}

	free(var);

	return NULL;

}

void quit(){

	int i;
	for(i = 0; i < NR_ENTR; i++)
		unlink(fifo[i]);

	close(log_fd);

	//	Destruir mutexes
	for(i = 0; i < NR_ENTR; i++)
		pthread_mutex_destroy(mutex[i]);
	shmdt(mutex);
	shmctl(shmid, IPC_RMID, NULL);
}
