/*
 * key_value.c
 *
 *  Created on: Mar 4, 2017
 *      Author: ArnoldK
 */


#include "config.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>

//UNCOMMENT To debug
//#define DEBUG
//#define DEBUGFIFOWRITE
//#define DEBUGFIFOREAD

//Name of the database (shared across the same session) and 2 semaphores for Reader & Writers
char *db_name = NULL;
sem_t *sem_mutex ;
sem_t *sem_db ;

//Pointers to each read element in for the read cycle
struct index *read_array[READ_CYCLING_MAX];



//Always called this method before exiting (also called in kv_delete_db)
void free_to_exit(){
	free(db_name); free_read_cycle(); unlink_sem();
}

void free_read_cycle(){
	//Each time we access a new database, reset the count for the read cycling OR before exit application
	for (int i = 0; i < READ_CYCLING_MAX; i++){
		if (read_array[i] == NULL) {
			break;
		}
		free(read_array[i]);
	}
}

void unlink_sem(){
	//Unlink all semaphores used
	for (int i = 0; i < POD_NUMBER_LIMIT; i++){
		char int_to_string[6]; //since i is an int, it will have max 5 digits + null terminating char
		sprintf(int_to_string, "%d", i);

		char mutex[14] = "26mutex";
		char db[11] = "26db";
		strcat(mutex, int_to_string); strcat(db, int_to_string);
		sem_unlink(mutex); sem_unlink(db);
	}
}

//Initialize all necessary variables and arrays before create command
void initialize(){
	if(db_name!=NULL)free(db_name);
	free_read_cycle();
	for (int i = 0; i < READ_CYCLING_MAX; i++){
		read_array[i] = malloc(sizeof(struct node));
		read_array[i]->index = -1;
	}
}

//Hash function provided by the professor
int hash_func(char *word){
	int hashAddress = 5381;
	for (int counter = 0; word[counter]!='\0'; counter++){
		hashAddress = ((hashAddress << 5) + hashAddress) + word[counter];
	}
	return hashAddress % POD_NUMBER_LIMIT < 0 ? -hashAddress % POD_NUMBER_LIMIT : hashAddress % POD_NUMBER_LIMIT;
}

void moveMemory(char *mem_addr){

	//We know the pod is full so there is POD_SIZE number of key_value pairs in the current pod

	//Swap every element on the array except the first one (which will be deleted) to free the last spot
	//This operation is costly but we have to do so to the prevent the case where the key_value is not in the array already
	//and at the same time to respect the FIFO order.
	for(int j = 1; j < POD_SIZE; j++){
		memcpy(mem_addr + (j-1)*sizeof(struct node), mem_addr + j*sizeof(struct node), sizeof(struct node));
	}
}

int kv_delete_db(){
	if(db_name != NULL)shm_unlink(db_name);
	free_to_exit();
	return 0;
}


int  kv_store_create(const char *name){
	//Input validation
	if (name == NULL){
		db_name = NULL;
		printf("Please enter a valid database name...\n");
		return -1;
	}
	if (!strcmp(name, "\0")){
		db_name = NULL;
		printf("Name is empty, Please enter a valid database name...\n");
		return -1;
	}


	//Create or Open shared memory slot, If unsuccessful, prevent the caller
	int fd;	int new = 0;
	fd = shm_open(name, O_RDWR, S_IRWXU);
	if( fd < 0){
		fd = shm_open(name, O_CREAT | O_RDWR, S_IRWXU);
		new = 1;
	}
	if (fd == -1){
		printf("Could not create shared memory\n");
		return -1;
	}

	//Copy the name of the database to a global variable to save it for the whole session
	db_name = strdup(name);

	/* Size of the shared memory =
	 * Size of node (a pod contains X number of nodes) * Number of nodes in a pod * Number of pods
	 * + Size of the array index for the pods + Size of the array for the rc counter of the semaphores
	 */
	size_t length = sizeof(struct node) * POD_SIZE * POD_NUMBER_LIMIT + sizeof(int) * POD_NUMBER_LIMIT + sizeof(int) * POD_NUMBER_LIMIT;

	//Resize the shared memory which the max length of our key_value hashmap
	ftruncate(fd, length);

	char *mem_addr = mmap(NULL, length, PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);


	//If the store is newly created, initiate the index pod array padded with 0's at the end of the shared memory and the rc array for the semaphores
	if(new){
		int *pod_index = calloc(POD_NUMBER_LIMIT, sizeof(int));
		int *rc_index = calloc(POD_NUMBER_LIMIT, sizeof(int));
		memcpy(mem_addr + sizeof(struct node) * POD_SIZE * POD_NUMBER_LIMIT, pod_index, sizeof(int) * POD_NUMBER_LIMIT);
		memcpy(mem_addr + sizeof(struct node) * POD_SIZE * POD_NUMBER_LIMIT + sizeof(int) * POD_NUMBER_LIMIT, rc_index, sizeof(int) * POD_NUMBER_LIMIT);
		free(pod_index); free(rc_index);
	}

	munmap(mem_addr, length);
	return 0;
}


