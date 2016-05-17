#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "viatura.h"

time_t current_time;
time_t closing_time;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *ctrl_thread(void *var);
void *arrum_thread(void *var);

int main(int argc, char *argv[]){

	if (argc != 3) {
		printf("Utilizacao: %s <N_LUGARES> <T_ABERTURA>\n", argv[0]);
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

	current_time = time(NULL);

	closing_time = current_time + (time_t) atoi(argv[2]);	

	pthread_t N_ctrl, S_ctrl, E_ctrl, O_ctrl;

	pthread_create(&N_ctrl, NULL, ctrl_thread, "/tmp/fifoN");
	pthread_create(&S_ctrl, NULL, ctrl_thread, "/tmp/fifoS");
	pthread_create(&E_ctrl, NULL, ctrl_thread, "/tmp/fifoE");
	pthread_create(&O_ctrl, NULL, ctrl_thread, "/tmp/fifoO");

	pthread_join(N_ctrl, NULL);
	pthread_join(S_ctrl, NULL);
	pthread_join(E_ctrl, NULL);
	pthread_join(O_ctrl, NULL);

	return 0;
}

void *ctrl_thread(void *var){

	int fifo = mkfifo((char*)var, 0644);

	int fd_read = open((char*)var, O_RDONLY);
	int fd_write = open((char*)var, O_WRONLY);
	
	vehicle* vehicle;
	pthread_t arrum;
	
	while(vehicle = new_vehicle(fd) != NULL){

		if(tmp->id == -1){
			close(fd_write);
			continue;
		}

		pthread_create(&arrum, NULL, arrum_thread, vehicle);
	}
	
	close(fd_read);	
	unlink((char*) var);
	
	return NULL;

}
	

}

void *arrum_thread(void *var){

	pthread_detach(pthread_self());
	vehicle* vehicle = (vehicle*) var;
	free(vehicle);

}
