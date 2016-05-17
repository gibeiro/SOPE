/*
- entre a geração de dois veiculos pode há um intervalo de tempo de 0, 1 ou 2 unidades de tempo (probabilidades no enunciado)
	srand(time(NULL))
- ciclo de geração de veiculos:
	int t = 0;
	while(t < atoi(argv[]))

*/

#include <stdio.h>
#include <stdlib.h>

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

	return 0;
}
