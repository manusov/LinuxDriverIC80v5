//--- Char device PCI device driver for IC80v5 POST diagnostics card ---
//--- Created by IC Book Labs, thanks Derek Molloy example ---
//--- This is combined version (8+16 bit), see also 8 and 16-bit ---

//--- Connect Linux kernel headers ---
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <asm/uaccess.h>  // This required for the copy to user function      

//--- Character device identifiers ---
#define  DEVICE_NAME_8BIT  "ic80char_8bit"    // The device will appear at /dev/ic80char_8bit using this value
#define  CLASS_NAME_8BIT   "ic80class_8bit"   // The device class -- this is a character device driver

#define  DEVICE_NAME_16BIT "ic80char_16bit"   // Same for 16-bit output support
#define  CLASS_NAME_16BIT  "ic80class_16bit"

//--- PCI Vendor and Device identifiers ---
#define PCI_VENDOR_ICBOOK 0xB00C   // IC Book PCI Vendor ID
#define PCI_DEVICE_IC80v5 0x001C   // IC80v5 PCI Device ID

//--- Static global variables for character device support ---
static int    majorNumber_8 = 0;      // Stores the device number, determined automatically
static int    numberOpens_8 = 0;      // Number of times the device is opened, counter
static char   message_8[256] = {0};   // Memory for the string that is passed from userspace
static short  size_of_message_8;      // Used to remember the size of the string stored

static int    majorNumber_16 = 0;     // Same for 16-bit output support
static int    numberOpens_16 = 0;
static char   message_16[256] = {0};
static short  size_of_message_16;

static struct class*  pointerDriverClass_8  = NULL;   // The device-driver class struct pointer
static struct device* pointerDriverDevice_8 = NULL;   // The device-driver device struct pointer

static struct class*  pointerDriverClass_16  = NULL;  // Same for 16-bit output support
static struct device* pointerDriverDevice_16 = NULL;

//--- Static global variables for IC80 PCI device support ---
static int ic80_port = 0x80;       // Port address
static int ic80_data = 0;          // Data variable for output

//--- Helper function for convert ASCII string to integer ---
//--- Note standard library methods has restrictions at kernel mode ---
static int ascii2int(const char *buffer, int len, int max)
{
   int i=0, j=0;               // Counter and Limit for ASCII->BINARY convertor
   int value=0;                // Temporary value for ASCII->BINARY convertor
   int accumulator = 0;        // data accumulator
// if (len>=max) { j=max; }    // set maximum MAX (2 or 4) chars hexadecimal digit
   if (len>max)
      {
      accumulator = -EINVAL;   // error if length > MAX (2 or 4 for 8 or 16-bit)
      }
//
   else { j=len; }
   for (i=0; i<j; i++)         // cycle for convert ASCII-BINARY
      {
      if (accumulator < 0) break;
      accumulator <<= 4;
      if ( (buffer[i]>=0x30) & (buffer[i]<=0x39) )     { value = buffer[i] & 0xF; }
      else if ( (buffer[i]>='A') & (buffer[i]<='F') )  { value = buffer[i]-'A'+10; }
      else if ( (buffer[i]>='a') & (buffer[i]<='f') )  { value = buffer[i]-'a'+10; }
      else
          {
		  value = 0;
	      accumulator = -EINVAL;    // error if not a hexadecimal digit
	      break;
	      }
      accumulator += value;
      }
   return accumulator;
}

//---------- 8-bit output mode support ---------------------------------

//--- Standard function: character device OPEN, for 8-bit ---
static int dev_open_8 (struct inode *inodep, struct file *filep)
{
   numberOpens_8++;
   printk(KERN_INFO "IC80v5 8-bit character device has been opened %d time(s)\n", numberOpens_8);
   return 0;
}

//--- Standard function: character device CLOSE, for 8-bit ---
static int dev_release_8 (struct inode *inodep, struct file *filep)
{
   printk(KERN_INFO "IC80v5 8-bit character device successfully closed\n");
   return 0;
}

