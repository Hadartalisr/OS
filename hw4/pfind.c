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
    return(EXIT_SUCCESS);
  }
  else{
    list->tail->next = node;
    list->tail = node;
  }
  return(EXIT_SUCCESS);
}


/**
 * pull directory from the head of the list.
 */
int pull(struct my_list* list, char** directory){
  struct my_node* h;

  if(list == NULL){
    perror("ERROR - list was not initialized.");
    return(EXIT_FAILURE);
  }
  if(list->head == NULL){
    printf("list is empty.\n");
    return(EXIT_FAILURE);
  }

  h = list->head; // head node
  list->head = h->next;
  if(list-> head == NULL){ // the list is empty
    list->tail = NULL;
  }
  if(directory != NULL){
    *directory = h->directory; 
  }
  free(h);
  return(EXIT_SUCCESS);
}


int my_list_free(struct my_list* list){
  while(list -> head){
    free(list->head->directory);
    pull(list, NULL);
  }
  return(EXIT_SUCCESS);
}


//--------------------------------------------------------------------------------------------- 
//-------------     -------------     Single Thread Methods     -------------     -------------
//---------------------------------------------------------------------------------------------


void* thread_func(void *t) {
  long my_id = (long)t;

  printf("Starting thread_func(): thread %ld\n", my_id);

  /*
   Lock mutex and wait for signal.  Note that the pthread_cond_wait
   routine will automatically and atomically unlock mutex while it waits.
   Also, note that if COUNT_LIMIT is reached before this routine is run by
   the waiting thread, the loop will be skipped to prevent pthread_cond_wait
   from never returning.
  */
  /*
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
  */
  pthread_exit((void *)&my_id);
}




//------------------------------------------------------------------------------------ 
//-------------     -------------     Main Methods     -------------     -------------
//------------------------------------------------------------------------------------


/**
 * argv[1] - search root directory
 * argv[2] - search term
 * argv[3] - num of threads ( > 0)
 */
int get_arguments(int argc, char* argv[],
  char** root, char** search_term, int* number_of_threads) {
  if (argc != 4){
    perror("ERROR - get_arguments : argc != 4");
    return(EXIT_FAILURE);
  }
  *root = argv[1];
  *search_term = argv[2];
  *number_of_threads = atoi(argv[3]);
  if(*number_of_threads < 1){
    return(EXIT_FAILURE);
  }
  return(EXIT_SUCCESS);
}


void check_status(int status){
  if(status == EXIT_FAILURE){
    exit(EXIT_FAILURE);
  }
}


int init_threads(pthread_t* threads, int number_of_threads){
  int rc ;
  for(int i = 0 ; i < number_of_threads; i++){
    rc = pthread_create(&threads[i], NULL, thread_func, (void *)&i);
    if(rc != EXIT_SUCCESS){
      perror("ERROR - init_threads : pthread_create failure.");
    }
  }
  return(EXIT_SUCCESS);
}


int wait_for_threads_to_finish(pthread_t* threads, int number_of_threads){
  int rc;
  void* status;

  for (int i = 0; i < number_of_threads; i++) {
    rc = pthread_join(threads[i], &status);
    if (rc) {
      fprintf(stderr, "ERROR in pthread_join(): %d\n",rc);
      exit(-1);
    }
    printf("wait_for_threads_to_finish: completed join with thread %d "
           "having a status of %ld\n",i, (long)status);
  }
  return(EXIT_SUCCESS);
}


int main(int argc, char *argv[]) {
  
  int status;

  int number_of_threads;
  char* search_term;
  char* root;
  pthread_t* threads;


  // get arguments
  status = get_arguments(argc, argv, &root, &search_term, &number_of_threads);
  check_status(status);


  // allocate memory for the threads
  threads = (pthread_t*)calloc(number_of_threads, sizeof(pthread_t*));
  if(threads == NULL){
    perror("ERROR - in calloc threads.\n");
    return(EXIT_FAILURE);
  }

  // Initialize mutex and condition variable objects
  pthread_mutex_init(&count_mutex, NULL);
  pthread_cond_init(&count_threshold_cv, NULL);


  //init_threads
  init_threads(threads, number_of_threads);
  wait_for_threads_to_finish(threads, number_of_threads);


  printf("Main(): Waited on %d  threads. Done.\n", number_of_threads);

  // Clean up and exit
  pthread_mutex_destroy(&count_mutex);
  pthread_cond_destroy(&count_threshold_cv);
  pthread_exit(NULL);
}
//============================== END OF FILE =================================
