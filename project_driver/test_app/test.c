#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h> // sysconf();
#include <sys/mman.h> // flag definitions: PROT_READ, ...
#include <string.h> // strcmp(), strncpy()

#define DEVICE "/dev/dummy_two"
#define BUF_SIZE 4096

int main(){
  int fd;
  char option[3] = "xx", buf[BUF_SIZE];
  char* mem;
  long page_size = sysconf(_SC_PAGE_SIZE);
  
  fd=open(DEVICE,O_RDWR); //open for reading and writing
  if(fd==-1){
    printf("file %s either does not exist or has been locked by another process\n",DEVICE);
    close(fd);
    exit(fd);
  }
  printf("Options:\n");
  printf("rk: read through kernel\n");
  printf("wk: write through kernel\n");
  printf("m: mmap\n");
  printf("rd: read directly\n");
  printf("wd: write directly\n");
  printf("e: exit\n\n");
  
  while(strcmp(option,"e")){
    printf(">>Enter option: ");
    do{
      scanf("%s",option);
      //fgets(option,3,stdin);
      printf(">%s<\n",option);
    }while(strcmp(option,"\n"));
    printf(">%s<\n",option);
    if(!strcmp(option,"wk")){
      printf("write through kernel: ");
      //fgets(buf,3,sizeof(buf));
      scanf("data: %[^\n]",buf);
      write(fd,buf,sizeof(buf));
    }
    else if(!strcmp(option,"rk")){
      read(fd,buf,sizeof(buf));
      printf("read through kernel:  %s\n",buf);
    }
    else if(!strcmp(option,"m")){
      printf("mmap\n");
      mem = (char*) mmap(NULL, page_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    }
    else if(!strcmp(option,"wd")){
      printf("write directly\n");
      scanf("data: %[^\n]",buf);
      fgets(buf,sizeof(buf),stdin);
      strncpy(mem,buf,sizeof(buf));
    }
    else if(!strcmp(option,"rd")){
      printf("read directly\n");
      printf("%s",mem);
    }
    else if(!strcmp(option,"e")){
      printf("exit\n");
      close(fd);
    }
    else{
      printf("incorrect option\n");
    }
  }
  return 0;
}
 
