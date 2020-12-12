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
  int module_status;

  if (argc < 4){
    perror("ERROR - Less then 4 arguments were passed.");
    return EXIT_FAILURE;
  }

  file_descriptor = open(argv[1], O_RDWR);
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

  module_status = write(file_descriptor, argv[3], strlen(argv[3]));
  if (module_status < 0){
    perror("ERROR - write module problem.");
    return EXIT_FAILURE;
  }   

  close(file_descriptor);

  return EXIT_SUCCESS;
  
}