int  kv_store_write(char *key, char *value){

#ifdef DEBUG
	printf("write\n");
	printf("key: %s\n", key);
	//	printf("value: %s\n", value);
#endif

	//Input validation
	if (key == NULL || value == NULL || !strcmp(key, "\0") || !strcmp(value, "\0")){
		printf("Please enter a valid key and value...\n");
		return -1;
	}
	if (db_name == NULL){
		printf("Please create a valid db...\n");
		return -1;
	}

	//Open shared memory slot
	int fd = shm_open(db_name, O_RDWR, S_IRWXU);
	//If unsuccessful, prevent the caller
	if (fd == -1){
		printf("Could not open shared memory\n");
		return -1;
	}

	//Get size of the shared memory
	struct stat s;
	if(fstat(fd, &s) == -1){
		printf("Could not determine length of the shared memory\n");
		return -1;
	}

	//Map this new memory address to a base address generated by the os
	char *mem_addr = mmap(NULL, s.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);


	//Copy the key and value to the node and truncate if necessary
	struct node *new_node = malloc(sizeof(struct node));
	strncpy(new_node->key, key, sizeof(new_node->key));
	strncpy(new_node->value,value, sizeof(new_node->value));
	new_node->key[KEY_MAX_LENGTH-1] = '\0';
	new_node->value[VALUE_MAX_LENGTH-1] = '\0';

	//Show new result if truncated
	if (strlen(key) > KEY_MAX_LENGTH-1) printf("Key too big, Truncated key is: %s\n", new_node->key);
	if (strlen(value) > VALUE_MAX_LENGTH-1) printf("Value too big, it will be truncated\n");

	//Find the hashkey of the key to write
	int hashkey = hash_func(new_node->key);

	/**********************************************Critical Code*********************************************************/

	//Create the name of the semaphores using the hashkey value so each pod has a different semaphore
	char hashkeyString[6]; //since hashkey is an int, it will have max 5 digits + null terminating char
	sprintf(hashkeyString, "%d", hashkey);

	char db[11] = "26db";
	strcat(db, hashkeyString);

	//The Write semaphore. Getting exclusive access to write
	sem_db = sem_open(db, O_CREAT, 0644, 1);
	sem_wait(sem_db);

	/**************Semaphore Wait*****************/

	//Find the size of the current pod to know where to write the new element
	int pod_index[POD_NUMBER_LIMIT];
	memcpy(pod_index, mem_addr + sizeof(struct node) * POD_SIZE * POD_NUMBER_LIMIT, sizeof(pod_index));

#ifdef DEBUGFIFOWRITE
	//printf("******************************************************************************************************\n");
	for(int i =0; i<POD_NUMBER_LIMIT; i++){
		if (i!=hashkey){
			printf("Pod size: %d\n", pod_index[i]);
		}
		else{
			printf("Pod size: %d +1\n", pod_index[i]);
		}
	}
	//printf("******************************************************************************************************\n");
#endif

	//If the pod is full delete the first key_value swap all the others -1 position
	if (pod_index[hashkey] == POD_SIZE){
		moveMemory(mem_addr + hashkey*POD_SIZE*sizeof(struct node));
		pod_index[hashkey]--;
	}

	//Put the element in the shared memory using its hashmap key and the index of its new position
	//(add it to the memory address of the shared memory)
	memcpy(mem_addr + hashkey*POD_SIZE*sizeof(struct node) + pod_index[hashkey]*sizeof(struct node), new_node, sizeof(struct node));

	//Update the index array of the pods (to track the number of elements into each pod) and copy array to memory
	pod_index[hashkey]++;
	memcpy(mem_addr + sizeof(struct node) * POD_SIZE * POD_NUMBER_LIMIT, pod_index, sizeof(pod_index));

#ifdef DEBUGFIFOWRITE

	for(int k = 0; k<POD_NUMBER_LIMIT; k++){
		//for(int j = 0; j < pod_index[hashkey]; j++){
		for(int j = 0; j < POD_SIZE; j++){
			memcpy(new_node, mem_addr + k*POD_SIZE*sizeof(struct node) + j*sizeof(struct node), sizeof(struct node));
			printf("Inside ALL Array%d: %s\n", k+1, new_node->key);
			if (new_node == NULL){
				continue;
			}
			//printf("Inside ALL Array%d: %s\n", k+1, new_node->key);
		}
	}
	printf("\n");
#endif

	sem_post(sem_db);
	/**************Semaphore Signal****************/

	/**********************************************Critical Code End*****************************************************/

	sem_close(sem_db);
	munmap(mem_addr, s.st_size);
	free(new_node);
	return 0;
}


