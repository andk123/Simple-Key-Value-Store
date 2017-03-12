/* Written by Xiru Zhu modified by Arnold K
 * This is for testing your assignment 2. 
 */

#include "comp310_a2_test.h"

void kill_shared_mem(){
    //In case it already exists
    sem_unlink(__TEST_SHARED_SEM_NAME__);
    shm_unlink(__TEST_SHARED_MEM_NAME__);
}

void intHandler(int dummy) {
    for(int i = 0; i < __TEST_FORK_NUM__; i++){
        kill(pids[i], SIGKILL);
    }
    sem_close(open_sem_lock);
    sem_unlink(__TEST_SHARED_SEM_NAME__);
    kv_delete_db();
    exit(0);
}

int main(){
    int max_write, errors;
    char buf[__TEST_MAX_DATA_LENGTH__];
    char key[__TEST_MAX_KEY_SIZE__];
    int write_table[__TEST_FORK_NUM__];
    int temp_flag;
    int write_count;
    char **read_all;

    kill_shared_mem();
    open_sem_lock = sem_open(__TEST_SHARED_SEM_NAME__, O_CREAT, 0644, 0);
    srand(time(NULL));

    signal(SIGINT, intHandler);
    signal(SIGQUIT, intHandler);
    signal(SIGTSTP, intHandler);

    errors = 0;
    printf("-----------Multiprocess Testing Of Shared Memory Database-----------\n");
    printf("-----------CAUTION. This test assumes test 1 with no errors. -----------\n");
    for(int k = 0; k < RUN_ITERATIONS; k++){
        printf("-----------ROUND %d-----------\n", k + 1);
        temp_flag = -1;
        write_count = 0;
        memset(pids, -1, __TEST_FORK_NUM__ * sizeof(int));

        max_write = (rand()) % __TEST_MAX_POD_ENTRY__/2 + __TEST_MAX_POD_ENTRY__/2;
        for(int j = 0; j < __TEST_FORK_NUM__; j++){
            write_table[j] = max_write/__TEST_FORK_NUM__ - rand() % 5;
            write_count += write_table[j];
        }
        generate_string(key, __TEST_MAX_KEY_SIZE__);
        int child_num;
        for(child_num = 0; child_num < __TEST_FORK_NUM__; child_num++){
            pids[child_num] = fork();
            if(pids[child_num] == 0){
                break;
            }
        }
        if(pids[__TEST_FORK_NUM__ - 1] > 0){
            // Wait for childs to finish up
        	temp_flag = kv_store_create(__TEST_SHARED_MEM_NAME__);
            if(temp_flag != 0){
                printf("Error! Init failed\n");
                errors++;
            }
            for(int i = 0; i < __TEST_FORK_NUM__; i++){
                sem_post(open_sem_lock);
            }
            for(int i = 0; i < __TEST_FORK_NUM__; i++){
                waitpid(pids[i], NULL, 0);
            }
        }else{
            open_sem_lock = sem_open(__TEST_SHARED_SEM_NAME__, 0);
            errno = 0;
            sem_wait(open_sem_lock);
            temp_flag = kv_store_create(__TEST_SHARED_MEM_NAME__);
            if(temp_flag != 0){
                printf("Error! Init failed\n");
            }
            int i;
            for(i = 0; i < write_table[child_num]; i++){
                generate_string(buf, __TEST_MAX_DATA_LENGTH__);
                temp_flag = kv_store_write(key, buf);
                if(temp_flag < 0){
                    printf("Error! Write failed\n");
                }
            }
            sem_close(open_sem_lock);
            exit(0);
        }
        //At this point we read all values  
        read_all = kv_store_read_all(key);
        if(read_all == NULL){
            printf("Error! Read failed\n");
            errors++;
            continue;
        }
        temp_flag = 0;
        for(int i = 0;;i++){
            if(read_all[i] == NULL){
                break;
            }else{
                temp_flag++;
            }
        }
        if(temp_flag != write_count){
            printf("Error! Invalid number of reads. Should be %d but only received %d\n", write_count, temp_flag);
            errors++;
        }

        for(int i = 0; i < temp_flag; i++){
            free(read_all[i]);
        }
        free(read_all);
        kv_delete_db();
    }
    sem_close(open_sem_lock);
    sem_unlink(__TEST_SHARED_SEM_NAME__);
    printf("-----------Error Count: %d-----------\n\n", errors);
    printf("-----------Multiprocess Testing Of Shared Memory Database END-----------\n");
    return 0;
}
