FLAGS = -o3 -Wall -std=c11 

all: pt
clean:
	rm ./*.o 

pt: pt.o
	./pt.o

pt.o : 
	gcc $(FLAGS) pt.c -o pt.o