char *kv_store_read(const char *key){
#ifdef DEBUG
	printf("read\n");
	printf("key: %s\n", key);
#endif
	//Input validation
	if (key == NULL || !strcmp(key, "\0")){
		printf("Please enter a valid key...\n");
		return 0;
	}
	if (db_name == NULL){
		printf("Please create a valid db...\n");
		return 0;
	}

	//Open shared memory slot and map it
	int fd = shm_open(db_name, O_RDWR, S_IRWXU);
	if (fd == -1){
		printf("Could not open shared memory\n");
		return 0;
	}

	struct stat s;
	if(fstat(fd, &s) == -1){
		printf("Could not determine length of the shared memory\n");
		return 0;
	}

	char *mem_addr = mmap(NULL, s.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);

	//Reproduce key and make sure it is right size, Show a new result if truncated
	char normalizedKey[KEY_MAX_LENGTH];
	strncpy(normalizedKey, key, KEY_MAX_LENGTH);
	normalizedKey[KEY_MAX_LENGTH-1] = '\0';
	if (strlen(key) > KEY_MAX_LENGTH-1) printf("Key too big, Truncated key is: %s\n", normalizedKey);

	//Found the hash number from the key
	int hashkey = hash_func(normalizedKey);

	/**********************************************Critical Code*********************************************************/

	//Create the name of the semaphores using the hashkey value so each pod has a different semaphore
	char hashkeyString[6]; //since hashkey is an int, it will have max 5 digits + null terminating char
	sprintf(hashkeyString, "%d", hashkey);

	char mutex[14] = "26mutex";
	char db[11] = "26db";
	strcat(mutex, hashkeyString);
	strcat(db, hashkeyString);

	//Reader semaphores. Getting exclusive access to rc
	sem_mutex = sem_open(mutex, O_CREAT, 0644, 1);
	sem_db = sem_open(db, O_CREAT, 0644, 1);
	int rc_index[POD_NUMBER_LIMIT];

	sem_wait(sem_mutex);
	/***********Semaphore - Update RC**************/

	//Increment the rc and put it back in memory
	memcpy(rc_index, mem_addr + sizeof(struct node) * POD_SIZE * POD_NUMBER_LIMIT + sizeof(int) * POD_NUMBER_LIMIT, sizeof(rc_index));
	rc_index[hashkey]++;
	memcpy(mem_addr + sizeof(struct node) * POD_SIZE * POD_NUMBER_LIMIT + sizeof(int) * POD_NUMBER_LIMIT, rc_index, sizeof(int) * POD_NUMBER_LIMIT);

	// Release access to other readers. If this was the first rc block the access to the writer
	if (rc_index[hashkey] == 1) sem_wait(sem_db);
	sem_post(sem_mutex);
	/****************End semaphore***************/


	//Retrieve the pod index to limit the number of searches in the end for large number of elements in a pod
	int pod_index[POD_NUMBER_LIMIT];
	memcpy(pod_index, mem_addr + sizeof(struct node) * POD_SIZE * POD_NUMBER_LIMIT, sizeof(pod_index));

	//If the size the pod is 0 then the key does not exist (Update semaphores before that)
	if(pod_index[hashkey] == 0){

		/***********Semaphore - Update RC**************/
		sem_wait(sem_mutex);

		//Decrement the rc and put it back in memory
		memcpy(rc_index, mem_addr + sizeof(struct node) * POD_SIZE * POD_NUMBER_LIMIT + sizeof(int) * POD_NUMBER_LIMIT, sizeof(rc_index));
		rc_index[hashkey]--;
		memcpy(mem_addr + sizeof(struct node) * POD_SIZE * POD_NUMBER_LIMIT + sizeof(int) * POD_NUMBER_LIMIT, rc_index, sizeof(int) * POD_NUMBER_LIMIT);

		// Release access to other readers. If this was the last rc release the the access to the writer
		if (rc_index[hashkey] == 0) sem_post(sem_db);
		sem_post(sem_mutex);
		/****************End semaphore***************/

		sem_close(sem_mutex); sem_close(sem_db);
		return 0;
	}


	//Find the index of the correct value to read (FIFO). If no value from that key was yet read, take the first one encountered
	//When we hit a -1, all the values after that are uninitialized so don't need to traverse the whole array
	int i;
	for (i = 0; i < READ_CYCLING_MAX; i++){
		if (read_array[i]->index == -1) {
			strcpy(read_array[i]->key, normalizedKey);
			read_array[i]->index = 0;
			break;
		}
		if(!strcmp(read_array[i]->key, normalizedKey)){
			read_array[i]->index++;
			break;
		}
	}

	//Count each corresponding key until the count is equal to the index we previously created
	struct node *new_node = malloc(sizeof(struct node));

#ifdef DEBUGFIFOREAD
	printf("Array index: %d\n", read_array[i]->index);

	for(int k = 0; k<POD_NUMBER_LIMIT; k++){
		for(int j = 0; j < pod_index[hashkey]; j++){
			memcpy(new_node, mem_addr + k*POD_SIZE*sizeof(struct node) + j*sizeof(struct node), sizeof(struct node));
			if (new_node == NULL){
				continue;
			}
			printf("Inside ALL Array%d: %s\n", k+1, new_node->key);
		}
	}
	printf("\n");
#endif

	int count = 0;
	for(int j = 0; j < pod_index[hashkey]; j++){
		memcpy(new_node, mem_addr + hashkey*POD_SIZE*sizeof(struct node) + j*sizeof(struct node), sizeof(struct node));
		if(!strcmp(new_node->key, normalizedKey)){
			if(count == read_array[i]->index) break;
			else count++;
		}

		//If we got to then end of the array, therefore we have to go back to beginning and pick the first read value
		if(j == pod_index[hashkey]-1){
			//Avoid infinite loop if value not found
			if(count == 0) break;
			j = -1;
			count = 0;
			read_array[i]->index = 0;
		}
	}

	/***********Semaphore - Update RC**************/
	sem_wait(sem_mutex);

	//Decrement the rc and put it back in memory
	memcpy(rc_index, mem_addr + sizeof(struct node) * POD_SIZE * POD_NUMBER_LIMIT + sizeof(int) * POD_NUMBER_LIMIT, sizeof(rc_index));
	rc_index[hashkey]--;
	memcpy(mem_addr + sizeof(struct node) * POD_SIZE * POD_NUMBER_LIMIT + sizeof(int) * POD_NUMBER_LIMIT, rc_index, sizeof(int) * POD_NUMBER_LIMIT);

	// Release access to other readers. If this was the last rc release the the access to the writer
	if (rc_index[hashkey] == 0) sem_post(sem_db);
	sem_post(sem_mutex);
	/****************End semaphore***************/

	/**********************************************Critical Code End*****************************************************/

	sem_close(sem_mutex); sem_close(sem_db);
	munmap(mem_addr, s.st_size);

	//If we did not find the right key, return nothing found
	if(strcmp(new_node->key, normalizedKey)){
		free(new_node);
		return 0;
	}

	//Copy the value to a new char array to free that variable in the caller
	char *value = strdup(new_node->value);
	free(new_node);

#ifdef DEBUG
	printf("value: %s\n\n", value);
#endif

	return value;
}


