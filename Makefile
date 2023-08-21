CC = gcc
CFLAGS= -ansi -Wall -g -O0 -Wwrite-strings -Wshadow -fstack-protector-all -pedantic-errors 
PROGS = d8sh

all: $(PROGS)

d8sh: d8sh.o executor.o lexer.o parser.tab.o
	$(CC) -lreadline -o d8sh d8sh.o executor.o lexer.o parser.tab.o

d8sh.o: d8sh.c executor.h lexer.h
	$(CC) $(CFLAGS) -c d8sh.c

executor.o: executor.c executor.h command.h
	$(CC) $(CFLAGS) -c executor.c

lexer.o: lexer.c parser.tab.h
	$(CC) $(CFLAGS) -c lexer.c

parser.tab.o: parser.tab.c command.h
	$(CC) $(CFLAGS) -c parser.tab.c
clean: 
	rm -f *.o shell_jr
