CC=gcc
CFLAGS=
FMT=indent

ifdef framesize
CFLAGS += -DFRAME_STORE_SIZE=$(framesize) -DVAR_STORE_SIZE=$(varmemsize) -DPAGING_MODE
endif

mysh: shell.c interpreter.c shellmemory.c pcb.c ready_queue.c scheduler.c
	$(CC) $(CFLAGS) -c shell.c interpreter.c shellmemory.c pcb.c ready_queue.c scheduler.c
	$(CC) $(CFLAGS) -o mysh shell.o interpreter.o shellmemory.o pcb.o ready_queue.o scheduler.o -lpthread

style: shell.c shell.h interpreter.c interpreter.h shellmemory.c shellmemory.h
	$(FMT) $?

clean:
	$(RM) mysh; $(RM) *.o; $(RM) *~
