CC = gcc
CFLAGS = -pthread

all: program_a program_b

program_a: A.c
	$(CC) $(CFLAGS) -o A A.c

program_b: program_B.c
	$(CC) $(CFLAGS) -o B B.c


clean:
	rm -f A B