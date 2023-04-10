#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <crypter.h>

int main()
{
  DEV_HANDLE cdev;
  char *msg = "Hello CS730!";
  char op_text[16];
  KEY_COMP a=30, b=17;
  uint64_t size = strlen(msg);
  strcpy(op_text, msg);
  cdev = create_handle();

  if(cdev == ERROR)
  {
    printf("Unable to create handle for device\n");
    exit(0);
  }

  if(set_key(cdev, a, b) == ERROR){
    printf("Unable to set key\n");
    exit(0);
  }

  printf("Original Text: %s\n", msg);

  encrypt(cdev, op_text, size, 0);
  printf("Encrypted Text: %s\n", op_text);

  decrypt(cdev, op_text, size, 0);
  printf("Decrypted Text: %s\n", op_text);

  close_handle(cdev);
  return 0;
}
