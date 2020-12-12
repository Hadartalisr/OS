// Declare what kind of code we want
// from the header files. Defining __KERNEL__
// and MODULE allows us to access kernel-level
// code not usually available to userspace programs.
#undef __KERNEL__
#define __KERNEL__
#undef MODULE
#define MODULE

#include <linux/kernel.h>   /* We're doing kernel work */
#include <linux/module.h>   /* Specifically, a module */
#include <linux/fs.h>       /* for register_chrdev */
#include <linux/uaccess.h>  /* for get_user and put_user */
#include <linux/string.h>   /* for memset. NOTE - not string.h!*/
#include <linux/radix-tree.h> /* the kernel has this data structure */
#include <linux/slab.h> /* for kmalloc */
MODULE_LICENSE("GPL");
#include "message_slot.h"
#define DEVICE_RANGE_NAME "message_slot"

struct message_slot_info
{
  spinlock_t lock;
};

typedef struct FILE_CONFIG{
  int minor_num;
  unsigned long channel_id ; 
} file_config;

struct message
{
  char buffer[BUF_LEN]; // the messfage buffer
  int size; // the size of the message
};

// used to prevent concurent access into the same device
static int dev_open_flag = 0;
static struct message_slot_info device_info;
static struct radix_tree_root *minors[256]; // there can be at most 256 different message slots


//================== Radix Tree FUNCTIONS ===========================


int allocate_new_radix_tree(int minor_num){

  printk("allocate_new_radix_tree(%d)\n", minor_num);

  if(minor_num < 0 || minor_num >= 256){
    return SUCCESS;
  }
  minors[minor_num] = kmalloc(sizeof(struct radix_tree_root),GFP_KERNEL);
  INIT_RADIX_TREE(minors[minor_num],GFP_KERNEL);
  return SUCCESS;
}

int free_radix_tree(int minor_num){
  struct radix_tree_root *root = minors[minor_num];
  struct radix_tree_iter iter; 
  void ** slot;
  radix_tree_for_each_slot(slot, root, &iter, 0){
    kfree(*slot);
  }
  return SUCCESS; 
}

int free_all_minors(void){
  int i;

  printk("free_all_minors\n");

  for(i = 0; i < 256; i++){
    if(minors[i] != NULL){
      free_radix_tree(i);
    }
  };
  return SUCCESS;
}

// return the message for a given file
// if message doesnt exist and the channel_id is valid 
// - create new message and returns it.
struct message* get_message_from_file(struct file* file){
  int minor_num;
  unsigned long channel_id;
  struct radix_tree_root* root;
  struct message* message;

  printk("Invoking get_message_from_file(%p)\n", file);

  minor_num = ((file_config *)(file -> private_data))-> minor_num;
  printk(KERN_INFO "minor num - %d", minor_num);
  channel_id = ((file_config *)(file -> private_data))-> channel_id;
  printk(KERN_INFO "channel_id - %ld", channel_id);
  root = minors[minor_num];
  if(channel_id == 0){
    return NULL;
  }
  message = (struct message*)radix_tree_lookup(root, channel_id);
  if(message == NULL){
    message = (struct message*)kmalloc(sizeof(struct message*), GFP_KERNEL);
    if(message == NULL){
      return NULL;
    }
    message->size = 0;
    radix_tree_insert(root, channel_id, message);
  }

  printk("END - Invoking get_message_from_file(%p)\n", file);
  return message;

}



//================== DEVICE FUNCTIONS ===========================
static int device_open( struct inode* inode,
                        struct file*  file )
{
  unsigned long flags; // for spinlock
  file_config* fc;


  // spin_unlock
  printk("Invoking device_open(%p)\n", file);
  // We don't want to talk to two processes at the same time
  spin_lock_irqsave(&device_info.lock, flags);
  if( 1 == dev_open_flag )
  {
    spin_unlock_irqrestore(&device_info.lock, flags);
    return -EBUSY;
  }
  ++dev_open_flag;
  spin_unlock_irqrestore(&device_info.lock, flags);

  

  // allocate normal kernel ram 
  fc = kmalloc(sizeof(file_config),GFP_KERNEL); 
  fc->channel_id = 0; 
  fc->minor_num =  iminor(inode); // as was told in the recitation - 
  // we can get the minor num from the inode.
  file->private_data = fc; // file structure has this usable field

  if(minors[fc->minor_num] == NULL){
    allocate_new_radix_tree(fc->minor_num);
  }
  
  printk("END - Invoking device_open(%p)\n", file);
  return SUCCESS;
}

//---------------------------------------------------------------
static int device_release( struct inode* inode,
                           struct file*  file)
{
  unsigned long flags; // for spinlock
  file_config* fc;

  printk("Invoking device_release(%p,%p)\n", inode, file);

