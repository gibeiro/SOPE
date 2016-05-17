typedef struct{

	int parking_durantion;
	int id;
	int fifo_id;
	char entrance;

} vehicle;

vehicle* new_vehicle(int fd);
