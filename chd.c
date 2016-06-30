#include <linux/init.h>           // Macros used to mark up functions e.g. __init __exit
#include <linux/module.h>         // Core header for loading LKMs into the kernel
#include <linux/device.h>         // Header to support the kernel Driver Model
#include <linux/kernel.h>         // Contains types, macros, functions for the kernel
#include <linux/fs.h>             // Header for the Linux file system support
#include <asm/uaccess.h>          // Required for the copy to user function
#include <linux/mutex.h>
#define  DEVICE_NAME "ebbchar"    ///< The device will appear at /dev/ebbchar using this value
#define  CLASS_NAME  "ebb"        ///< The device class -- this is a character device driver
 
MODULE_LICENSE("GPL");            ///< The license type -- this affects available functionality
MODULE_AUTHOR("Milad Qasemi");    ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("A simple Linux char driver");  ///< The description -- see modinfo
MODULE_VERSION("0.1");            ///< A version number to inform users
 
static int    majorNumber;                  ///< Stores the device number -- determined automatically
static char   message[256] = {0};           ///< Memory for the string that is passed from userspace
static short  size_of_message;              ///< Used to remember the size of the string stored
static int    numberOpens = 0;              ///< Counts the number of times the device is opened
static struct class*  ebbcharClass  = NULL; ///< The device-driver class struct pointer
static struct device* ebbcharDevice = NULL; ///< The device-driver device struct pointer
static int InputStack[4]={10,10,10,10};
static int Top=-1;
static int input;
static DEFINE_MUTEX(char_mutex);


	int i=0;
 	int w1=0,w2=0;
	int bias=0;
	int alpha=1;
	int threshold=0;
	int inputResult[4][3] = {  
	   {1, 1, 1} ,   /*  initializers for row indexed by 0 */
	   {1, -1, -1} ,   /*  initializers for row indexed by 1 */
	   {-1, 1, -1} ,  /*  initializers for row indexed by 2 */
	   {-1, -1, -1}
	};
 
// The prototype functions for the character driver -- must come before the struct definition
static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);
 
/** @brief Devices are represented as file structure in the kernel. The file_operations structure from
*  /linux/fs.h lists the callback functions that you wish to associated with your file operations
*  using a C99 syntax structure. char devices usually implement open, read, write and release calls
*/
static struct file_operations fops =
{
   .open = dev_open,
   .read = dev_read,
   .write = dev_write,
   .release = dev_release,
};
 
/** @brief The LKM initialization function
*  The static keyword restricts the visibility of the function to within this C file. The __init
*  macro means that for a built-in driver (not a LKM) the function is only used at initialization
*  time and that it can be discarded and its memory freed up after that point.
*  @return returns 0 if successful
*/


	  int calculateYin(int x1,int w1,int x2,int w2,int b)
	{
		return (x1*w1)+(x2*w2)+b;
	} 

  int fYin(int yIn,int threshold)
	{
		if(yIn>threshold){return 1;}
		else if(yIn<(-threshold)){return -1;}
		else {return 0;}
	}


  int findingBias(int Bold,int t)
	{
		return Bold+(alpha*t);
	}

  int reCalculateWeight(int Wold,int x,int t)
	{
		return Wold+(alpha*x*t);
	}

  
  void findingWeights(void)
	{
         i=0;
	   while(i < 4){
		while(inputResult[i][2] != fYin(calculateYin(inputResult[i][0],w1,inputResult[i][1],w2,bias),threshold))
			{ 
			 
			   w1=reCalculateWeight(w1,inputResult[i][0],inputResult[i][2]);
			   w2=reCalculateWeight(w2,inputResult[i][1],inputResult[i][2]);
			   bias = findingBias(bias,inputResult[i][2]);
	  		
				
			}
	
		i++;
		      }
	}



  int calculateResult(int x1,int x2)
	{
		 printk(KERN_ALERT "calculating the result");
		return fYin(calculateYin(x1,w1,x2,w2,bias),threshold);
	}


  int changeInput(int x)
	{
		if(x==-1){return 0;}
		else if(x==0){return -1;}
                else{return 1;}
	}
  

static void push(int input)
{
	 if(!mutex_trylock(&char_mutex)){    /// Try to acquire the mutex (i.e., put the lock on/down)
		                                  /// returns 1 if successful and 0 if there is contention
	      printk(KERN_ALERT "Device in use by another process");
	      return -EBUSY;
	   }
	
   printk(KERN_ALERT "stack top is %d \n",Top);


	if(Top==1)
	{
		printk(KERN_ALERT "stack is full\n");
	}
	else
	{
	  printk(KERN_ALERT "pushed %d to stack \n",input);
	  Top++;
	  InputStack[Top]= input;
          
	}

      mutex_unlock(&char_mutex);
}



static int charToint(const char * s)
	{
		return (s[0]-'0');
	}
static int IsStackFull(void)
	{
		if(Top == 0  || Top == -1)
		{
		 return 0;
		}
		else
		{
		return 1;
		}
	}


static void emptyStack(void)
	{
		Top=-1;
		InputStack[0]=10;
		InputStack[1]=10;
	}



