FLAGS = -o3 -Wall -std=c11 
LIBS = -lm

all: hw1
clean:
	rm ./3 ./test.o

hw1: hw1.o
	./3 

hw1.o : 
	gcc $(FLAGS) os.c pt.c

test: test.o
	./test.o 

test.o:
	gcc $(FLAGS) tester.c pt.c -o test.o $(LIBS)

