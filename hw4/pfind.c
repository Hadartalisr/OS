#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
# include <errno.h>


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
pthread_t* threads;


// if there are still running threads
pthread_mutex_t is_running_mutex;//
int isRunning = 1;

// the amount of threads which are waiting for new directory
// if all of them are waiting - we have finished the tree scan
pthread_mutex_t waiting_threads_mutex;
pthread_cond_t another_waiting_thread;
int waiting_threads = 0;

// to count if all the threads have died.
pthread_mutex_t died_threads_mutex;
pthread_cond_t another_thread_died;
int died_threads = 0;

// to check one by one if there are running threads
pthread_mutex_t check_running_threads;


// list locks - for pull and push
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
int is_empty(){
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
  //printf("push - %s \n", directory);
  //sleep(1);
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
  //printf("pull \n");
  //sleep(1);
  struct my_node* h = (struct my_node*)calloc(1,sizeof(struct my_node*));

  if(list == NULL){
    perror("ERROR - list was not initialized.");
    return(EXIT_FAILURE);
  }
  if(list->head == NULL){
    //printf("list is empty.\n");
    return(EXIT_FAILURE);
  }

  h = list->head; // head node
  list->head = h->next;

  if(list-> head == NULL){ // the list is empty
    //printf("now the list is empty \n");
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


int increase_files_count(){
  files_count += 1;
  return(EXIT_SUCCESS);
}


int handle_directory(char* dir_path, char* dir_name){
  //printf("handle_directory -- %s \n", dir_path);
  if((strcmp(dir_name,".") == 0) || (strcmp(dir_name,"..") == 0)){
    return EXIT_SUCCESS;
  }
  char* new_dir_path = malloc(sizeof(char)* PATH_MAX);
  if(new_dir_path == NULL){
    fprintf(stderr, "ERROR - handle_directory : could not allocate memory for dir.");
    return(EXIT_FAILURE);
  }
  strcpy(new_dir_path, dir_path);
  pthread_mutex_lock(&list_lock);
  push(new_dir_path);
  pthread_mutex_unlock(&list_lock);
  pthread_cond_signal(&directory_was_pushed);
  return(EXIT_SUCCESS);
}


int handle_file(char* file_path, char* file_name){
  //printf("handle_file - %s \n", file_path);
  if((strcmp(file_name,".") == 0) || (strcmp(file_name,"..") == 0)){
    return EXIT_SUCCESS;
  }
  if(strstr(file_name,search_term) != NULL){
    pthread_mutex_lock(&files_count_mutex);
    increase_files_count();
    pthread_mutex_unlock(&files_count_mutex);
    printf("%s\n",file_path);
  }
  return(EXIT_SUCCESS);
}


int handle_directory_from_list(char* directory_path){
  //printf("handle_directory_from_list - %s \n", directory_path);
  struct dirent* dir_entry;
  DIR* directory;
  char path[PATH_MAX];
  int rc = 0;

  directory = opendir(directory_path);
  if(directory == NULL){
    if(errno == EACCES){
      printf("Directory %s Permission denied.\n", directory_path);
      return(EXIT_SUCCESS);
    }
    else{
      fprintf(stderr,"ERROR - handle_directory : could not open path %s.\n",
        directory_path);
      return(EXIT_FAILURE);
    }
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
      if(rc == EXIT_FAILURE){
        return(EXIT_FAILURE);
      }
    }
    else{
      rc = handle_file(path, dir_entry->d_name);
      if(rc == EXIT_FAILURE){
        return(EXIT_FAILURE);
      }
    }
  }

  closedir(directory);
  return(EXIT_SUCCESS);

}


int get_is_running(void){
  int i = 0 ;
  pthread_mutex_lock(&is_running_mutex);
  i = isRunning;
  pthread_mutex_unlock(&is_running_mutex);
  return(i);
}

int get_has_all_died(void){
  int i = 0 ;
  pthread_mutex_lock(&died_threads_mutex);
  if(number_of_threads == died_threads){
    i = 1;
  }
  pthread_mutex_unlock(&died_threads_mutex);
  return(i);
}


void thread_wait_for_directory(){
  // increase the number of waiting threads
  pthread_mutex_lock(&waiting_threads_mutex);
  //printf("I'm waiting \n");
  waiting_threads++;
  sleep(0);
  pthread_cond_broadcast(&another_waiting_thread);
  pthread_mutex_unlock(&waiting_threads_mutex);
  
  // each time one thread is waiting for new directory to get into the list
  pthread_mutex_lock(&list_lock);
  pthread_cond_wait(&directory_was_pushed, &list_lock);
    pthread_mutex_lock(&waiting_threads_mutex);
      waiting_threads--;
    pthread_mutex_unlock(&waiting_threads_mutex);
  pthread_mutex_unlock(&list_lock);

}



void* thread_func(void *t) {
  long my_id = (long)t;
  int status;

  //printf("Starting thread_func(): thread %ld\n", my_id);
  
  while(1) {
    if(get_is_running() == 0){
      break;
    }  

    pthread_mutex_lock(&list_lock);
    if(is_empty() == 0){
      char* directory = (char*)calloc(1,sizeof(char*));
      pull(&directory);
      pthread_mutex_unlock(&list_lock);
      status = handle_directory_from_list(directory);
      if(status == EXIT_FAILURE){
        pthread_mutex_lock(&died_threads_mutex);
        died_threads += 1;
        pthread_mutex_unlock(&died_threads_mutex);
        pthread_cond_signal(&another_thread_died);
        pthread_exit(NULL);
      }
    }
    else{ // the list is empty
      pthread_mutex_unlock(&list_lock);
      thread_wait_for_directory();
    }
    sleep(0); // let other threads the ability to run
  }
   
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


int init_threads(){
  int rc ;
  for(int i = 0 ; i < number_of_threads; i++){
    rc = pthread_create(&(threads[i]), NULL, thread_func, (void *)&i);
    if(rc != EXIT_SUCCESS){
      perror("ERROR - init_threads : pthread_create failure.");
      exit(EXIT_FAILURE);
    }
  }
  return(EXIT_SUCCESS);
}


int ondestroy_threads(){
  int rc ;
  for(int i = 0 ; i < number_of_threads; i++){
    rc = pthread_cancel(threads[i]);
    if(rc != EXIT_SUCCESS){
      perror("ERROR - ondestroy_threads : pthread_cancel failure.");
    }
  }
  return(EXIT_SUCCESS);
}


int wait_for_threads_to_finish(){

  while(1){
    if(get_is_running() == 0 || get_has_all_died() == 1){
      break;
    }
    pthread_mutex_lock(&waiting_threads_mutex);
    //sleep(1);
    //printf("waiting_threads - %d\n", waiting_threads);
    if(waiting_threads == number_of_threads){
      pthread_mutex_lock(&is_running_mutex);
      isRunning = 0;
      pthread_mutex_unlock(&is_running_mutex);
      break;
    }
    pthread_cond_wait(&another_waiting_thread, &waiting_threads_mutex);
    pthread_mutex_unlock(&waiting_threads_mutex);
  }
  
  return(EXIT_SUCCESS);
}


/**
 * Initialize mutex and condition variable objects
 */
int oninit_mutex(void){
  pthread_cond_init(&another_waiting_thread, NULL);
  pthread_mutex_init(&files_count_mutex, NULL);
  pthread_mutex_init(&list_lock, NULL);
  pthread_mutex_init(&check_running_threads, NULL);
  pthread_mutex_init(&waiting_threads_mutex, NULL);
  pthread_cond_init(&directory_was_pushed, NULL);
  return(EXIT_SUCCESS);
}


/**
 * 
 */
int ondestroy_mutex(void){
  pthread_cond_destroy(&another_waiting_thread);
  pthread_mutex_destroy(&files_count_mutex);  
  pthread_mutex_destroy(&list_lock);
  pthread_mutex_destroy(&check_running_threads);
  pthread_mutex_destroy(&waiting_threads_mutex);
  pthread_cond_destroy(&directory_was_pushed);
  return(EXIT_SUCCESS);
}



int main(int argc, char *argv[]) {
  
  int status;
  char* root;


  // get arguments
  status = get_arguments(argc, argv, &root);
  if(status == EXIT_FAILURE){
    exit(EXIT_FAILURE);
  }

  status = my_list_init();
  if(status == EXIT_FAILURE){
    exit(EXIT_FAILURE);
  }


  status = oninit_mutex();
  if(status == EXIT_FAILURE){
    exit(EXIT_FAILURE);
  }

  // allocate memory for the threads
  threads = (pthread_t*)calloc(number_of_threads, sizeof(pthread_t*));
  if(threads == NULL){
    perror("ERROR - in calloc threads.\n");
    return(EXIT_FAILURE);
  }


  init_threads();
  push(root);
  wait_for_threads_to_finish(threads);
  ondestroy_threads();

  
  printf("Done searching, found %d files\n", files_count);


  status = ondestroy_mutex();
  if(status == EXIT_FAILURE){
    exit(EXIT_FAILURE);
  }
  my_list_free();

  return(EXIT_SUCCESS);
}
//============================== END OF FILE =================================