//--- Standard function: character device READ, for 8-bit ---
static ssize_t dev_read_8 (struct file *filep, char *buffer, size_t len, loff_t *offset)
{
   int error_count = 0;
   // copy_to_user has the format ( * to, *from, size) and returns 0 on success
   error_count = copy_to_user(buffer, message_8, size_of_message_8);
   // errors handling
   if (error_count==0)
      {            // if true then have success
      printk(KERN_INFO "IC80v5 8-bit: sent %d characters to the user\n", size_of_message_8);
      return (size_of_message_8=0);  // clear the position to the start and return 0
      }
   else 
      {
      printk(KERN_INFO "IC80v5 8-bit: failed to send %d characters to the user\n", error_count);
      return -EFAULT;              // Failed -- return a bad address message (i.e. -14)
      }
}

//--- Standard function: character device WRITE, for 8-bit ---
static ssize_t dev_write_8 (struct file *filep, const char *buffer, size_t len, loff_t *offset)
{
   sprintf(message_8, "%s(%zu letters)", buffer, len);    // appending received string with its length
   size_of_message_8 = strlen(message_8);                 // store the length of the stored message
   printk(KERN_INFO "IC80v5 8-bit: received %zu characters from the user\n", len);
   // convert ASCII string to integer
   ic80_data = ascii2int(buffer, len, 2);
   // check for errors
   if (ic80_data < 0 ) return ic80_data;
   // masking before output
   ic80_data &= 0xFF; 
   // output to port 80h  
   outb(ic80_data, ic80_port);
   // write to kernel log
   printk( KERN_ALERT "IC80v5 8-bit: PORT = %08Xh , VALUE = %08Xh\n" , ic80_port , ic80_data );
   // return
   return len;
}

//---------- 16-bit output mode support --------------------------------

//--- Standard function: character device OPEN, for 16-bit ---
static int dev_open_16 (struct inode *inodep, struct file *filep)
{
   numberOpens_16++;
   printk(KERN_INFO "IC80v5 16-bit character device has been opened %d time(s)\n", numberOpens_8);
   return 0;
}

//--- Standard function: character device CLOSE, for 16-bit ---
static int dev_release_16 (struct inode *inodep, struct file *filep)
{
   printk(KERN_INFO "IC80v5 16-bit character device successfully closed\n");
   return 0;
}

//--- Standard function: character device READ, for 16-bit ---
static ssize_t dev_read_16 (struct file *filep, char *buffer, size_t len, loff_t *offset)
{
   int error_count = 0;
   // copy_to_user has the format ( * to, *from, size) and returns 0 on success
   error_count = copy_to_user(buffer, message_16, size_of_message_16);
   // errors handling
   if (error_count==0)
      {            // if true then have success
      printk(KERN_INFO "IC80v5 16-bit: sent %d characters to the user\n", size_of_message_16);
      return (size_of_message_16=0);  // clear the position to the start and return 0
      }
   else 
      {
      printk(KERN_INFO "IC80v5 16-bit: failed to send %d characters to the user\n", error_count);
      return -EFAULT;              // Failed -- return a bad address message (i.e. -14)
      }
}

//--- Standard function: character device WRITE, for 16-bit ---
static ssize_t dev_write_16 (struct file *filep, const char *buffer, size_t len, loff_t *offset)
{
   sprintf(message_16, "%s(%zu letters)", buffer, len);     // appending received string with its length
   size_of_message_16 = strlen(message_16);                 // store the length of the stored message
   printk(KERN_INFO "IC80v5 16-bit: received %zu characters from the user\n", len);
   // convert ASCII string to integer
   ic80_data = ascii2int(buffer, len, 4);
   // check for errors
   if (ic80_data < 0 ) return ic80_data;
   // masking before output
   ic80_data &= 0xFFFF; 
   // output to port 80h  
   outw(ic80_data, ic80_port);
   // write to kernel log
   printk( KERN_ALERT "IC80v5 16-bit: PORT = %08Xh , VALUE = %08Xh\n" , ic80_port , ic80_data );
   // return
   return len;
}

