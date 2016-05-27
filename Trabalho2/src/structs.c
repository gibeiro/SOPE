#include <stdio.h>
#include <fcntl.h>
#include "structs.h"

const char *fifo[] = {
		"fifoN",
		"fifoS",
		"fifoE",
		"fifoO"
};

size_t entrance[] = {N,S,E,O};

vehicle last_vehicle(){

	vehicle v;

	v.id = -1;

	return v;
}

void vehicle_info(vehicle *v){

	printf("Vehicle %d\nParking duration: %d\n",v->id,(int)v->duration);
}
