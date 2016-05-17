CFLAGS	= -c -Wall
LDFLAGS = -pthread
CC	= gcc

all: gerador parque

gerador: gerador.o
	@mkdir -p bin
	@$(CC) obj/$< $(LDFLAGS) -o bin/$@

parque: parque.o
	@mkdir -p bin
	@$(CC) obj/$< $(LDFLAGS) -o bin/$@

gerador.o:
	@mkdir -p obj
	@$(CC) src/gerador.c $(CFLAGS)
	@mv $@ obj	
	
parque.o:
	@mkdir -p obj
	@$(CC) src/parque.c $(CFLAGS)
	@mv $@ obj

clean:
	@rm -r bin obj