  // ready for our next caller
  spin_lock_irqsave(&device_info.lock, flags);
  --dev_open_flag;
  spin_unlock_irqrestore(&device_info.lock, flags);


  fc = (file_config*)file->private_data; 
  kfree(fc);
  return SUCCESS;
}

//---------------------------------------------------------------
// a process which has already opened
// the device file attempts to read from it
static ssize_t device_read( struct file* file,
                            char __user* buffer,
                            size_t       length,
                            loff_t*      offset )
{

  int status; 
  struct message* message;
  unsigned long channel_id;
  int i;

  printk("Invoking device_read(%p)\n", file);

  //check if ioctl was used before reading
  channel_id = ((file_config *)(file -> private_data))-> channel_id;
  if(channel_id == 0){
    printk(KERN_ERR "ERROR - channel_id = 0\n");
    return -EINVAL;
  }

  message = get_message_from_file(file);
  printk(KERN_INFO "buffer - %s", message->buffer);
  printk(KERN_INFO "size - %d", message->size);

  if(message->size == 0){
    return -EWOULDBLOCK;
  }

  if(length < message->size){
    return -ENOSPC;
  }

  for(i = 0 ; i < message->size ; i++){ 
    status = put_user(message->buffer[i], &buffer[i]);
    if(status < 0){
      return -EINVAL;
    }
  }

  printk("END - Invoking device_read(%p)\n", file);
  return i;
}

//---------------------------------------------------------------
// a processs which has already opened
// the device file attempts to write to it
static ssize_t device_write( struct file*       file,
                             const char __user* buffer,
                             size_t             length,
                             loff_t*            offset)
{

  int i;
  unsigned long channel_id;
  struct message* message;

  printk("Invoking device_write(%p)\n", file);
  
  printk(KERN_INFO "length - (%ld)\n", length);
  printk(KERN_INFO "buffer - (%s)\n", buffer);

  if(buffer == NULL || length <= 0 || length > BUF_LEN){
    printk(KERN_ERR "ERROR - EMSGSIZE\n");
    return -EMSGSIZE;
  }
  
  printk(KERN_INFO "The buffer to be writen - \n %s", buffer);

  //check if channel has been set
  channel_id = ((file_config *)(file -> private_data))-> channel_id;
  if(channel_id == 0){
    printk(KERN_ERR "ERROR - channel_id = 0\n");
    return -EINVAL;
  }

  message = get_message_from_file(file);

  if(message == NULL){
    printk(KERN_ERR "ERROR - message\n");
    return -EINVAL;
  }

  for(i = 0 ; i < length ; i++){
    get_user(message->buffer[i], &buffer[i]);
  }

  if(i == length){ // OK
    message->size = length;
  }
  else{
    message->size = 0;
    return -1;
  }
 
  printk("END - Invoking device_write(%p)\n", file);
  // return the number of input characters used
  return i;
}

//----------------------------------------------------------------
static long device_ioctl( struct   file* file,
                          unsigned int   ioctl_command_id,
                          unsigned long  ioctl_param )
{
  // Make sure that the CMD and the param (channel_id) are valid 
  if((MSG_SLOT_CHANNEL != ioctl_command_id ) || (ioctl_param <= 0) )
  {
    return -EINVAL;
  }
  // change the configuration in the file so its inner private data
  // which holds the channel will be updated to ioctl_param 
  ((file_config *)(file -> private_data))-> channel_id = ioctl_param;

  return SUCCESS;
}






//==================== DEVICE SETUP =============================

// This structure will hold the functions to be called
// when a process does something to the device we created
struct file_operations Fops =
{
  .owner	  = THIS_MODULE, 
  .read           = device_read,
  .write          = device_write,
  .open           = device_open,
  .unlocked_ioctl = device_ioctl,
  .release        = device_release,
};

//---------------------------------------------------------------
// Initialize the module - Register the character device
static int __init simple_init(void)
{
  int rc = -1;
  // init dev struct
  memset( &device_info, 0, sizeof(struct message_slot_info) );
  spin_lock_init( &device_info.lock );


  // Register driver capabilities. Obtain major num
  rc = register_chrdev( MAJOR_NUM, DEVICE_RANGE_NAME, &Fops );

  // Negative values signify an error
  if( rc < 0 )
  {
    printk( KERN_ALERT "%s registraion failed for  %d\n", DEVICE_RANGE_NAME, MAJOR_NUM );
    return rc;
  }

  printk(KERN_ALERT "\n\n\n\nRegisteration is successful.\n\n\n\n");
  
  return SUCCESS;
}

//---------------------------------------------------------------
static void __exit simple_cleanup(void)
{
  // Unregister the device - Should always succeed
  unregister_chrdev(MAJOR_NUM, DEVICE_RANGE_NAME);
  free_all_minors();
}



//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);

//========================= END OF FILE =========================
