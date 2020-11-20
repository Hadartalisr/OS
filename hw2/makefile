FLAGS = -o3 -D_POSIX_C_SOURCE=200809 -Wall -std=c11
LIBS = -lm

all: hw2

clean:
	rm ./hw2.o

hw2: hw2.o
	./hw2.o 

hw2.o : 
	gcc $(FLAGS) shell.c myshell.c -o hw2.o