char **kv_store_read_all(const char *key){

	//Input validation
	if (key == NULL || !strcmp(key, "\0")){
		printf("Please enter a valid key...\n");
		return 0;
	}
	if (db_name == NULL){
		printf("Please create a valid db...\n");
		return 0;
	}

	//Open shared memory slot
	int fd = shm_open(db_name, O_RDWR, S_IRWXU);
	//If unsuccessful, prevent the caller
	if (fd == -1){
		printf("Could not open shared memory\n");
		return 0;
	}

	//Get size of the shared memory
	struct stat s;
	if(fstat(fd, &s) == -1){
		printf("Could not determine length of the shared memory\n");
		return 0;
	}

	//Map this new memory address to a base address generated by the os
	char *mem_addr = mmap(NULL, s.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	close(fd);

	//Reproduce key and make sure it is right size, and show new result if truncated
	char normalizedKey[KEY_MAX_LENGTH];
	strncpy(normalizedKey, key, KEY_MAX_LENGTH);
	normalizedKey[KEY_MAX_LENGTH-1] = '\0';
	if (strlen(key) > KEY_MAX_LENGTH-1) printf("Key too big, Truncated key is: %s\n", normalizedKey);

	//Found the hash number from the key
	int hashkey = hash_func(normalizedKey);


	/**********************************************Critical Code*********************************************************/

	//Create the name of the semaphores using the hashkey value so each pod has a different semaphore
	char hashkeyString[6]; //since hashkey is an int, it will have max 5 digits + null terminating char
	sprintf(hashkeyString, "%d", hashkey);

	char mutex[14] = "26mutex";
	char db[11] = "26db";
	strcat(mutex, hashkeyString); strcat(db, hashkeyString);

	//Reader semaphores. Getting exclusive access to rc
	sem_mutex = sem_open(mutex, O_CREAT, 0644, 1);
	sem_db = sem_open(db, O_CREAT, 0644, 1);
	int rc_index[POD_NUMBER_LIMIT];

	sem_wait(sem_mutex);
	/***********Semaphore - Update RC**************/

	//Increment the rc and put it back in memory
	memcpy(rc_index, mem_addr + sizeof(struct node) * POD_SIZE * POD_NUMBER_LIMIT + sizeof(int) * POD_NUMBER_LIMIT, sizeof(rc_index));
	rc_index[hashkey]++;
	memcpy(mem_addr + sizeof(struct node) * POD_SIZE * POD_NUMBER_LIMIT + sizeof(int) * POD_NUMBER_LIMIT, rc_index, sizeof(int) * POD_NUMBER_LIMIT);

	// Release access to other readers. If this was the first rc block the the access to the writer
	if (rc_index[hashkey] == 1) sem_wait(sem_db);
	sem_post(sem_mutex);
	/****************End semaphore***************/


	//Retrieve the pod index to limit the number of searches in the end for large number of elements in a pod
	int pod_index[POD_NUMBER_LIMIT];
	memcpy(pod_index, mem_addr + sizeof(struct node) * POD_SIZE * POD_NUMBER_LIMIT, sizeof(pod_index));


	//If the size the pod is 0 then the key does not exist, exit without success (update semaphores before that)
	if(pod_index[hashkey] == 0) {

		/***********Semaphore - Update RC**************/
		sem_wait(sem_mutex);

		//Decrement the rc and put it back in memory
		memcpy(rc_index, mem_addr + sizeof(struct node) * POD_SIZE * POD_NUMBER_LIMIT + sizeof(int) * POD_NUMBER_LIMIT, sizeof(rc_index));
		rc_index[hashkey]--;
		memcpy(mem_addr + sizeof(struct node) * POD_SIZE * POD_NUMBER_LIMIT + sizeof(int) * POD_NUMBER_LIMIT, rc_index, sizeof(int) * POD_NUMBER_LIMIT);

		// Release access to other readers. If this was the last rc release the the access to the writer
		if (rc_index[hashkey] == 0) sem_post(sem_db);
		sem_post(sem_mutex);
		/****************End semaphore***************/

		sem_close(sem_mutex); sem_close(sem_db);
		return 0;
	}

	//Copy each value corresponding to the key in the read_all string array
	struct node *new_node = malloc(sizeof(struct node));
	char **read_all = malloc((POD_SIZE+1)*sizeof(char[VALUE_MAX_LENGTH]));

	int count = 0;
	for(int j = 0; j < pod_index[hashkey]; j++){
		memcpy(new_node, mem_addr + hashkey*POD_SIZE*sizeof(struct node) + j*sizeof(struct node), sizeof(struct node));
		if(!strcmp(new_node->key, normalizedKey)){
			read_all[count] = strdup(new_node->value);
			count++;
		}
	}
	//Set the last element to NULL to know when the array finishes
	read_all[count] = NULL;

	/***********Semaphore - Update RC**************/
	sem_wait(sem_mutex);

	//Decrement the rc and put it back in memory
	memcpy(rc_index, mem_addr + sizeof(struct node) * POD_SIZE * POD_NUMBER_LIMIT + sizeof(int) * POD_NUMBER_LIMIT, sizeof(rc_index));
	rc_index[hashkey]--;
	memcpy(mem_addr + sizeof(struct node) * POD_SIZE * POD_NUMBER_LIMIT + sizeof(int) * POD_NUMBER_LIMIT, rc_index, sizeof(int) * POD_NUMBER_LIMIT);

	// Release access to other readers. If this was the last rc release the the access to the writer
	if (rc_index[hashkey] == 0) sem_post(sem_db);
	sem_post(sem_mutex);
	/****************End semaphore***************/

	/**********************************************Critical Code End*****************************************************/

	sem_close(sem_mutex); sem_close(sem_db);
	munmap(mem_addr, s.st_size);
	free(new_node);
	return read_all;
}