//---------- PCI support -----------------------------------------------

//--- Structure for Vendor:Device detection, must be nul-terminated ---
static struct pci_device_id ids[] = 
{
	{ PCI_DEVICE(PCI_VENDOR_ICBOOK, PCI_DEVICE_IC80v5), },
	{ 0, }  // List terminator = 0
};
MODULE_DEVICE_TABLE(pci, ids);

//--- This function called by Linux kernel when driver installed ---
//--- Probe function: device detection restrict after VID:DID match ---
static int probe(struct pci_dev *dev, const struct pci_device_id *id)
{
//--- PCI device support ---
	pci_enable_device(dev);
	return 0;  // Return 0 means device detected, no addition filtering
}

//--- This function called by Linux kernel when driver un-installed ---
//--- Reserved for device disable operation --- 
static void remove(struct pci_dev *dev)
{
	// This function reserved yet
}

//---------- Interface structures for Linux Kernel communications ------

//--- Interface structure for character device driver --- 
//--- This structure used by kernel as standard entry points list ---
//--- Declared functions called as user space communication callbacks ---
static struct file_operations fops_8 =
{
   .open = dev_open_8,
   .read = dev_read_8,
   .write = dev_write_8,
   .release = dev_release_8,
};

//--- Same for 16-bit output ---
static struct file_operations fops_16 =
{
   .open = dev_open_16,
   .read = dev_read_16,
   .write = dev_write_16,
   .release = dev_release_16,
};

//--- Interface structure for PCI device driver --- 
//--- This structure used by kernel as standard entry points list ---
static struct pci_driver pci_driver = 
{
	.name = "IC Book IC80v5 PCI diagnostics POST card",
	.id_table = ids,    // vid:did sequence for device detection
	.probe = probe,     // pointer to INSTALL function
	.remove = remove,   // pointer to UN-INSTALL function
};

//---------- Driver load and unload support callbacks ------------------

