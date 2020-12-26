FLAGS = -o3 -Wall -std=c11 
LIBS = -lm -pthread

all: hw4
	
clean:
	rm ./*.o

hw4: hw4.o
	./hw4.o

hw4.o:
	gcc $(FLAGS) mutex_ex3.c -o hw4.o $(LIBS)
 
