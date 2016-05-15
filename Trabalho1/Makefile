CFLAGS	= -c -Wall
CC	= gcc

all: rmdup lsdir rlsdir	

rmdup: rmdup.o
	@mkdir -p bin
	@$(CC) obj/$< -o bin/$@

lsdir: lsdir.o
	@mkdir -p bin
	@$(CC) obj/$< -o bin/$@

rlsdir: rlsdir.o
	@mkdir -p bin
	@$(CC) obj/$< -o bin/$@

rmdup.o:
	@mkdir -p obj
	@$(CC) src/rmdup.c $(CFLAGS)
	@mv $@ obj

lsdir.o:
	@mkdir -p obj
	@$(CC) src/lsdir.c $(CFLAGS)
	@mv $@ obj
	
rlsdir.o:
	@mkdir -p obj
	@$(CC) src/rlsdir.c $(CFLAGS)
	@mv $@ obj	

clean:
	@rm -r bin obj