#include "read_eval.h"
#include <signal.h>

//Global variables
char *buffer = NULL;


//To take care of the signal CTLR+C to exit program and free the remaining variable at same time
static void sigHandler(int sig){
	if(sig == SIGINT){
		kv_delete_db();
		//free_to_exit();
		free(buffer);
		exit(EXIT_SUCCESS);
	}
}

char** tokenize(char* buffer, const char delimiter) {
	size_t length = strlen(buffer);
	for (size_t i = (size_t) 0; i < length; i++) {
		if (buffer[i] == delimiter) buffer[i] = '\0';
		if (buffer[i] == '\n') buffer[i] = '\0';
	}
	size_t num_token = (size_t) 0;
	int flag = 0;
	for (size_t i = (size_t) 0; i < length; i++) {
		if (buffer[i] == '\0') flag = 0;
		if (buffer[i] != '\0' && flag == 0) {
			num_token++;
			flag = 1;
		}
	}
	char** ret_tokens = (char**) malloc(sizeof(char*) * (num_token + 1));
	if (ret_tokens == NULL) return NULL;
	size_t count = (size_t) 0; flag = 0;
	for (size_t i = (size_t) 0; i < length; i++) {
		if (buffer[i] == '\0') flag = 0;
		if (buffer[i] != '\0' && flag == 0) {
			ret_tokens[count] = &buffer[i];
			count++;
			flag = 1;
		}
	}
	return ret_tokens;
}

void read_eval() {
	size_t length = (size_t) 0;
	printf(">> ");
	while ((getline(&buffer, &length, stdin) != 0)) {
		char* buffer_to_free = buffer;
		char** cmd = tokenize(buffer, ' ');
		//Make sure the line is not empty
		if(cmd[0] == NULL){
		}
		else if (strcmp(cmd[0], "exit") == 0) {
			kv_delete_db();
			//free_to_exit();
			free(buffer_to_free);
			free(cmd);
			return;
		} else if (strcmp(cmd[0], "create") == 0) {
			//kv_delete_db();
			initialize();
			printf("exec code = %d\n", kv_store_create(cmd[1]));
		} else if (strcmp(cmd[0], "write") == 0) {
			char *args1, *args2;

			//Input validation
			if(cmd[1] == NULL || cmd[2] == NULL){
				printf("Please enter a valid key and value...\n");
				printf("exec code = -1\n");
			}
			else {
				args1 = strdup(cmd[1]); args2 = strdup(cmd[2]);
				printf("exec code = %d\n", kv_store_write(args1, args2));

				free(args1);
				free(args2);
			}
		} else if (strcmp(cmd[0], "read") == 0) {
			char* content = kv_store_read(cmd[1]);
			if (content == NULL) {
				printf("read content = (null)\n");
			} else {
				printf("read content = %s\n", content);
				free(content);
			}
		} else if (strcmp(cmd[0], "readall") == 0) {
			char** content = kv_store_read_all(cmd[1]);
			if (content == NULL) {
				printf("read all error\n");
			} else {
				size_t count = (size_t) 0;
				while (content[count] != NULL) {
					printf("result[%zu]=%s\n", count, content[count]);
					free(content[count]);
					count = count + 1;
				}
				free(content);
			}
		}
		else if (strcmp(cmd[0], "saveexit") == 0) {
					free_to_exit();
					free(buffer_to_free);
					free(cmd);
					return;
				}
		else if (!strcmp(cmd[0], "help") || !strcmp(cmd[0], "-h")) {
			printf("1. Please use CTRL+C to OR type 'exit' to exit program and effectively clear all the memory\n");
			printf("2. By default when you quit, the key-value store created will be deleted\n");
			printf("3. To quit but keep the store saved. Type 'saveexit'. The saved store will then be located at /dev/shm/ for futur sessions\n");
			printf("4. Key bigger than %d char will be automatically truncated\n", KEY_MAX_LENGTH-1);
			printf("5. Value bigger than %d char will be automatically truncated\n", VALUE_MAX_LENGTH-1);
			printf("6. If you change database with the command CREATE, the read cycle will be reset for the process\n");
			printf("7. If a pod is full, the oldest entry in the pod will be deleted regardless of its key (FIFO)\n");
			//printf("8. SET MAXIMUM NUMBER OF KEY-VALUE PAIRS\n"); //Not implemented yet
		}
		free(buffer_to_free);
		free(cmd);
		buffer = NULL; length = (size_t) 0;
		printf(">> ");
	}
}

int main(int argc, char const *argv[]) {

	// Listen to the signal CTRL+C (kill program)
	if (signal(SIGINT, sigHandler) == SIG_ERR){
		printf("ERROR! Could not bind the signal handler\n");
		exit(EXIT_FAILURE);
	}

	printf("Welcome to the Key-value Store Database\n"
			"Please enter help or -h to show a list of the features\n");

	read_eval();
	return 0;
}

