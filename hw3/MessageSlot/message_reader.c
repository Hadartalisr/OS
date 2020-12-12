#include "message_slot.h"    

#include <fcntl.h>      /* open */ 
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[])
{

  int file_descriptor; 
  unsigned long channel_id;
  char buffer[BUF_LEN];
  int module_status;
  int status;
  ssize_t read_status;

  if (argc < 3){
    perror("ERROR - Less then 3 arguments were passed.");
    return EXIT_FAILURE;
  }

  file_descriptor = open(argv[1], O_RDONLY);
  if (file_descriptor < 0){
    perror("ERROR - file_descriptor was not found.");
    return EXIT_FAILURE;
  }
  
  channel_id = atoi(argv[2]);


  module_status = ioctl(file_descriptor, MSG_SLOT_CHANNEL, channel_id);
  if (module_status < 0){
    perror("ERROR - ioctl module problem .");
    return EXIT_FAILURE;
  }

  read_status = read(file_descriptor, buffer, BUF_LEN);
  if (read_status < 0){
    perror("ERROR - read module problem !");
    return EXIT_FAILURE;
  }
  

  status = write(STDOUT_FILENO, buffer, module_status);
  if(status < 0){
    perror("ERROR - read module problem 2.");
    return EXIT_FAILURE;
  }   

  status = close(file_descriptor);
  if (status < 0){
    perror("ERROR - file_descriptor was not closed.");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
  
}
