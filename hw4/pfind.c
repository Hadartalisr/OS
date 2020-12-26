#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

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

struct my_list* list;
char* search_term;
int number_of_threads;


// the amount of threads which are waiting for new directory
// if all of them are waiting - we have finished the tree scan
pthread_mutex_t waiting_threads_mutex;

// list locks
pthread_mutex_t list_lock;
pthread_cond_t directory_was_pushed;


// number of files we found
pthread_mutex_t files_count_mutex;
int files_count = 0;


//------------------------------------------------------------------------------------ 
//-------------     -------------     List Methods     -------------     -------------
//------------------------------------------------------------------------------------     

int my_list_init(void){
  list = (struct my_list*)calloc(1,sizeof(struct my_list*));
  if(list == NULL){
    perror("ERROR - my_list_init : cound not allocate list.\n");
    return(EXIT_FAILURE);
  }
  list->head = NULL;
  list->tail = NULL;
  return(EXIT_SUCCESS);
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
int push (char* directory){
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
int pull(char** directory){
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


int my_list_free(void){
  while(list -> head){
    free(list->head->directory);
    pull(NULL);
  }
  return(EXIT_SUCCESS);
}


//--------------------------------------------------------------------------------------------- 
//-------------     -------------     Single Thread Methods     -------------     -------------
//---------------------------------------------------------------------------------------------

void check_status(int status){
  if(status == EXIT_FAILURE){
    exit(EXIT_FAILURE);
  }
}


int increase_files_count(){
  files_count += 1;
  return(EXIT_SUCCESS);
}


int handle_directory(char* dir_path, char* file_name){
  if((strcmp(file_name,".") == 0) || (strcmp(file_name,".") == 1)){
    return EXIT_SUCCESS;
  }
  char* new_dir_path = malloc(sizeof(char)* PATH_MAX);
  if(new_dir_path == NULL){
    perror("ERROR - handle_directory : could not allocate memory for dir.");
    return(EXIT_FAILURE);
  }
  pthread_mutex_lock(&list_lock);
  push(new_dir_path);
  pthread_mutex_unlock(&list_lock);
  pthread_cond_broadcast(&directory_was_pushed);
  return(EXIT_SUCCESS);
}


int handle_file(char* file_path, char* file_name){
  if((strcmp(file_name,".") == 0) || (strcmp(file_name,".") == 1)){
    return EXIT_SUCCESS;
  }
  if(strstr(file_name,search_term) != NULL){
    pthread_mutex_lock(&files_count_mutex);
    increase_files_count();
    pthread_mutex_unlock(&files_count_mutex);
    printf("%s\n", file_path);
  }
  return(EXIT_SUCCESS);
}


int handle_directory_from_list(char* directory_path){
  struct dirent* dir_entry;
  DIR* directory;
  char path[PATH_MAX];
  int rc = 0;

  if((directory = opendir(directory_path)) == NULL){
    fprintf(stderr,"ERROR - handle_directory : could not open path %s.\n",
    directory_path);
    return(EXIT_SUCCESS);
  }

  while((dir_entry = readdir(directory)) != NULL){
    struct stat my_stat;

    // concat the new path of the file/dir to the existing path
    strcpy(path, directory_path);
    if(directory_path[strlen(directory_path)-1] != '/'){
      strcat(path, "/");
    }
    strcat(path,dir_entry->d_name);

    rc = lstat(path, &my_stat); // we use lstat because softlink can exist. 
    if(rc < 0){
      fprintf(stderr, "ERROR - lstat couldn't handle path: %s\n", path);
    }
    if(S_ISDIR(my_stat.st_mode)){
      rc = handle_directory(path, dir_entry->d_name);
      check_status(rc);
    }
    else{
      rc = handle_file(path, dir_entry->d_name);
      check_status(rc);
    }

  }

  closedir(directory);
  return(EXIT_SUCCESS);
}









void* thread_func(void *t) {
  long my_id = (long)t;

  printf("Starting thread_func(): thread %ld\n", my_id);
  

  /*
   Lock mutex and wait for signal.  Note that the pthread_cond_wait
   routine will automatically and atomically unlock mutex while it waits.
   Also, note that if COUNT_LIMIT is reached before this routine is run by
   the waiting thread, the loop will be skipped to prevent pthread_cond_wait
   from never returning.
  
  
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
  char** root) {
  if (argc != 4){
    perror("ERROR - get_arguments : argc != 4");
    return(EXIT_FAILURE);
  }
  *root = argv[1];
  search_term = argv[2];
  number_of_threads = atoi(argv[3]);
  if(number_of_threads < 1){
    return(EXIT_FAILURE);
  }
  return(EXIT_SUCCESS);
}



int init_threads(pthread_t* threads){
  int rc ;
  for(int i = 0 ; i < number_of_threads; i++){
    rc = pthread_create(&threads[i], NULL, thread_func, (void *)&i);
    if(rc != EXIT_SUCCESS){
      perror("ERROR - init_threads : pthread_create failure.");
    }
  }
  return(EXIT_SUCCESS);
}


int wait_for_threads_to_finish(pthread_t* threads){
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


/**
 * Initialize mutex and condition variable objects
 */
int oninit_mutex(void){
  pthread_mutex_init(&files_count_mutex, NULL);
  pthread_mutex_init(&list_lock, NULL);
  pthread_mutex_init(&waiting_threads_mutex, NULL);
  pthread_cond_init(&directory_was_pushed, NULL);
  return(EXIT_SUCCESS);
}


/**
 * 
 */
int ondestroy_mutex(void){
  pthread_mutex_destroy(&files_count_mutex);  
  pthread_mutex_destroy(&list_lock);
  pthread_mutex_destroy(&waiting_threads_mutex);
  pthread_cond_destroy(&directory_was_pushed);
  return(EXIT_SUCCESS);
}



int main(int argc, char *argv[]) {
  
  int status;
  char* root;
  pthread_t* threads;


  // get arguments
  status = get_arguments(argc, argv, &root);
  check_status(status);

  status = my_list_init();
  check_status(status);

  status = oninit_mutex();
  check_status(status);


  // allocate memory for the threads
  threads = (pthread_t*)calloc(number_of_threads, sizeof(pthread_t*));
  if(threads == NULL){
    perror("ERROR - in calloc threads.\n");
    return(EXIT_FAILURE);
  }


  init_threads(threads);
  wait_for_threads_to_finish(threads);

  
  printf("Done searching, found %d files\n", files_count);


  status = ondestroy_mutex();
  check_status(status);
  my_list_free();


  pthread_exit(NULL);
}
//============================== END OF FILE =================================
