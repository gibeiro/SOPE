#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct vehicle;
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

	char fifoN[] = "fifoN";
	char fifoS[] = "fifoS";
	char fifoE[] = "fifoE";
	char fifoO[] = "fifoO";

	printf("Making FIFOs\n");
	mode_t mode = 0644;
	mkfifo(fifoN, mode);
	mkfifo(fifoS, mode);
	mkfifo(fifoE, mode);
	mkfifo(fifoO, mode);

	printf("Creating controler threads\n");
	pthread_t tid_N, tid_S, tid_E, tid_O;
	pthread_create(&tid_N, NULL, ctrl_thread, fifoN);
	pthread_create(&tid_S, NULL, ctrl_thread, fifoS);
	pthread_create(&tid_E, NULL, ctrl_thread, fifoE);
	pthread_create(&tid_O, NULL, ctrl_thread, fifoO);

	printf("Opening FIFOs WRONLY\n");
	int fd_N = open(fifoN, O_WRONLY);
	int fd_E = open(fifoE, O_WRONLY);
	int fd_S = open(fifoS, O_WRONLY);
	int fd_O = open(fifoO, O_WRONLY);

	printf("Waiting %s seconds ...\n",argv[2]);
	sleep(atoi(argv[2]));

	vehicle close_park = last_vehicle();
	int sizeof_vehicle = sizeof(vehicle);
	//pthread_mutex_lock(&mutex);

	printf("Done! Closing park ...\n");
	write(fd_N,&close_park,sizeof_vehicle);
	write(fd_S,&close_park,sizeof_vehicle);
	write(fd_E,&close_park,sizeof_vehicle);
	write(fd_O,&close_park,sizeof_vehicle);

	printf("Done! Closing FIFOs...\n");
	close(fd_N);
	close(fd_S);
	close(fd_E);
	close(fd_O);

	printf("Done! Waiting for threads to close...\n");
	pthread_join(tid_N, NULL);
	pthread_join(tid_S, NULL);
	pthread_join(tid_E, NULL);
	pthread_join(tid_O, NULL);

	//pthread_mutex_unlock(&mutex);

	printf("Done! Unlinking FIFOs...\n");
	unlink(fifoN);
	unlink(fifoS);
	unlink(fifoE);
	unlink(fifoO);

	printf("Done!\n");
	return 0;
}

void *ctrl_thread(void *var){

	char* fifo_name = (char*) var;
	printf("ctrl_thread(\"%s\")\n",fifo_name);

	int fd_read = open(fifo_name, O_RDONLY);

	vehicle vehicle;
	pthread_t arrum;

	while(vehicle = new_vehicle(fd) != NULL){

		if(vehicle.id == -1)
			break;

		pthread_create(&arrum, NULL, arrum_thread, vehicle);
	}

	printf("Closing ctrl_thread(\"%s\")\n",fifo_name);

	return NULL;

}

void *arrum_thread(void *var){
/*
	pthread_detach(pthread_self());
	vehicle* vehicle = (vehicle*) var;
	free(vehicle);
	*/
	printf("arrum_thread()\n");

}

typedef struct{

	int parking_durantion;
	int id;
	int fifo_id;
	char entrance;

} vehicle;

vehicle new_vehicle(int fd){

	vehicle v;

	read(fd,&v,sizeof(vehicle);

	if(v.id == -1)
		return NULL;
	else
		return v;
}

vehicle last_vehicle(){

	vehicle v;

	v.id = -1;

	return v;
}
