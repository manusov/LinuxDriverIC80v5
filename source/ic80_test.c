//--- Char PCI device PCI driver test for IC80v5 POST diagnostics card ---
//--- Created by IC Book Labs, thanks Derek Molloy example ---
//--- This is combined version (8+16 bit), see also 8 and 16-bit ---

#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>

#define BUFFER_LENGTH 256               // The buffer length limit
static char receive[BUFFER_LENGTH];     // The receive buffer from the LKM

int main()
{
   int ret, fd;                         // return code and file descriptor
   char stringToSend[BUFFER_LENGTH];

//---------- 8-bit test ----------
   
   fd = open("/dev/ic80char_8bit", O_RDWR);             // Open the device with read/write access
   if (fd < 0)
      {
      perror("Failed to open IC80v5 device (8-bit)");
      return errno;
      }
      
   printf("Test v0.11. IC80v5 (8-bit mode) device open OK.\nPlease type 2-char 8-bit hex code (00-FF) : ");
   scanf("%[^\n]%*c", stringToSend);                // Read in a string (with spaces)
   printf("Writing message to the device [%s].\n", stringToSend);
   ret = write(fd, stringToSend, strlen(stringToSend)); // Send the string to the LKM
   if (ret < 0)
      {
      perror("Failed to write the message to the device");  // no "." because ":" added by OS error writer
      return errno;
      }
   printf("Done, check 8-bit result at POST card.\n\n");

//---------- 16-bit test ----------
   
   fd = open("/dev/ic80char_16bit", O_RDWR);             // Open the device with read/write access
   if (fd < 0)
      {
      perror("Failed to open IC80v5 device (16-bit)");
      return errno;
      }
      
   printf("IC80v5 (16-bit mode) device open OK.\nPlease type 4-char 16-bit hex code (0000-FFFF) : ");
   scanf("%[^\n]%*c", stringToSend);                // Read in a string (with spaces)
   printf("Writing message to the device [%s].\n", stringToSend);
   ret = write(fd, stringToSend, strlen(stringToSend)); // Send the string to the LKM
   if (ret < 0)
      {
      perror("Failed to write the message to the device");  // no "." because ":" added by OS error writer
      return errno;
      }
   printf("Done, check 16-bit result at POST card.\n");


//--- Reserved for READ support ---
/*
   printf("Press ENTER to read back from the device...\n");
   getchar();

   printf("Reading from the device...\n");
   ret = read(fd, receive, BUFFER_LENGTH);        // Read the response from the LKM
   if (ret < 0){
      perror("Failed to read the message from the device.");
      return errno;
   }
   printf("The received message is: [%s]\n", receive);
*/
   
   
   return 0;
}
