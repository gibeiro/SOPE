CFLAGS	= -Wall -pthread -D_REENTRANT -lpthread -lrt

all:
	@mkdir -p bin
	@gcc  src/parque.c src/structs.c -o bin/parque $(CFLAGS) 
	@gcc  src/gerador.c src/structs.c -o bin/gerador $(CFLAGS)
		
clean:
	@rm -r bin