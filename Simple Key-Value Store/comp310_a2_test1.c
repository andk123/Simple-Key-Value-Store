/* Written by Xiru Zhu modified by Arnold K
 * This is for testing your assignment 2. 
 */

#include "comp310_a2_test.h"
void kill_shared_mem(){
    //In case it already exists
    shm_unlink(__TEST_SHARED_MEM_NAME__);
}

void intHandler(int dummy) {
    kv_delete_db();
    exit(0);
}

void read_write_test(char **keys_buf, char ***data_buf, int elem_num[__TEST_MAX_KEY__], int i, int *errors){
    int temp_flag = -1;
    char *temp;
    generate_string(data_buf[i][elem_num[i]], __TEST_MAX_DATA_LENGTH__);
    generate_key(keys_buf[i], __TEST_MAX_KEY_SIZE__, keys_buf, i+1);

    temp_flag = kv_store_write(keys_buf[i], data_buf[i][elem_num[i]]);
    temp = kv_store_read(keys_buf[i]);
    if(temp == NULL || temp_flag != 0){
        printf("Gave NULL when value should be there or write failed\n");
        errors++;
    }else if(strcmp(temp, data_buf[i][elem_num[i]]) != 0){
        printf("Read : %s\nShould have read %s\n", temp, data_buf[i][elem_num[i]]);
        errors++;
    }
    elem_num[i]++;
    free(temp);
}

void write_test(char **keys_buf, char ***data_buf, int elem_num[__TEST_MAX_KEY__], int k, int *errors){
    int temp_flag = -1;
    char *temp;
    printf("-----------Attempting Writes ROUND %d/%d-----------\n", k+1, __TEST_MAX_POD_ENTRY__);
    for(int i = 0; i < __TEST_MAX_KEY__; i++){
        generate_unique_data(data_buf[i][elem_num[i]], (rand() + 5) % __TEST_MAX_DATA_LENGTH__, data_buf[i], k);
        temp_flag = kv_store_write(keys_buf[i], data_buf[i][elem_num[i]]);
        if(temp_flag != 0){
            printf("Write failed\n");
            *errors += 1;
        }
        elem_num[i]++;
    }
    printf("-----------Error Count: %d-----------\n\n", *errors);
}

void read_test(char **keys_buf, char ***data_buf, int elem_num[__TEST_MAX_KEY__], 
               int expected_result[__TEST_MAX_POD_ENTRY__][__TEST_MAX_KEY__],
               int k, int *errors){
    int temp_flag = -1;
    char *temp;
    printf("-----------Attempting Reads ROUND %d/%d-----------\n", k+1, __TEST_MAX_POD_ENTRY__);
    for(int i = 0; i < __TEST_MAX_KEY__; i++){
        temp = kv_store_read(keys_buf[i]);
        if(temp == NULL){
            printf("This should not return NULL. All these keys do have values\n");
            *errors += 1;
            continue;
        }
        temp_flag = -1;
        for(int j = 0; j < elem_num[i]; j++){
            if(strcmp(temp, data_buf[i][j]) == 0){
                temp_flag = j;
                break;
            }
        }
        if(temp_flag == -1){
            printf("No Value %s Stored at key %s\n", temp, keys_buf[i]);
            *errors += 1;
        }else{
            expected_result[k][i] = temp_flag;
        }
        free(temp);
    }
    printf("-----------Error Count: %d-----------\n\n", *errors);
}

