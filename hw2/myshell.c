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
 * The method return 1 if the cmd is a background cmd 
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


int run_proccess(int count, char** argslist, int is_background_cmd){
    pid_t pid = fork();
    if(pid < 0){ // system fork fuilare.
        printf(stderr, "ERROR - The OS could not fork pid - %d.\n", getppid());
    }
    if(pid == 0){ //child
        if(is_background_cmd){ // need to delete the last char
            argslist[count-1] = NULL;
        }
        
    }
    else { //father
        if(!is_background_cmd){
            waitpid(pid, NULL, NULL); // if  wait for the child to finish
        }
    }
}


int process_arglist(int count, char** arglist){
    int ret ;
    int index; 

    ret = is_piped_cmd(count, arglist, index);
    if (ret){

    }
    ret = is_background_cmd(count, arglist);
    ret = run_proccess(count, arglist, ret);
    return 1;
}


int prepare(void){
    //signal(SIGINT,)
    return 0;
}

int finalize(void){
     return 0;   
}