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

#define SIZE (1<<23)

#define MAJOR_NUM 100

#define IOCTL_MVE_VMA_TO _IOR(MAJOR_NUM, 0, char*)
#define IOCTL_MVE_VMA _IOWR(MAJOR_NUM, 1, char*)
#define IOCTL_PROMOTE_VMA _IOR(MAJOR_NUM, 2, char*)
#define IOCTL_COMPACT_VMA _IOWR(MAJOR_NUM, 3, char*)

int main()
{
   char *ptr;
   char buf[16];
   int ct;
   int flag;
   pid_t pid = getpid();
   printf("pid:%u\n",pid);
   unsigned dummy = 1;
   int fd = open("/dev/cs614",O_RDWR);
   if(fd < 0){
       perror("open");
       exit(-1);
   }

   ptr = mmap(NULL, SIZE, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS|MAP_PRIVATE, 0, 0);
   if(ptr == MAP_FAILED){
        perror("mmap");
        exit(-1);
   }
   
   printf("PTR: %p\n", ptr);

   // creates the kernel thread
   if(ioctl(fd, IOCTL_PROMOTE_VMA, &dummy) < 0){
       printf("2MB promote thread creation failed\n");
       exit(-1);
   }

   //Access the Area
   int co = 0;
   for(int i=0;i<SIZE;i+=4096){
       ptr[i] = 'B';
       co++;
   }
   printf("counter: %d\n", co);
   
   int temp = 1;
   int fd_sysfs = open("/sys/kernel/kobject_cs614/promote",O_RDWR);
   if(fd_sysfs < 0){
       perror("open");
       exit(-1);
   }

   //indicates the kernel thread to promote 4KB pages to 2MB
   if(write(fd_sysfs, &temp, sizeof(temp)) < 0){
       perror("sysfs write");
       exit(-1);
   }

   //Access the Area
   int counter = 0;;
   for(int i=0;i<SIZE;i+=4096){
       char var = ptr[i];
       if(var == 'B'){
        counter++;
       }
   }
    printf("counter: %d\n", counter);

   //check kernel thread has finished work and set sysfs to 0
   if(read(fd_sysfs, &temp, sizeof(temp)) < 0){
          perror("sysfs read");
          exit(-1);
   }
   if(!temp){
       printf("pages are merged\n");
   }
   close(fd_sysfs);
   printf("Calling munmap\n");
   counter = 0;
   ct = 0;
   flag = 0;
   for(int i = 0; i<512; i++){
        if(ptr[i*4096] == 'B'){
            counter++;
            if((((unsigned long)ptr + i * 4096 ) % (1<<21)) == 0) {
                flag = 1;
            }
            if(!flag){
                ct++;
            }
        }
        else{
            printf("ptr[%d]: %d\n", i, ptr[i*4096]);
        }
   }
   printf("counter: %d, ct: %d\n", counter, ct);

   if(ioctl(fd, 6, &dummy) < 0){
       printf("page table walk failed\n");
       exit(-1);
   }
   close(fd);

   return 0;
}
