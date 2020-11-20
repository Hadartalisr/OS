FLAGS = -o3 -Wall -std=c11 

all: hw1
clean:
	rm ./*.o 

hw1: hw1.o
	./hw1.o

hw1.o : 
	gcc $(FLAGS) os.c -o hw1.o

pt: pt.o
	./pt.o 

pt.o: 
	gcc $(FLAGS) pt.c -o pt.o