#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct my_node
{
  char* directory;
  struct my_node* next;
};

struct my_list
{
  struct my_node* head;
  struct my_node* tail;
};

pthread_mutex_t count_mutex;
pthread_cond_t count_threshold_cv;


//------------------------------------------------------------------------------------ 
//-------------     -------------     List Methods     -------------     -------------
//------------------------------------------------------------------------------------     
struct my_list* my_list_init(void){
  struct my_list* list = (struct my_list*)calloc(1,sizeof(struct my_list*));
  list->head = NULL;
  list->tail = NULL;
  return list;
}

/**
 * return 1 if list is empty, otherwise false.
 */
int is_empty(struct my_list *list){
  int i = 0;
  if(list == NULL){
    i = 1;
  }
  else if(list -> head == NULL && list->tail == NULL){
    i = 1;
  }
  return i;
}

/**
 * push directory into the tail of the list.
 */
int push (struct my_list* list, char* directory){
  struct my_node* node = (struct my_node*)calloc(1,sizeof(struct my_node*)); 
  if(node == NULL){
    perror("my_node calloc fault\n");
    return(EXIT_FAILURE);
  }
  node -> directory = directory;
  node -> next = NULL;
  if(list == NULL){
    perror("ERROR - list was not initialized.");
    return(EXIT_FAILURE);
  }
  if(is_empty(list)){
    list -> head = node;
    list -> tail = node;
    return EXIT_SUCCESS;
  }
  else{
    list->tail->next = node;
    list->tail = node;
  }
}

/**
 * pull directory from the head of the list.
 * if the list is empty return NULL.
 */
char* pull(struct my_list* list){
  struct my_node* h;
  char* new_directory;

  if(list == NULL){
    perror("ERROR - list was not initialized.");
    return(EXIT_FAILURE);
  }
  if(list->head == NULL){
    printf("list is empty.\n");
    return NULL;
  }

  h = list->head; // head node
  list->head = h->next;
  if(list-> head == NULL){ // the list is empty
    list->tail = NULL;
  }
  new_directory = h->directory;
  free(h);
  return new_directory;
}


int my_list_free(struct my_list* list){
  while(list -> head){
    free(list->head->directory);
    pull(list);
  }
  return(EXIT_SUCCESS);
}


//----------------------------------------------------------------------------
void * push(void *t) {
  long my_id = (long)t;

  for (int i = 0; i < 10; i++) {
    pthread_mutex_lock(&count_mutex);
    count++;

    // Check the value of count and signal waiting thread when condition is
    // reached.  Note that this occurs while mutex is locked.
    if (count == 10) {
      pthread_cond_signal(&count_threshold_cv);
      printf("push(): thread %ld, count = %d  Threshold reached.\n", my_id,
             count);
    }

    printf("push(): thread %ld, count = %d, unlocking mutex\n", my_id,
           count);
    pthread_mutex_unlock(&count_mutex);

    // Do some "work" so threads can alternate on mutex lock
    sleep(1);
  }
  pthread_exit(NULL);
}

//----------------------------------------------------------------------------
void * pull(void *t) {
  long my_id = (long)t;

  printf("Starting watch_count(): thread %ld\n", my_id);

  /*
   Lock mutex and wait for signal.  Note that the pthread_cond_wait
   routine will automatically and atomically unlock mutex while it waits.
   Also, note that if COUNT_LIMIT is reached before this routine is run by
   the waiting thread, the loop will be skipped to prevent pthread_cond_wait
   from never returning.
  */
  pthread_mutex_lock(&count_mutex);
  while (count < 10) {
    printf("pull(): thread %ld Meditating on condition variable.\n",
           my_id);
    pthread_cond_wait(&count_threshold_cv, &count_mutex);
    printf("pull(): thread %ld Condition signal received.\n", my_id);

    count += 125;

    printf("pull(): thread %ld count now = %d.\n", my_id, count);
  }
  pthread_mutex_unlock(&count_mutex);
  pthread_exit(NULL);
}

//----------------------------------------------------------------------------
// individual directories are searched by different threads
// argv[1] - search root directory
// argv[2] - search term
// argv[3] - num of threads ( > 0)
int main(int argc, char *argv[]) {
  
  int rc;
  long t1 = 1, t2 = 2, t3 = 3;
  pthread_t threads[3];
  pthread_attr_t attr;

  // Initialize mutex and condition variable objects
  pthread_mutex_init(&count_mutex, NULL);
  pthread_cond_init(&count_threshold_cv, NULL);

  // For portability, explicitly create threads in a joinable state
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  pthread_create(&threads[0], &attr, pull, (void *)t1);
  pthread_create(&threads[1], &attr, push, (void *)t2);
  pthread_create(&threads[2], &attr, push, (void *)t3);

  // Wait for all threads to complete
  for (int i = 0; i < NUM_THREADS; i++) {
    pthread_join(threads[i], NULL);
  }

  printf("Main(): Waited on %d  threads. Done.\n", NUM_THREADS);

  // Clean up and exit
  pthread_attr_destroy(&attr);
  pthread_mutex_destroy(&count_mutex);
  pthread_cond_destroy(&count_threshold_cv);
  pthread_exit(NULL);
}
//============================== END OF FILE =================================
