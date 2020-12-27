FLAGS = -o3 -Wall -std=c11 -D_POSIX_C_SOURCE=200809 
LIBS = -pthread

all: hw4
	
clean:
	rm ./*.o

hw4: hw4.o
	./hw4.o ~/OS .c 5

hw4.o:
	gcc $(FLAGS) pfind.c -o hw4.o $(LIBS)
 
