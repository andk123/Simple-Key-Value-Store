/*
 * key_value.c
 *
 *  Created on: Mar 4, 2017
 *      Author: ArnoldK
 */


#include "read_eval.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

#define DEBUG

//Name of the database (shared across the same session)
char *db_name;

//Size of each node in a pod (to define size of shared memory)
size_t length = sizeof(struct node);

//Hash function provided by the professor
int hash_func(char *word){
    int hashAddress = 5381;
    for (int counter = 0; word[counter]!='\0'; counter++){
        hashAddress = ((hashAddress << 5) + hashAddress) + word[counter];
    }
    return hashAddress % POD_NUMBER_LIMIT < 0 ? -hashAddress % POD_NUMBER_LIMIT : hashAddress % POD_NUMBER_LIMIT;
}


int  kv_store_create(const char *name){

	#ifdef DEBUG
	printf("%d\n", (int)length);
	#endif

	if (name == NULL){
		printf("Please enter a valid database name...\n");
		return -1;
	}


	//Create or Open shared memory slot
	int fd = shm_open(name, O_CREAT|O_RDWR, S_IRWXU);
	//If unsuccessful, prevent the caller
	if (fd == -1){
		printf("Could not create shared memory\n");
		return -1;
	}

	db_name = strdup(name);

	//Resize the shared memory which the max length of our key_value hashmap
	ftruncate(fd, length);
	close(fd);


	return 0;

}
int  kv_store_write(const char *key, const char *value){

	struct stat s;

	//Open shared memory slot
	int fd = shm_open(db_name, O_RDWR, S_IRWXU);
	//If unsuccessful, prevent the caller
	if (fd == -1){
		printf("Could not open shared memory\n");
		return -1;
	}

	if(fstat(fd, &s) == -1){
		printf("Could not determine length of the shared memory\n");
		return -1;
	}

	//Map this new memory address to a base address generated by the os
	char *mem_addr = mmap(NULL, s.st_size, PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);

	//memcpy(mem_addr, str, strlen(str));


	return 0;
}
char *kv_store_read(const char *key){

	struct stat s;

	//Open shared memory slot
	int fd = shm_open(db_name, O_RDWR, S_IRWXU);
	//If unsuccessful, prevent the caller
	if (fd == -1){
		printf("Could not open shared memory\n");
		//return -1;
	}

	if(fstat(fd, &s) == -1){
		printf("Could not determine length of the shared memory\n");
		//return -1;
	}

	//Map this new memory address to a base address generated by the os
	char *mem_addr = mmap(NULL, s.st_size, PROT_READ, MAP_SHARED, fd, 0);
	close(fd);

	//write(STDOUT_FILENO, mem_addr, s.st_size);


	return 0;
}

char **kv_store_read_all(const char *key){

	return 0;
}