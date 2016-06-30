#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
 
#define BUFFER_LENGTH 256               ///< The buffer length (crude but fine)
static char receive[BUFFER_LENGTH];     ///< The receive buffer from the LKM
 
	int checkInput(char *s)
	{
		if(strlen(s) == 1  &&    ( ( s[0] == '1' ) || ( s[0] == '0' ) ) )
			{
				return 1;
			}
		return 0;
		
	}
int main(){

   int ret, fd;
   char stringToSend[BUFFER_LENGTH];
   printf("Starting device test code example...\n");
   fd = open("/dev/ebbchar", O_RDWR);             // Open the device with read/write access
   if (fd < 0){
      perror("Failed to open the device...");
      return errno;
   }

   printf("Type in a short string to send to the kernel module:\n");
   scanf("%[^\n]%*c", stringToSend);                // Read in a string (with spaces)
	
	if(!checkInput(stringToSend))
		{
			 printf("input must be 1 or 0 \n");
			 printf("terminating the operation \n");
			return 0;
		}

   printf("Writing [%s]  to the device .\n", stringToSend);
   ret = write(fd, stringToSend, strlen(stringToSend)); // Send the string to the LKM
    
	if(ret == 0)
		{
		printf("stack is full\n");
			while(ret == 0){
			ret = write(fd, stringToSend, strlen(stringToSend));
			printf("waiting for stack to become empty\n");
			sleep(5);
			}
                }

   if(ret > 0)
	{
             printf("input is successfully written in stack\n");
	}

   if (ret < 0){
      perror("Failed to write the message to the device.");
      return errno;
   }

      

  
 

   printf("End of the program\n");
   return 0;
}
