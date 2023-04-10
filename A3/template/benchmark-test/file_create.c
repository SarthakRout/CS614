#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
// Default file size in MB
int default_file_size = 10;
char seed_data[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ12345678ARUN\n";

void mount_shm()
{
        int result = system("sudo mount -o remount /dev/shm");
        if(result < 0)
        {
                printf("Error while mounting the shm \n");
                exit(0);
        }
        printf(" /dev/shm mounted successfully \n");
        sleep(1);
}

void create_file(int file_size)
{
        long i;
        FILE * fptr;
        
        fptr = fopen("/dev/shm/in_memory.txt", "w");
        long line =0;
        for (i = 0; i < ((file_size)*1024*1024L); i++) {
            if(i % 4096 == 4095)
            {
                    line++;
                    fputc('\n',fptr);
                    continue;
            }
            int cur_ind = i % 64;
            fputc(seed_data[cur_ind], fptr);
        }
        fclose(fptr);
}

int main(int argc, char *argv[]) {
        // File size in MB
        int file_size = default_file_size;       
        if(argc  >= 2)
        {
                file_size = atoi(argv[1]);
                printf("Creating In Memory file of size %d MB \n", file_size);
        }
        mount_shm();
        create_file(file_size);
        printf("File created successfully \n");
        return 0;       
}
