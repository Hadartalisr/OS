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





//================== DEVICE FUNCTIONS ===========================
static int device_open( struct inode* inode,
                        struct file*  file )
{
  unsigned long flags; // for spinlock
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

  
  // allcate normal kernel ram 
  file_config* fileConfig = kmalloc(sizeof(file_config),GFP_KERNEL); 





  return SUCCESS;
}

//---------------------------------------------------------------
static int device_release( struct inode* inode,
                           struct file*  file)
{
  unsigned long flags; // for spinlock
  printk("Invoking device_release(%p,%p)\n", inode, file);

  // ready for our next caller
  spin_lock_irqsave(&device_info.lock, flags);
  --dev_open_flag;
  spin_unlock_irqrestore(&device_info.lock, flags);
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
  if((MSG_SLOT_CHANNEL |= ioctl_command_id ) || (ioctl_param <= 0) )
  {
    return -EINVAL;
  }

  file_co
  // Get the parameter given to ioctl by the process
  printk( "Invoking ioctl: setting MSG_SLOT_CHANNEL to %ld\n", ioctl_param );
  channel = ioctl_param;

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
    printk( KERN_ALERT "%s registraion failed for  %d\n",
                       DEVICE_FILE_NAME, MAJOR_NUM );
    return rc;
  }

  printk( "Registeration is successful. ");
  printk( "If you want to talk to the device driver,\n" );
  printk( "you have to create a device file:\n" );
  printk( "mknod /dev/%s c %d MINOR_NUM\n", DEVICE_FILE_NAME, MAJOR_NUM );
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
}



//---------------------------------------------------------------
module_init(simple_init);
module_exit(simple_cleanup);

//========================= END OF FILE =========================
