OBJS=etoken.o main.o
EXOBJS=etoken.o example.o

CC=gcc
LFLAGS=-Wall
CFLAGS=-Wall -c

# link object files and create the executable

etoken: $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o etoken

# alternative executable
example: $(EXOBJS)
	$(CC) $(LFLAGS) $(EXOBJS) -o example


# compile targets

etoken.o: etoken.h etoken.c
	$(CC) $(CFLAGS) etoken.c
main.o: main.c
	$(CC) $(CFLAGS) main.c
example.o: example.c
	$(CC) $(CFLAGS) example.c

clean:
	\rm -f *.o *.h.gch etoken example
