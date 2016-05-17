#include "viatura.h"

vehicle* new_vehicle(int fd){

	vehicle* v = (vehicle *)malloc(sizeof(vehicle *));

	if( read(fd,v,sizeof(vehicle) < 1)
		return NULL;
	else
		return v;
}
