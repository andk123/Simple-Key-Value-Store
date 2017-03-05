#include "read_eval.h"

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
    char* buffer = NULL; size_t length = (size_t) 0;
    printf(">> ");
    while ((getline(&buffer, &length, stdin) != 0)) {
        char* buffer_to_free = buffer;
        char** cmd = tokenize(buffer, ' ');
        if (strcmp(cmd[0], "exit") == 0) {
            free(buffer_to_free);
            free(cmd);
            return;
        } else if (strcmp(cmd[0], "create") == 0) {
            printf("exec code = %d\n", kv_store_create(cmd[1]));
        } else if (strcmp(cmd[0], "write") == 0) {
            printf("exec code = %d\n", kv_store_write(cmd[1], cmd[2]));
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
        free(buffer_to_free);
        free(cmd);
        buffer = NULL; length = (size_t) 0;
        printf(">> ");
    }
}

int main(int argc, char const *argv[]) {
    read_eval();
    return 0;
}
