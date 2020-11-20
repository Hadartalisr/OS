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
    int ret = strcmp(arglist[count-1],"&") == 0; // compare the last char to &
    return ret; 
}


/**
 * The method checks if a cmd is has a pipe. 
 * return 1 if is piped and changes the index to index of | in the arglist array 
 */
int is_piped_cmd(int count, char** arglist, int* index){
    int ret = 0;
    for (int i=0 ; i<count ; i++){
        if(strcmp(arglist[i],"|") == 0){
            ret = 1;
            *index = i;
        }
    }
    return ret; 
}


void child_sigint_handler(){
    printf("\n");
}


void shell_sigint_handler(){}


int run_proccess(int count, char** argslist, int is_background_cmd){
    pid_t pid = fork();
    if(pid < 0){ // system fork fuilare.
        fprintf(stderr, "ERROR - The OS could not fork pid - %d.\n", getppid());
        exit(EXIT_FAILURE);
    }
    if(pid == 0){ //child
        signal(SIGINT,&child_sigint_handler);
        if(is_background_cmd){ // need to delete the last char
            argslist[count-1] = NULL;
        }
        execvp(argslist[0], argslist);
        return 0;
    }
    else { //father
        if(!is_background_cmd){
            waitpid(pid, NULL, 0); // if  wait for the child to finish
        }
        return 0;
    }
}


int process_arglist(int count, char** arglist){
    int ret ;
    int index; 

    ret = is_piped_cmd(count, arglist, &index);
    if (ret){

    }
    else {
        ret = is_background_cmd(count, arglist);
        ret = run_proccess(count, arglist, ret);
    }
    return 1;
}





int prepare(void){
    signal(SIGINT,&shell_sigint_handler); // change the sigint for the shell
    return 0;
}


int finalize(void){
    // need to kill all the childs
    return 0;   
}