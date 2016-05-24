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
#include <string.h>

#define	N	0
#define	S	1
#define	E	2
#define	O	3

typedef struct {

	clock_t parking_duration;
	int id;
	int fifo_id;

} vehicle;

typedef struct {

	vehicle v;
	size_t i;

} arrum_param;

static const char *fifo[] = {
		"fifoN",
		"fifoS",
		"fifoE",
		"fifoO"
};

static size_t entrance[] = {N,S,E,O};

static pthread_mutex_t * volatile mutex;
static volatile size_t empty_spots;

vehicle new_vehicle(int fd);
vehicle last_vehicle();
void vehicle_info(vehicle *v);

void *ctrl_thread(void *var);
void *arrum_thread(void *var);

int main(int argc, char *argv[]){

	//	Variaveis locais
	size_t i;



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



	//	Permitir partilha de
	//	mutexes em diferentes
	//	processos
	pthread_mutexattr_t mattr;
	pthread_mutexattr_setpshared(&mattr,PTHREAD_PROCESS_SHARED);



	//	Partilhar mutexes
	pthread_mutex_t m[4];
	size_t shm_size = 4*sizeof(m);

	key_t key = ftok("parque",0);
	int shmid = shmget(key, shm_size,IPC_CREAT | 0666);
	if(shmid == -1){
		perror("shmget()");
		exit(EXIT_FAILURE);
	}

	mutex = (pthread_mutex_t*) shmat(shmid,NULL,0);
	if(mutex == (pthread_mutex_t*)-1){
		perror("shmat()");
		exit(EXIT_FAILURE);
	}

	memcpy(mutex,m,shm_size);



	// Inicializar mutexes
	for(i = 0; i < 4; i++){
		pthread_mutex_t m_tmp;
		mutex[i] = m_tmp;
		if(pthread_mutex_init(&mutex[i],&mattr) != 0){
			perror("pthread_mutex_init()");
			exit(EXIT_FAILURE);
		}
	}



	//	Criar FIFOs
	mode_t mode = 0644;
	for(i = 0; i < 4; i++)
		if(mkfifo(fifo[i], mode) != 0){
			perror("pthread_mutex_init()");
			exit(EXIT_FAILURE);
		}


	//	Criar threads dos controladores
	pthread_t tid[4];
	for(i = 0; i < 4; i++){
		pthread_t tid_tmp;
		if(pthread_create(&tid_tmp, NULL, ctrl_thread, &entrance[i]) != 0){
			perror("pthread_create()");
			exit(EXIT_FAILURE);
		}
		tid[i] = tid_tmp;
	}



	//	Abrir FIFOs
	int fd[4];
	for(i = 0; i < 4; i++){
		int fd_tmp;
		if( (fd_tmp = open(fifo[i], O_WRONLY)) == -1){
			perror("open()");
			exit(EXIT_FAILURE);
		}
		fd[i] = fd_tmp;
	}



	//	Esperar tempo de funcionamento
	//	do parque
	sleep(atoi(argv[2]));



	//	Fechar parque
	vehicle close_park = last_vehicle();
	int sizeof_vehicle = sizeof(vehicle);

	printf("Done! Closing park ...\n");
	for(i = 0; i < 4; i++){
		if(pthread_mutex_lock(&mutex[i]) != 0){
			perror("pthread_mutex_lock()");
			exit(EXIT_FAILURE);
		}
		if(write(fd[i],&close_park,sizeof_vehicle) != 0){
			perror("write()");
			exit(EXIT_FAILURE);
		}
	}



	//	Fechar FIFOs
	printf("Done! Closing FIFOs...\n");
	for(i = 0; i < 4; i++)
		if(close(fd[i]) != 0){
			perror("close()");
			exit(EXIT_FAILURE);
		}



	//	Esperar que os controladores
	//	terminem
	printf("Done! Waiting for threads to close...\n");

	for(i = 0; i < 4; i++){

		if(pthread_join(tid[i], NULL) != 0){
			perror("pthread_join()");
			exit(EXIT_FAILURE);
		}

		if(pthread_mutex_unlock(&mutex[i]) != 0){
			perror("pthread_mutex_unlock()");
			exit(EXIT_FAILURE);
		}
	}



	//	Desassociar e limpar
	//	memoria partilhada
	if(shmdt(mutex) != 0){
		perror("shmdt()");
		exit(EXIT_FAILURE);
	}

	if(shmctl(shmid, IPC_RMID, NULL) == -1){
		perror("shmctl()");
		exit(EXIT_FAILURE);
	}



	printf("Done! Unlinking FIFOs...\n");
	unlink(fifo[N]);
	unlink(fifo[S]);
	unlink(fifo[E]);
	unlink(fifo[O]);

	printf("Done!\n");
	return 0;
}

