OBJS=etoken.o main.o

CC=gcc
LFLAGS=-Wall
CFLAGS=-Wall -c

etoken: $(OBJS)
	$(CC) $(LFLAGS) $(OBJS) -o etoken

etoken.o: etoken.h etoken.c
	$(CC) $(CFLAGS) etoken.c
main.o: main.c
	$(CC) $(CFLAGS) main.c

clean:
	\rm -f *.o *.h.gch etoken
