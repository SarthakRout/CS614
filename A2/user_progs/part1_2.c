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
   char *ptr, *ptr1, *ptr2;
   unsigned long new_addr = 0;

   int fd = open("/dev/cs614",O_RDWR);
   if(fd < 0){
       perror("open");
       exit(-1);
   }

   ptr = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
   ptr1 = mmap(NULL, 4096, PROT_READ|PROT_WRITE,MAP_ANONYMOUS|MAP_PRIVATE,-1, 0);
   ptr2 = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
   printf("ptr:%p, ptr1:%p, ptr2:%p\n",ptr, ptr1, ptr2);
   if(ptr == MAP_FAILED || ptr1 == MAP_FAILED || ptr2 == MAP_FAILED){
        perror("mmap");
        exit(-1);
   }
   addr_details.vma_addr = (unsigned long)ptr1;
   ptr1[0] = 'A'; 
   printf("going to move ptr1 [%p] to available hole\n",ptr1);
   if(ioctl(fd, IOCTL_MVE_VMA, &addr_details) < 0){
       printf("VMA move failed\n");
       exit(-1);
   }
   //new vma address passed from kernel module
   new_addr = addr_details.to_addr;
   printf("new address:%lx\n",new_addr);

   //After successful VMA move below access should succeed
   char * temp = (char*)new_addr;
   printf("value:%c\n",temp[0]);
 
   close(fd);
   munmap((void *)ptr, 4096);
   munmap((void *)ptr1, 4096);
   munmap((void *)ptr2, 4096);
   return 0;
}
