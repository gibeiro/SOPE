CFLAGS	=	-Wall 
LDFLAGS	=	-lpthread -lrt
#-lpthread -lrt

all:
	@mkdir -p bin
	@gcc $(CFLAGS) src/parque.c src/structs.c -o bin/parque $(LDFLAGS) 
	@gcc $(CFLAGS) src/gerador.c src/structs.c -o bin/gerador $(LDFLAGS)
		
clean:
	@rm -r bin