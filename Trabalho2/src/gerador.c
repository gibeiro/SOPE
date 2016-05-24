/*
- entre a geração de dois veiculos pode há um intervalo de tempo de 0, 1 ou 2 unidades de tempo (probabilidades no enunciado)
	srand(time(NULL))
- ciclo de geração de veiculos:
	int t = 0;
	while(t < atoi(argv[]))

 */


/*
thread vehicle

fd = open(FIFO,...,O_WRONLY|O_NONBLOCK);
if(fd != -1){
write(...,vehicle,...);
close(fd);
}
if(fd == -1){
//parque fechado
}
*/


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>

pthread_mutex_t *mutex;

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

	key_t key = ftok("sope",0);
	int shmid = shmget(key,0,0);
	mutex = (pthread_mutex_t*) shmat(shmid,0,0);

	time_t end = time(NULL) + (time_t) atoi(argv[1]);

	while(time(NULL) < end){

	}

	return 0;
}
