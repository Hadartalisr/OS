#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void* thread_func(void* thread_params){
    printf("i'm thread #%lu\n", pthread_self());
    printf("I've got %s\n", (char*)thread_params);
    pthread_exit((void*)5);
}

int main(int argc, char* argv[]){
    printf("main thread #%lu\n", pthread_self());  
    pthread_t thread_id ; 
    void* status; 
    int rc;
    rc = pthread_create(&thread_id, NULL, thread_func, (void*)("hello !"));

    pthread_join(thread_id, &status);
    printf("main - %d\n",(int*)status);

}