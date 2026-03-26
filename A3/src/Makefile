CC=gcc
CFLAGS= -g -O0
FMT=indent

mysh: shell.c interpreter.c shellmemory.c executor.c shellmemory.h
	$(CC) $(CFLAGS) -D FRAME_STORE_SIZE=$(framesize) -D VAR_STORE_SIZE=$(varmemsize) -c shell.c interpreter.c shellmemory.c executor.c -pthread
	$(CC) $(CFLAGS) -o mysh shell.o interpreter.o shellmemory.o executor.o

style: shell.c shell.h interpreter.c interpreter.h shellmemory.c shellmemory.h executor.h
	$(FMT) $?

clean: 
	$(RM) mysh; $(RM) *.o; $(RM) *~

