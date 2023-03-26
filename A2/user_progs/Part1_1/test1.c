/**
 * CS614: Linux Kernel Programming
 * You are free to modify and replicate to create more test cases
 * to thoroughly test your implementation.
*/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/fcntl.h>
#include<signal.h>
#include<sys/ioctl.h>
#include<sys/mman.h>
#include <sys/unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#define MAJOR_NUM 100

#define IOCTL_MVE_VMA_TO _IOR(MAJOR_NUM, 0, char*)
#define IOCTL_MVE_VMA _IOWR(MAJOR_NUM, 1, char*)
#define IOCTL_PROMOTE_VMA _IOR(MAJOR_NUM, 2, char*)
#define IOCTL_COMPACT_VMA _IOWR(MAJOR_NUM, 3, char*)


struct address{
    unsigned long vma_addr;
    unsigned long to_addr;
};
struct address addr_details;
int main()
{
   char *ptr;
   unsigned long new_addr;

   int fd = open("/dev/cs614",O_RDWR);
   if(fd < 0){
       perror("open");
       exit(-1);
   }

   ptr = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0); 
   

   printf("ptr:%p\n",ptr);
   if(ptr == MAP_FAILED){
        perror("mmap");
        exit(-1);
   }
   addr_details.vma_addr = (unsigned long)ptr;
   addr_details.to_addr = (unsigned long)ptr+(1UL<<21);
   ptr[0] = 'A';
   printf("going to move ptr1 [%lx] to [%lx]\n",addr_details.vma_addr,addr_details.to_addr);
   if(ioctl(fd, IOCTL_MVE_VMA_TO, &addr_details) < 0){
       printf("VMA move failed\n");
       exit(-1);
   }
   else{
    printf("IOCTL Call completed\n");
   }
   //After successful move, below line should print correct value
   char * temp = (char*)addr_details.to_addr;
   printf("value:%c\n",temp[0]);

   close(fd);

   return 0;
}
