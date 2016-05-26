#include <time.h>
#include <pthread.h>

#ifndef STRUCTS_H
#define STRUCTS_H

#define	N	0
#define	S	1
#define	E	2
#define	O	3
#define NR_ENTR	4
#define FIFO_NAME_BUFFER_SIZE	100
#define LOG_LINE_MAX_SIZE 100
#define MAX_MESSAGE_SIZE	30

extern const char *fifo[NR_ENTR];
extern size_t entrance[NR_ENTR];

typedef struct {
	clock_t duration;
	int id;
} vehicle;

typedef struct {
	int tid;
	pthread_t t;
	vehicle v;
	size_t i;

} thread_param;

vehicle new_vehicle(int fd);
vehicle last_vehicle();
void vehicle_info(vehicle *v);

#endif /*STRUCTS_H*/