void get_patterns(int expected_result[__TEST_MAX_POD_ENTRY__][__TEST_MAX_KEY__],
                  int i, int *patterns, int *pattern_length, int *errors){
    *pattern_length = 1;
    int min_pattern = __TEST_MAX_POD_ENTRY__ + 1;
    int min_index = 0;
    int pattern_index;
    int invalid_flag = 0;
    int all_same_flag = 1;
    memset(patterns, __TEST_MAX_POD_ENTRY__ + 1, __TEST_MAX_POD_ENTRY__ * sizeof(int));
    //Find Pattern
    for(int k = 0; k < __TEST_MAX_POD_ENTRY__; k++){
        if(min_pattern > expected_result[k][i]){
            min_pattern = expected_result[k][i];
            min_index = k;
        }
    }
    patterns[0] = min_pattern;
    for(int k = 1; k < __TEST_MAX_POD_ENTRY__; k++){
        pattern_index = (k + min_index) % __TEST_MAX_POD_ENTRY__;
        if(patterns[0] == expected_result[pattern_index][i]){
            break;   
        }else{
            patterns[k] = expected_result[pattern_index][i];
            *pattern_length += 1;
        }
    }
    //Test for extreme collision
    for(int k = 1; k < __TEST_MAX_POD_ENTRY__; k++){
        if(expected_result[0][i] != expected_result[k][i]){
            all_same_flag = 0;
            break;
        }
    }
    if(all_same_flag){
        printf("Only returning a single value for each key. There should be multiple unless extreme collision. \n");
        *errors += __TEST_MAX_POD_ENTRY__;
    }
}

void read_order_test(int expected_result[__TEST_MAX_POD_ENTRY__][__TEST_MAX_KEY__],
                      int i, int *patterns, int pattern_length, int *errors){
    //Check Read Order
    int FIFO_TEST[pattern_length];
    int start_index = 0;
    for(int k = 0; k < pattern_length; k++){
        if(expected_result[0][i] == patterns[k]){
            start_index = k;
            break;
        }
    }
    for(int k = 0; k < __TEST_MAX_POD_ENTRY__; k++){
        if(expected_result[k][i] != patterns[(k+start_index)%pattern_length]){
            printf("Invalid Ordering ... %d %d %d\n", pattern_length, patterns[pattern_length - 1], expected_result[k][i]);
            errors++;
            printf("Current Pattern: ");
            for(int j = 0; j < __TEST_MAX_POD_ENTRY__; j++){
                printf("%d, ", expected_result[j][i]);
            }
            printf("\n");
            break;
        }
    }

    memset(FIFO_TEST, -1, pattern_length * sizeof(int));
    //Check FIFO
    for(int i = 0; i < pattern_length; i++){
        FIFO_TEST[__TEST_MAX_POD_ENTRY__ - 1 - patterns[i]] = 1;
    }
    for(int i = 0; i < pattern_length; i++){
        if(FIFO_TEST[i] != 1){
            printf("Error! Missing value %d This should have been read\n", __TEST_MAX_POD_ENTRY__ -1 - i);
            errors++;
        }
    }
}