static int __init ebbchar_init(void){
   printk(KERN_INFO "Initializing the EBBChar LKM\n");

	findingWeights(); //initializing weights


    mutex_init(&char_mutex);
   // Try to dynamically allocate a major number for the device -- more difficult but worth it
   majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
   if (majorNumber<0){
      printk(KERN_ALERT "failed to register a major number\n");
      return majorNumber;
   }
   printk(KERN_INFO "registered correctly with major number %d\n", majorNumber);
 
   // Register the device class
   ebbcharClass = class_create(THIS_MODULE, CLASS_NAME);
   if (IS_ERR(ebbcharClass)){                // Check for error and clean up if there is
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to register device class\n");
      return PTR_ERR(ebbcharClass);          // Correct way to return an error on a pointer
   }
   printk(KERN_INFO "device class registered correctly\n");
 
   // Register the device driver
   ebbcharDevice = device_create(ebbcharClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
   if (IS_ERR(ebbcharDevice)){               // Clean up if there is an error
      class_destroy(ebbcharClass);           // Repeated code but the alternative is goto statements
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to create the device\n");
      return PTR_ERR(ebbcharDevice);
   }
   printk(KERN_INFO "device class created correctly\n"); // Made it! device was initialized
   return 0;
}
 
/** @brief The LKM cleanup function
*  Similar to the initialization function, it is static. The __exit macro notifies that if this
*  code is used for a built-in driver (not a LKM) that this function is not required.
*/
static void __exit ebbchar_exit(void){
   device_destroy(ebbcharClass, MKDEV(majorNumber, 0));     // remove the device
   class_unregister(ebbcharClass);                          // unregister the device class
   class_destroy(ebbcharClass);                             // remove the device class
   unregister_chrdev(majorNumber, DEVICE_NAME);             // unregister the major number
   printk(KERN_INFO "Goodbye from the LKM!\n");
}
 
/** @brief The device open function that is called each time the device is opened
*  This will only increment the numberOpens counter in this case.
*  @param inodep A pointer to an inode object (defined in linux/fs.h)
*  @param filep A pointer to a file object (defined in linux/fs.h)
*/
static int dev_open(struct inode *inodep, struct file *filep){

   numberOpens++;
   printk(KERN_INFO "Device has been opened %d time(s)\n", numberOpens);
   return 0;
}
 
/** @brief This function is called whenever device is being read from user space i.e. data is
*  being sent from the device to the user. In this case is uses the copy_to_user() function to
*  send the buffer string to the user and captures any errors.
*  @param filep A pointer to a file object (defined in linux/fs.h)
*  @param buffer The pointer to the buffer to which this function writes the data
*  @param len The length of the b
*  @param offset The offset if required
*/
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
   int error_count = 0;

	printk(KERN_ALERT " first character in stack is %d \n", InputStack[0]);
	printk(KERN_ALERT "second character in stack is %d \n", InputStack[1]);

	if(IsStackFull())
	{
			
	 	message[0] =changeInput( calculateResult(changeInput(InputStack[0]) , changeInput(InputStack[1])) )+'0';
		emptyStack();
	}
        else
	{
		strcpy(message,"n");		
	        printk(KERN_ALERT "Not enough input\n");
	}

   // copy_to_user has the format ( * to, *from, size) and returns 0 on success


   error_count = copy_to_user(buffer, message, size_of_message);
	
	
   if (error_count==0){            // if true then have success
      printk(KERN_INFO "Sent %d characters to the user\n", size_of_message);
      return (size_of_message=0);  // clear the position to the start and return 0
   }
   else {
      printk(KERN_INFO "Failed to send %d characters to the user\n", error_count);
      return -EFAULT;              // Failed -- return a bad address message (i.e. -14)
   }
}
 
/** @brief This function is called whenever the device is being written to from user space i.e.
*  data is sent to the device from the user. The data is copied to the message[] array in this
*  LKM using the sprintf() function along with the length of the string.
*  @param filep A pointer to a file object
*  @param buffer The buffer to that contains the string to write to the device
*  @param len The length of the array of data that is being passed in the const char buffer
*  @param offset The offset if required
*/
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){

  // sprintf(message, "%s(%d letters)", buffer, len);   // appending received string with its length
   
   	size_of_message = strlen(buffer);                 // store the length of the stored message
   
		
	input = charToint(buffer);                   //change the char to int

        if(!(input == 1 || input == 0) || !(size_of_message == 1))
	{
          return -1;	
	}

        else if(IsStackFull())  
	{
	  return 0;
	}

	else
	{
          push(input);
	}
	
	
	
	printk(KERN_ALERT " first character in stack is %d \n", InputStack[0]);
	printk(KERN_ALERT "second character in stack is %d \n", InputStack[1]);
		
   return len;
}
 
/** @brief The device release function that is called whenever the device is closed/released by
*  the userspace program
*  @param inodep A pointer to an inode object (defined in linux/fs.h)
*  @param filep A pointer to a file object (defined in linux/fs.h)
*/
static int dev_release(struct inode *inodep, struct file *filep){
  
   printk(KERN_INFO "Device successfully closed\n");
   return 0;
}





/** @brief A module must use the module_init() module_exit() macros from linux/init.h, which
*  identify the initialization function at insertion time and the cleanup function (as
*  listed above)
*/
module_init(ebbchar_init);
module_exit(ebbchar_exit);
