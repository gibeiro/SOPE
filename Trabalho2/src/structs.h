#include <time.h>
#include <pthread.h>

#ifndef STRUCTS_H
#define STRUCTS_H

#define	N	0
#define	S	1
#define	E	2
#define	O	3

extern const char *fifo[];
extern size_t entrance[];

typedef struct {
	clock_t duration;
	int id;
	int fifo_id;

} vehicle;

typedef struct {
	pthread_t tid;
	vehicle v;
	size_t i;

} thread_param;

vehicle new_vehicle(int fd);
vehicle last_vehicle();
void vehicle_info(vehicle *v);

#endif /*STRUCTS_H*/
