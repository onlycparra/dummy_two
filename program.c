#include <stdio.h>
#include <stdlib.h>

int main(){
  FILE *fp;

  //open
  printf("Trying to open...\n");
  fp=fopen("/dev/chardev", "a+");
  if (fp == NULL){
    printf("did you run the program as superuser?\n");
    exit(EXIT_FAILURE);
  }

  
  //read
  printf("Trying to read...\n");
  char line[30];
  int i=0;
  do{
    fscanf(fp,"%c",(line+i));
    i += 1;
  }while(i<30);
  line[29]='\0';
  printf("Data read: %s\n", line);

  
  //write
  printf("Trying to write...\n");
  fprintf(fp, "Let me write this\n");

  
  //close
  printf("Trying to close...\n");
  fclose(fp);

  
  printf("Done\n");
  return 0;
}
