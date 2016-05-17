#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

time_t current_time;
time_t closing_time;

void *main_thread_func(void *var);

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

	pthread_t main_thread;

	pthread_create(&main_thread, NULL, main_thread_func, (void*) argv);

	pthread_join(main_thread, NULL);

	return 0;
}

void *main_thread_func(void *var){

	char **argv = (char**) var;	

	current_time = time(NULL);

	closing_time = current_time + (time_t) atoi(argv[2]);	

	printf("time: %d\nend: %d\n",(int) current_time,(int) closing_time);

	return NULL;

}
