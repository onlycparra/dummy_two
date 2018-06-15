#include <assert.h>
#include <stdio.h>
#include <drivers/dummy_drv.h>
#include <mm/core_mmu.h>	// define SMALL_PAGE_SIZE
#include <kernel/panic.h>
#include <kernel/tee_time.h>
#include <kernel/thread.h>
#include <platform_config.h>
#include <util.h>

struct dummy_data * read_dummy;

struct dummy_data* tee_dummy_open(void)
{
	int i;
	dummy_drv.size = 0;
	dummy_drv.length = 0;
	dummy_drv.read_len = 0;
	dummy_drv.data =  malloc(SMALL_PAGE_SIZE * NUM_PAGES);

	if(dummy_drv.data == NULL){
		return NULL;
	}
	dummy_drv.size = SMALL_PAGE_SIZE * NUM_PAGES;  	

	//READ
	read_dummy = malloc(sizeof(dummy_data));
	read_dummy->read = false;
	for(i=0; i< 5; i++){
		read_dummy->w[i] = 0;
	}
////	&dummy_data.read = false;
	return read_dummy; 
/*
        kbuf = malloc(len + 1);
        if (kbuf == NULL)
                return;

        if (tee_svc_copy_from_user(kbuf, buf, len) == TEE_SUCCESS) {
                kbuf[len] = '\0';
                trace_ext_puts(kbuf);
        }

        free(kbuf);
*/
}

int tee_dummy_close(void){
	int i = 0;
	if(dummy_drv.size > 0){
    		free(dummy_drv.data);
		dummy_drv.size = 0;
		dummy_drv.length = 0;
		dummy_drv.read_len = 0;
		dummy_drv.data = NULL;
		
		//dummy_read
		free(read_dummy);
		read_dummy->read = false;
		for(i=0; i< 5; i++){
			read_dummy->w[i] = 0;
		}
		read_dummy = NULL;
		
		return 0;
  	}else{
		return -1;
	}
}

/*struct dummy_data *tee_dummy_read(void){
	read_dummy->read =true;
	////&dummy_data.read = true;
	read_dummy->w1 = dummy_drv.data[0];  
	////&dummy_data.w1 = (unsigned long)dummy_drv.data[0];  
		
	return read_dummy;
	////return 0;
}*/
	
/*int tee_dummy_read(int smc_num, int idx){
	int start_idx; 	// 0, 5, 10, ...
	int i; 

	start_idx = smc_num * 6;

	if(start_idx > (int)dummy_drv.size)	
		return -1;	//try to access incorrect memory 
	for(i = 0; i < start_idx + idx; i++){
		read_dummy->w[i] = dummy_drv.data[i];
	}

	return 0;
}*/

/*
*	Func Name: tee_dummy_read
*
*	dummy_drv.read_len:: keep track of number of 'char' that has been read
*/
int tee_dummy_read(void){
	int start_idx;
	start_idx = dummy_drv.read_len/8;

	read_dummy->w[0] = dummy_drv.data[start_idx];
	read_dummy->w[1] = dummy_drv.data[start_idx+1];
	read_dummy->w[2] = dummy_drv.data[start_idx+2];

	//8 (char) * 3 (register) = 24
	dummy_drv.read_len = dummy_drv.read_len + 24;

	return 0;
}

/*
*  idx = amount of 'char' that is sent from NW dummy driver
*/
int tee_dummy_write(void){//int idx){	
	int i;
	int start_idx;

	start_idx = dummy_drv.length;

	//8 (char) * 4 (register) = 32
	for(i = 0; i < 32; i++){
		if(((char*)(read_dummy->w))[i] == 0){
			dummy_drv.length = dummy_drv.length + i + 1;
			return 0;
		}else{
			((char*)(dummy_drv.data))[start_idx + i] = ((char*)(read_dummy->w))[i];
		}
	}

	//dummy_drv.length = dummy_drv.length + idx;
	
	return 0;
}

int tee_dummy_reset(int cmd){
	if(cmd == 0){
		dummy_drv.read_len = 0;
	}else{
		dummy_drv.length = 0;
	}

	return 0;
}
