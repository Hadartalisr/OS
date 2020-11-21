#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>


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


void shell_ignore_sigint(){
    struct sigaction act;
    memset (&act, '\0', sizeof(act));
    act.sa_handler = SIG_IGN;
    if(sigaction(SIGINT,&act, NULL) == -1){
        perror("ERROR - sigaction failure");
    }
}


void shell_ignore_child_sigint(){
    struct sigaction act;
    memset (&act, '\0', sizeof(act));
    act.sa_handler = SIG_IGN;
    if(sigaction(SIGCHLD,&act, NULL) == -1){
        perror("ERROR - sigaction failure");
    }
}


void cmd_sigint(){
    struct sigaction act;
    memset (&act, '\0', sizeof(act));
    act.sa_handler = SIG_DFL;
    act.sa_flags = SA_RESTART;
    if(sigaction(SIGINT,&act, NULL) == -1){
        perror("ERROR - sigaction failure");
    }
}


int prepare(void){
    shell_ignore_sigint();
    shell_ignore_child_sigint();
    return 0;
}


int finalize(void){
    // need to make sure that there are no zombies in the background
    pid_t group_id = getpgid(getppid());
    killpg(group_id, SIGKILL);
    return 0;   
}


int run_proccess(int count, char** argslist, int is_background_cmd){
    pid_t group_id = getpgid(getpid());
    pid_t pid = fork();
    if(pid < 0){ // system fork fuilare.
        fprintf(stderr, "ERROR - The OS could not fork pid - %d.\n", getppid());
        exit(EXIT_FAILURE);
    }
    if(pid == 0){ //child
        setpgid( getpid() ,group_id);
        cmd_sigint();
        if(is_background_cmd){ // need to delete the last char
            argslist[count-1] = NULL;
        }
        execvp(argslist[0], argslist);
        // should not go in here - in case of error
        fprintf(stderr, "ERROR - invalid command : %s ...\n", argslist[0]);
        exit(EXIT_FAILURE);
    }
    else { //father
        if(!is_background_cmd){
            waitpid(pid, NULL, 0); // if  wait for the child to finish
        }
        return 1;
    }
}


int run_piped_proccess(int count, char** arglist, int index){
    arglist[index] = NULL;
    char** cmd_one = arglist;
    char** cmd_two = arglist + index + 1 ; // pointer to the first char array in the second cmd
    int pipefd[2];
    pid_t group_id = getpgid(getpid());
    if(pipe(pipefd)== -1){
        perror("Failed creating pipe");
        exit(EXIT_FAILURE);
    }
    /// pipefd[0] - reader ,  pipefd[1] -  writer
    pid_t pid = fork();
    if(pid < 0){ // system fork fuilare.
        fprintf(stderr, "ERROR - The OS could not fork pid - %d.\n", getppid());
        exit(EXIT_FAILURE);
    }
    else if (pid == 0){
        // child - the writer
        cmd_sigint();
        setpgid( getpid() ,group_id);
        close(pipefd[0]);
        while((dup2(pipefd[1], STDOUT_FILENO) == -1) && errno == EINTR){};
        close(pipefd[1]);
        execvp(cmd_one[0], cmd_one);
        // should not go in here - in case of error
        fprintf(stderr, "ERROR - invalid command : %s ...\n", cmd_one[0]);
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
            cmd_sigint();
            setpgid( getpid() ,group_id);
            close(pipefd[1]);
            while((dup2(pipefd[0], STDIN_FILENO) == -1) && errno == EINTR){};
            close(pipefd[0]);
            execvp(cmd_two[0], cmd_two);
            // should not go in here - in case of error
            fprintf(stderr, "ERROR - invalid command : %s ... \n", cmd_two[0]);
            exit(EXIT_FAILURE);
        }
        else{
            // parent - the shell process
            close(pipefd[0]);
            close(pipefd[1]);
            waitpid(pid, NULL, 0);
            waitpid(pid2, NULL, 0);
        }
    }
    return 1;
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


