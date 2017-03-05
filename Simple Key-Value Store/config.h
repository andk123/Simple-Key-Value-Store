#ifndef __COMP310_ASSIGNMENT2_TEST_CASE_CONFIG__
#define __COMP310_ASSIGNMENT2_TEST_CASE_CONFIG__

#include <stdlib.h>

#define DATA_BASE_NAME   "database"
#define KEY_MAX_LENGTH   32
#define VALUE_MAX_LENGTH 256
#define POD_NUMBER_LIMIT 256
#define POD_SIZE 256
#define READ_CYCLING_MAX 512


/* if you use the default interface uncomment below */
extern int  kv_store_create(const char *name);
extern int  kv_store_write(const char *key, const char *value);
extern char *kv_store_read(const char *key);
extern char **kv_store_read_all(const char *key);

struct node {
	char key[KEY_MAX_LENGTH];
	char value[VALUE_MAX_LENGTH];
};

struct index {
	char key[KEY_MAX_LENGTH];
	int index;
};

extern char *db_name;
extern struct index *read_array[READ_CYCLING_MAX];

/* if you write your own interface, please fill the following adaptor */
//int    (*kv_store_create)(const char*)             = NULL;
//int    (*kv_store_write)(const char*, const char*) = NULL;
//char*  (*kv_store_read)(const char*)               = NULL;
//char** (*kv_store_read_all)(const char* key)       = NULL;

#endif
