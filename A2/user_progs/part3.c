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
#define SIZE (1<<22)
#define NUM_PAGES 1024

#define IOCTL_MVE_VMA_TO _IOR(MAJOR_NUM, 0, char*)
#define IOCTL_MVE_VMA _IOWR(MAJOR_NUM, 1, char*)
#define IOCTL_PROMOTE_VMA _IOR(MAJOR_NUM, 2, char*)
#define IOCTL_COMPACT_VMA _IOWR(MAJOR_NUM, 3, char*)

struct address{
    unsigned long old;
    unsigned long new;
};

struct input{
    unsigned long addr;
    unsigned length;
    struct address* buff;
};

int main()
{
   char *ptr;
   struct input ip;
   struct address mapping[NUM_PAGES];
   unsigned long new_addr;

   int fd = open("/dev/cs614",O_RDWR);
   if(fd < 0){
       perror("open");
       exit(-1);
   }

   ptr = (char*)mmap(NULL, SIZE, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
   
   if(ptr == MAP_FAILED){
        perror("mmap");
        exit(-1);
   }
      
   for(int i=0; i<SIZE; i+=8192){
       memset(&ptr[i],'A',4096);
   }
   
   ip.addr = (unsigned long)ptr;
   ip.length = (SIZE>>12);
   ip.buff = mapping;
   
   printf("going to compact VMA ptr [%lx], length:%u, mapping:%lx\n",ip.addr,ip.length,(unsigned long)ip.buff);

   if(ioctl(fd, IOCTL_COMPACT_VMA, &ip) < 0){
       printf("VMA merge and move failed\n");
       exit(-1);
   }
   struct address * temp;
   //inspecting old address to new address mapping
   //after compaction following access should work
   //for all virtual pages in VMA range
   unsigned index = (&ptr[10]-ptr)>>12;
   unsigned offset = (unsigned long)&ptr[10]&0xfff;
   printf("index into mapping:%u,offset:%u\n",index,offset);
   temp = (struct address*)ip.buff+index;
   printf("old address:%lx, new address:%lx\n",temp->old, temp->new);
   printf("value at new addr:%c\n",*(char*)(temp->new+offset));

   close(fd);
   munmap((void *)ptr, SIZE);
   return 0;
}