//--- This function called by Linux kernel when driver installed ---
//--- Standard entry point for driver registration ---
static int __init ic80_char_driver_init(void)
{

//--- Character device support, 8-bit ---
   printk(KERN_INFO "IC80v5 8-bit: initializing the char device LKM\n");
   // Try to dynamically allocate a major number for the device, this prevents numbers duplications
   majorNumber_8 = register_chrdev(0, DEVICE_NAME_8BIT, &fops_8);
   if (majorNumber_8<0)
   {
      printk(KERN_ALERT "IC80v5 8-bit: failed to register a major number\n");
      return majorNumber_8;
   }
   printk(KERN_INFO "IC80v5 8-bit: registered correctly with major number %d\n", majorNumber_8);
   // Register the device class
   pointerDriverClass_8 = class_create(THIS_MODULE, CLASS_NAME_8BIT);
   if (IS_ERR(pointerDriverClass_8))
   {                // Check for error and clean up if there is
      unregister_chrdev(majorNumber_8, DEVICE_NAME_8BIT);
      printk(KERN_ALERT "IC80v5 8-bit: failed to register device class\n");
      return PTR_ERR(pointerDriverClass_8);          // Correct way to return an error on a pointer
   }
   printk(KERN_INFO "IC80v5 8-bit: device class registered correctly\n");
   // Register the device driver
   pointerDriverDevice_8 = device_create(pointerDriverClass_8, NULL, MKDEV(majorNumber_8, 0), NULL, DEVICE_NAME_8BIT);
   if (IS_ERR(pointerDriverDevice_8))
   {               // Clean up if there is an error
      class_destroy(pointerDriverClass_8);           // Repeated code but the alternative is goto statements
      unregister_chrdev(majorNumber_8, DEVICE_NAME_8BIT);
      printk(KERN_ALERT "IC80v5 8-bit: failed to create the device\n");
      return PTR_ERR(pointerDriverDevice_8);
   }
   printk(KERN_INFO "IC80v5 8-bit: device class created correctly\n"); // If this point reached, initializing OK

//--- Character device support, 16-bit ---
   printk(KERN_INFO "IC80v5 16-bit: initializing the char device LKM\n");
   // Try to dynamically allocate a major number for the device, this prevents numbers duplications
   majorNumber_16 = register_chrdev(0, DEVICE_NAME_16BIT, &fops_16);
   if (majorNumber_16<0)
   {
      printk(KERN_ALERT "IC80v5 16-bit: failed to register a major number\n");
      return majorNumber_16;
   }
   printk(KERN_INFO "IC80v5 16-bit: registered correctly with major number %d\n", majorNumber_16);
   // Register the device class
   pointerDriverClass_16 = class_create(THIS_MODULE, CLASS_NAME_16BIT);
   if (IS_ERR(pointerDriverClass_16))
   {                // Check for error and clean up if there is
      unregister_chrdev(majorNumber_16, DEVICE_NAME_16BIT);
      printk(KERN_ALERT "IC80v5 16-bit: failed to register device class\n");
      return PTR_ERR(pointerDriverClass_16);          // Correct way to return an error on a pointer
   }
   printk(KERN_INFO "IC80v5 16-bit: device class registered correctly\n");
   // Register the device driver
   pointerDriverDevice_16 = device_create(pointerDriverClass_16, NULL, MKDEV(majorNumber_16, 0), NULL, DEVICE_NAME_16BIT);
   if (IS_ERR(pointerDriverDevice_16))
   {               // Clean up if there is an error
      class_destroy(pointerDriverClass_16);           // Repeated code but the alternative is goto statements
      unregister_chrdev(majorNumber_16, DEVICE_NAME_16BIT);
      printk(KERN_ALERT "IC80v5 16-bit: failed to create the device\n");
      return PTR_ERR(pointerDriverDevice_16);
   }
   printk(KERN_INFO "IC80v5 16-bit: device class created correctly\n"); // If this point reached, initializing OK

//--- PCI device support ---
	return pci_register_driver(&pci_driver);  // use list reference
}

//--- This function called by Linux kernel when driver un-installed ---
//--- Standard entry point for driver un-registration ---
static void __exit ic80_char_driver_exit(void)
{

//--- Character device support, 8-bit ---
   device_destroy(pointerDriverClass_8, MKDEV(majorNumber_8, 0));    // remove the device
   class_unregister(pointerDriverClass_8);                           // unregister the device class
   class_destroy(pointerDriverClass_8);                              // remove the device class
   unregister_chrdev(majorNumber_8, DEVICE_NAME_8BIT);               // unregister the major number
   printk(KERN_INFO "IC80v5 8-bit: LKM unloaded\n");

//--- Character device support, 16-bit ---
   device_destroy(pointerDriverClass_16, MKDEV(majorNumber_16, 0));  // remove the device
   class_unregister(pointerDriverClass_16);                          // unregister the device class
   class_destroy(pointerDriverClass_16);                             // remove the device class
   unregister_chrdev(majorNumber_16, DEVICE_NAME_16BIT);             // unregister the major number
   printk(KERN_INFO "IC80v5 16-bit: LKM unloaded\n");

//--- PCI device support ---
	pci_unregister_driver(&pci_driver);   // use list reference
}

//---------- Standard declarations -------------------------------------

module_init(ic80_char_driver_init);    // Driver start entry point
module_exit(ic80_char_driver_exit);    // Driver stop entry point

MODULE_LICENSE("GPL");             // License string
MODULE_AUTHOR("Created by IC Book Labs, thanks Derek Molloy example");
MODULE_DESCRIPTION("PCI IC80v5 POST Card char device driver");
MODULE_VERSION("0.11 - combined 8/16");
