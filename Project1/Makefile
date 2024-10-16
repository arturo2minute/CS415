all: Project1

Project1: command.o main.o string_parser.o
	gcc -o Project1 command.o main.o string_parser.o

command.o: command.c command.h string_parser.c string_parser.h
	gcc -c command.c string_parser.c

main.o: main.c
	gcc -c main.c

clean:
	rm -f Project1 *.o
