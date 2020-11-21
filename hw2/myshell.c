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


void child_sigint_handler(){ // terminate the child
    printf("\n");
}

void shell_sigint_handler(){} // need to ignore the default sigint


void shell_sigchild_handler(){
    
}


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
        // should not go in here - in case of error
        fprintf(stderr, "ERROR - invalid command : %s ... ", argslist[0]);
        exit(EXIT_FAILURE);
    }
    else { //father
        if(!is_background_cmd){
            waitpid(pid, NULL, 0); // if  wait for the child to finish
        }
        return 0;
    }
}


int run_piped_proccess(int count, char** arglist, int index){
    arglist[index] = NULL;
    char** cmd_one = arglist;
    char** cmd_two = arglist + index + 1 ; // pointer to the first char array in the second cmd
    int pipefd[2];
    if(pipe(pipefd)== -1){
        perror("Failed creating pipe");
        exit(EXIT_FAILURE);
    }
    int readerfd = pipefd[0];
    int writerfd = pipefd[1];

    pid_t pid = fork();
    if(pid < 0){ // system fork fuilare.
        fprintf(stderr, "ERROR - The OS could not fork pid - %d.\n", getppid());
        exit(EXIT_FAILURE);
    }
    else if (pid == 0){
        // child - the writer
        close(readerfd);
        dup2(writerfd, STDOUT_FILENO);
        execvp(cmd_one[0], cmd_one);
        // should not go in here - in case of error
        fprintf(stderr, "ERROR - invalid command : %s ... ", cmd_one[0]);
        exit(EXIT_FAILURE);
    }
    else {
        // parent - the shell process + the reader
        pid_t pid2 = fork();
        if(pid2 < 0){ // system fork fuilare.
            fprintf(stderr, "ERROR - The OS could not fork pid - %d.\n", getppid());
            exit(EXIT_FAILURE);
        }
        else if(pid2 == 0){
            // child - the reader process
            close(writerfd);
            dup2(readerfd, STDIN_FILENO);
            execvp(cmd_two[0], cmd_two);
            // should not go in here - in case of error
            fprintf(stderr, "ERROR - invalid command : %s ... ", cmd_two[0]);
            exit(EXIT_FAILURE);

        }
        else{
            // parent - the shell process
            signal(SIGINT,&child_sigint_handler);
            waitpid(pid, NULL, 0);
            waitpid(pid2, NULL, 0);
            close(readerfd);
            close(writerfd);

        }
    }
    return 0;
}


int process_arglist(int count, char** arglist){
    int ret ;
    int index; 

    ret = is_piped_cmd(count, arglist, &index);
    if (ret){
        run_piped_proccess(count, arglist, index);
    }
    else {
        ret = is_background_cmd(count, arglist);
        ret = run_proccess(count, arglist, ret);
    }
    return 1;
}


int prepare(void){
    signal(SIGINT,&shell_sigint_handler); // change the sigint for the shell
    signal(SIGCHLD,&child_sigint_handler);
    return 0;
}


int finalize(void){
    // need to kill all the childs
    return 0;   
}