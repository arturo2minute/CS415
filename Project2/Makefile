all: part1 part2 part3 part4 iobound cpubound

part1: part1.o string_parser.o
	gcc -o part1 part1.o string_parser.o

part2: part2.o string_parser.o
	gcc -o part2 part2.o string_parser.o

part3: part3.o string_parser.o
	gcc -o part3 part3.o string_parser.o

part4: part4.o string_parser.o
	gcc -o part4 part4.o string_parser.o

%.o: %.c
	gcc -c $<

string_parser: string_parser.c string_parser.h
	gcc -c string_parser.c

iobound: iobound.c
	gcc iobound.c -o iobound

cpubound: cpubound.c
	gcc cpubound.c -o cpubound

clean:
	rm *.o part1 part2 part3 part4 iobound cpubound
