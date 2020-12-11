#include "message_slot.h"    

#include <fcntl.h>      /* open */ 
#include <unistd.h>     /* exit */
#include <sys/ioctl.h>  /* ioctl */
#include <stdio.h>
#include <stdlib.h>


int main(int argc, char* argv[])
{

  int file_descriptor; 
  unsigned long channel_id;
  char buffer[BUF_LEN];
  int module_status;
  int status;

  if (argc < 3){
    perror("ERROR - Less then 3 arguments were passed.");
    return EXIT_FAILURE;
  }

  file_descriptor = open(argv[1], O_WRONLY);
  if (file_descriptor < 0){
    perror("ERROR - file_descriptor was not found.");
    return EXIT_FAILURE;
  }
  
  channel_id = atoi(argv[2]);

  module_status = ioctl(file_descriptor,MSG_SLOT_CHANNEL, channel_id);
  if (module_status < 0){
    perror("ERROR - ioctl module problem .");
    return EXIT_FAILURE;
  }

  module_status = read(file_descriptor, buffer, BUF_LEN);
  if (module_status < 0){
    perror("ERROR - read module problem.");
    return EXIT_FAILURE;
  }
  status = write(STDOUT_FILENO,buffer,BUF_LEN);
  if(status < 0){
    perror("ERROR - read module problem 2.");
    return EXIT_FAILURE;
  }   

  close(file_descriptor);

  return EXIT_SUCCESS;
  
}
