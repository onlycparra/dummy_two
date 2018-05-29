#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#define DEVICE "/dev/dummy_two"
#define BUF_SIZE 64

int main(){
  int fd;
  char ch='x', buf[BUF_SIZE];
  
  fd=open(DEVICE,O_RDWR); //open for reading and writing
  if(fd==-1){
    printf("file %s either does not exist or has been locked by another process\n",DEVICE);
    exit(fd);
  }
  
  while(ch!='e'){
    printf("\nr=read. w=write. e=exit. Enter option: ");
    do{
      ch=getchar();
    }while(ch=='\n');
    
    switch(ch){
    case 'w':
      printf("enter data: ");
      scanf(" %[^\n]",buf);
      write(fd,buf,sizeof(buf));
      break;
      
    case 'r':
      read(fd,buf,sizeof(buf));
      printf("device: %s\n",buf);
      break;
      
    case 'e':
      printf("bye\n");
      break;
      
    default:
      printf("incorrect option\n");
      break;
    }
  }
  return 0;
}
 
