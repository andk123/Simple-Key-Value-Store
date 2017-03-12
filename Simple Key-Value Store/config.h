#ifndef __COMP310_ASSIGNMENT2_TEST_CASE_CONFIG__
#define __COMP310_ASSIGNMENT2_TEST_CASE_CONFIG__

#include <stdlib.h>
#include <stdio.h>

#define KEY_MAX_LENGTH   32
#define VALUE_MAX_LENGTH 256
#define POD_NUMBER_LIMIT 256
#define POD_SIZE 256
#define READ_CYCLING_MAX 512 //Max number of keys we can keep track of, for the read cycle


extern int  kv_store_create(const char *name);
extern int  kv_store_write(char *key, char *value);
extern char *kv_store_read(const char *key);
extern char **kv_store_read_all(const char *key);
extern int kv_delete_db();
extern void free_read_cycle();
extern void free_to_exit();
extern void initialize();
extern void unlink_sem();

struct node {
	char key[KEY_MAX_LENGTH];
	char value[VALUE_MAX_LENGTH];
};

struct index {
	char key[KEY_MAX_LENGTH];
	int index;
};

extern char *db_name;
extern char *mem_addr;
extern struct index *read_array[READ_CYCLING_MAX];

#endif
