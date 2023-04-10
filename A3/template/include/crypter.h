#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>


#ifndef CRYPTER_H_
#define CRYPTER_H_

typedef void* ADDR_PTR;
typedef int DEV_HANDLE;
typedef unsigned char KEY_COMP;

#define SET 1
#define UNSET 0
#define TRUE 1
#define FALSE 0
#define ERROR -1

typedef enum {INTERRUPT, DMA} config_t;

DEV_HANDLE create_handle();

void close_handle(DEV_HANDLE cdev);

int encrypt(DEV_HANDLE cdev, ADDR_PTR addr, uint64_t length, uint8_t isMapped);

int decrypt(DEV_HANDLE cdev, ADDR_PTR addr, uint64_t length, uint8_t isMapped);

int set_key(DEV_HANDLE cdev, KEY_COMP a, KEY_COMP b);

int set_config(DEV_HANDLE cdev, config_t type, uint8_t value);

ADDR_PTR map_card(DEV_HANDLE cdev, uint64_t size);

void unmap_card(DEV_HANDLE cdev, ADDR_PTR addr);

#endif