void *ctrl_thread(void *var){

	arrum_param param;

	param.i = *(size_t*) var;

	printf("ctrl_thread(\"%s\")\n",fifo[param.i]);


	//	Abrir o FIFO
	int fd = open(fifo[param.i], O_RDONLY);
	if(fd == -1){
		perror("open()");
		exit(EXIT_FAILURE);
	}

	pthread_t arrum;


	//	Ler veiculos do FIFO
	//	ate o parque fechar

	for(;param.v.id != -1; param.v = new_vehicle(fd)){



		//	Snal para fechar
		//	o parque
		if(param.v.id == -1){
			printf("close veicle\n");
			break;
		}

		if(&param.v == NULL){
			printf("got null\n");
			continue;
		}

		//	Read invalido
		if(
				param.v.id < 0 ||
				param.v.parking_duration <= 0 ||
				param.v.fifo_id <= 0
		)
			continue;

		//	Criar thread arrumador
		if(pthread_create(&arrum, NULL, arrum_thread, &param) != 0){
			perror("pthread_create()");
			exit(EXIT_FAILURE);
		}
	}

	//	Fechar FIFO
	if(close(fd) !=0){
		perror("close()");
		exit(EXIT_FAILURE);
	}

	printf("Closing ctrl_thread(\"%s\")\n",fifo[param.i]);

	return NULL;

}

void *arrum_thread(void *var){

	printf("arrum_thread()\n");

	//	Detach thread
	if(pthread_detach(pthread_self()) != 0){
		perror("pthread_detach()");
		exit(EXIT_FAILURE);
	}

	arrum_param param = *(arrum_param*) var;

	vehicle_info(&param.v);

	//	Caso hajam lugares livres
	if(empty_spots > 0){

		//	Decrementar lugares livres
		if(pthread_mutex_lock(&mutex[param.i]) != 0){
			perror("pthread_mutex_unlock()");
			exit(EXIT_FAILURE);
		}

		empty_spots--;

		if(pthread_mutex_unlock(&mutex[param.i]) != 0){
			perror("pthread_mutex_unlock()");
			exit(EXIT_FAILURE);
		}

		clock_t end = clock() + param.v.parking_duration;


		//	Esperar saida do carro
		while(clock() < end);


		//	Incrementar lugares livres
		if(pthread_mutex_lock(&mutex[param.i]) != 0){
			perror("pthread_mutex_unlock()");
			exit(EXIT_FAILURE);
		}

		empty_spots++;

		if(pthread_mutex_unlock(&mutex[param.i]) != 0){
			perror("pthread_mutex_unlock()");
			exit(EXIT_FAILURE);
		}
	}
	else{

	}

	return NULL;

}


vehicle new_vehicle(int fd){

	vehicle v;

	if(read(fd,&v,sizeof(vehicle)) <= 0){
		perror("read()");
		return *(vehicle*)NULL;
	}

	return v;
}

vehicle last_vehicle(){

	vehicle v;

	v.id = -1;

	return v;
}

void vehicle_info(vehicle *v){

	printf("Vehicle %d\nParking duration: %d\nFIFO id: %d\n\n",v->id,(int)v->parking_duration,v->fifo_id);
}