void read_all_test(char ** keys_buf, char *** data_buf, 
                   int expected_result[__TEST_MAX_POD_ENTRY__][__TEST_MAX_KEY__],
                   int i, int *patterns, int pattern_length, int *errors){
    //Read All tests
    char **read_all;
    int start_index = 0;
    int total_read = 0;
    read_all = kv_store_read_all(keys_buf[i]);
    if(read_all == NULL){
        printf("No Value returned by read all\n");
        *errors += 1;
    }else{
        for(int k = 0; k < pattern_length; k++){
            if(strcmp(read_all[0], data_buf[i][patterns[k]]) == 0){
                start_index = k;
                break;
            }
        }

        for(int k = 0;;k++){
            if(read_all[k] == NULL){
                break;
            }else{
                total_read++;
            }
        }
        if(total_read != pattern_length){
            printf("Invalid Read Length Based on Pattern Analysis\n");
            *errors += total_read > pattern_length ? total_read - pattern_length: pattern_length - total_read;
        }
        for(int k = 0; k < (total_read < pattern_length ? total_read : pattern_length); k++){
            if(strcmp(read_all[k], data_buf[i][patterns[(k + start_index) % pattern_length]]) != 0){
                printf("Invalid Ordering ... %d %d %d\n", pattern_length, patterns[pattern_length - 1], expected_result[k][i]);
                *errors += 1;
                printf("Current Pattern: ");
                for(int j = 0; j < __TEST_MAX_POD_ENTRY__; j++){
                    printf("%d, ", expected_result[j][i]);
                }
                printf("\n");
                break;
            }
        }
        for(int k = 0; k < total_read; k++){
            free(read_all[k]);
        }
        free(read_all);
    }
}
int main(){
    int errors = 0;
    char *temp;
    char **temp_all;
    char ***data_buf;
    char **keys_buf;
    int elem_num[__TEST_MAX_KEY__];
    int expected_result[__TEST_MAX_POD_ENTRY__][__TEST_MAX_KEY__];
    int patterns[__TEST_MAX_POD_ENTRY__];
    int pattern_length = 0;

    srand(time(NULL));
    //Handlers
    signal(SIGINT, intHandler);
    signal(SIGQUIT, intHandler);
    signal(SIGTSTP, intHandler);

    kill_shared_mem();

    for(int i = 0; i < __TEST_MAX_POD_ENTRY__; i++){
        memset(expected_result, -1, __TEST_MAX_KEY__ * sizeof(int));
    }
    memset(elem_num, 0, __TEST_MAX_KEY__ * sizeof(int));
    memset(patterns, __TEST_MAX_POD_ENTRY__ + 1, __TEST_MAX_POD_ENTRY__ * sizeof(int));

    data_buf = calloc(1, sizeof(char **) * __TEST_MAX_KEY__);
    keys_buf = calloc(1, sizeof(char **) * __TEST_MAX_KEY__);
    for(int i = 0; i < __TEST_MAX_KEY__; i++){
        data_buf[i] = calloc(1, sizeof(char*) * __TEST_MAX_POD_ENTRY__);
        keys_buf[i] = calloc(1, sizeof(char*) * __TEST_MAX_KEY_SIZE__);
        for(int j = 0; j < __TEST_MAX_POD_ENTRY__; j++){
            data_buf[i][j] = calloc(1, sizeof(char) * __TEST_MAX_DATA_LENGTH__ + 1);
        }
    }

    printf("-----------One Threaded Testing Of Shared Memory Database-----------\n");
    printf("-----------Initializing DB-----------\n");
    initialize();
    kv_store_create(__TEST_SHARED_MEM_NAME__);
    printf("-----------Attempting Invalid Read-----------\n");

    temp = kv_store_read("gtx 1080ti!!!");
    if(temp != NULL){
        errors++;
        printf("Did not return null on invalid key. \n");
        free(temp);
    }
    temp_all = kv_store_read_all("need that ryzen");
    if(temp_all != NULL){
        errors++;
        printf("Did not return null on invalid key. \n");
        free(temp_all);
    }

    printf("-----------Attempting Simple Read and Writes-----------\n");
    for(int i = 0; i < __TEST_MAX_KEY__; i++){
        read_write_test(keys_buf, data_buf, elem_num, i, &errors);
    }
    printf("-----------Error Count: %d-----------\n\n", errors);

    for(int k = 0; k < __TEST_MAX_POD_ENTRY__ - 1; k++){
        write_test(keys_buf, data_buf, elem_num, k, &errors);
    }

    for(int k = 0; k < __TEST_MAX_POD_ENTRY__; k++){
        read_test(keys_buf, data_buf, elem_num, expected_result, k, &errors);
    }

    printf("-----------Testing FIFO/Read Order/Read Alls -----------\n");
    for(int i = 0; i < __TEST_MAX_KEY__; i++){
        get_patterns(expected_result, i, patterns, &pattern_length, &errors);
        read_order_test(expected_result, i, patterns, pattern_length, &errors);
        read_all_test(keys_buf, data_buf, expected_result, i, patterns, pattern_length, &errors);
    }

    printf("-----------Error Count: %d-----------\n\n", errors);
    printf("-----------One Threaded Testing Of Shared Memory Database END-----------\n");
    printf("-----------TOTAL ERROR: %d-----------\n\n", errors);

    for(int i = 0; i < __TEST_MAX_KEY__; i++){
        for(int j = 0; j < __TEST_MAX_POD_ENTRY__; j++){
            free(data_buf[i][j]);
        }
        free(data_buf[i]);
        free(keys_buf[i]);
    }
    free(data_buf);
    free(keys_buf);
	kv_delete_db();
}
