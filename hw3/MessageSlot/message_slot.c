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

typedef struct MESSAGE
{
  char buffer[BUF_LEN]; // the messfage buffer
  int size; // the size of the message
} message;

// used to prevent concurent access into the same device
static int dev_open_flag = 0;
static struct message_slot_info device_info;
static struct radix_tree_root *minors[256]; // there can be at most 256 different message slots


//================== Radix Tree FUNCTIONS ===========================


int allocate_new_radix_tree(int minor_num){
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
  for(i = 0; i < 256; i++){
    if(minors[i] != NULL){
      free_radix_tree(i);
    }
  };
  return SUCCESS;
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

  //invalid argument error
  return -EINVAL;
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
  printk("Invoking device_write(%p,%ld)\n", file, length);
  for( i = 0; i < length && i < BUF_LEN; ++i )
  {
     // get_user(the_message[i], &buffer[i]);

  }
 
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

  printk( "Registeration is successful. ");
  printk( "If you want to talk to the device driver,\n" );
  printk( "you have to create a device file:\n" );
  printk( "mknod /dev/%s c %d MINOR_NUM\n", DEVICE_RANGE_NAME, MAJOR_NUM );
  printk( "You can echo/cat to/from the device file.\n" );
  printk( "Dont forget to rm the device file and "
          "rmmod when you're done\n" );

  return 0;
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