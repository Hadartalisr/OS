#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>


#ifdef DEBUG
	#define D 
#else	
	#define D for(;0;)
#endif



/**
 * The method return 1 is the cmd is a background cmd 
 */
int is_background_cmd(int count, char** arglist){
    int ret = (arglist[count-1] ==  "&"); // compare the last char to &
    return ret; 
}

/**
 * The method checks if a cmd is has a pipe. 
 * return 1 if is piped and changes the index to index of | in the arglist array 
 */
int is_piped_cmd(int count, char** arglist, int* index){
    int ret = 0;
    for (int i=0 ; i<count ; i++){
        if(arglist[i] == "|"){
            ret = 1;
            *index = i;
        }
    }
    return ret; 
}


int process_arglist(int count, char** arglist){
    int ret ;
    int index; 
    ret = is_background_cmd(count, arglist);
    if (ret){

    }
    else {
        ret = is_piped_cmd(count, arglist, index);
        if (ret){

        }
        else { // regulare cmd 

        }
    }
    int index;


}

int prepare(void){
    return 0;
}

int finalize(void){
     return 0;   
}