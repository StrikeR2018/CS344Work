
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define MAXLINE 50
#define MAXCHAR 1000

//MANY OF THE FUNCTIONS IN THIS FILE WERE ADAPTED FROM THE SAMPLE PROGRAMS IN JUSTIN GOINS' MODULE 6 AND THE SAMPLE PROGRAM IN THE ASSIGNMENT DESCRIPTION

#define NUM_THREADS 4
// Buffer 1
char buffer_1[MAXLINE][MAXCHAR];
// Number of items in the buffer
int count_1 = 0;
// Index where the input thread will put the next item
int prod_idx_1 = 0;
// Index where the square-root thread will pick up the next item
int con_idx_1 = 0;
// Initialize the mutex for buffer 1
pthread_mutex_t mutex_1 = PTHREAD_MUTEX_INITIALIZER;
// Initialize the condition variable for buffer 1
pthread_cond_t full_1 = PTHREAD_COND_INITIALIZER;


// Buffer 2
char buffer_2[MAXLINE][MAXCHAR];
// Number of items in the buffer
int count_2 = 0;
// Index where the square-root thread will put the next item
int prod_idx_2 = 0;
// Index where the output thread will pick up the next item
int con_idx_2 = 0;
// Initialize the mutex for buffer 2
pthread_mutex_t mutex_2 = PTHREAD_MUTEX_INITIALIZER;
// Initialize the condition variable for buffer 2
pthread_cond_t full_2 = PTHREAD_COND_INITIALIZER;

// Buffer 3
char buffer_3[MAXLINE][MAXCHAR];
// Number of items in the buffer
int count_3 = 0;
// Index where the square-root thread will put the next item
int prod_idx_3 = 0;
// Index where the output thread will pick up the next item
int con_idx_3 = 0;
// Initialize the mutex for buffer 3
pthread_mutex_t mutex_3 = PTHREAD_MUTEX_INITIALIZER;
// Initialize the condition variable for buffer 3
pthread_cond_t full_3 = PTHREAD_COND_INITIALIZER;

//gets the string input from the user or file
char* get_user_input(){
    char* input;
    char *currLine = NULL;
    size_t max = MAXCHAR;
    //allocate space for the user input string
    input = calloc(MAXCHAR + 1, sizeof(char));
    getline(&currLine, &max , stdin);

    //put the string into the input after getting it with getline
    strcpy(input,currLine);

    return input;
}

//puts an item into buffer 1
void put_buff_1(char* item){
    // Lock the mutex before putting the item in the buffer
    pthread_mutex_lock(&mutex_1);
    // Put the item in the buffer
    strcpy(buffer_1[prod_idx_1],item);
    // Increment the index where the next item will be put.
    prod_idx_1 = prod_idx_1 + 1;
    count_1++;
    // Signal to the consumer that the buffer is no longer empty
    pthread_cond_signal(&full_1);
    // Unlock the mutex
    pthread_mutex_unlock(&mutex_1);
}

//function called by thread 1
void *get_input(void *args){
    while(1){
        char* item = get_user_input();  //get input
        put_buff_1(item);
        //check for end line
        if(strncmp(item, "STOP\n", strlen("STOP\n")) == 0){
            break;
        }
    }
    return NULL;
}

//gets an item from buffer 1
char* get_buff_1(){
    // Lock the mutex before checking if the buffer has data
    pthread_mutex_lock(&mutex_1);
    while (count_1 == 0)
    // Buffer is empty. Wait for the producer to signal that the buffer has data
    pthread_cond_wait(&full_1, &mutex_1);
    char* item = buffer_1[con_idx_1];
    // Increment the index from which the item will be picked up
    con_idx_1 = con_idx_1 + 1;
    count_1--;
    // Unlock the mutex
    pthread_mutex_unlock(&mutex_1);
    // Return the item
    return item;
}

//put an item into buffer 2
void put_buff_2(char* item){
    // Lock the mutex before putting the item in the buffer
    pthread_mutex_lock(&mutex_2);
    // Put the item in the buffer
    strcpy(buffer_2[prod_idx_2],item);
    // Increment the index where the next item will be put.
    prod_idx_2 = prod_idx_2 + 1;
    count_2++;
    // Signal to the consumer that the buffer is no longer empty
    pthread_cond_signal(&full_2);
    // Unlock the mutex
    pthread_mutex_unlock(&mutex_2);
}

//function called by thread 2
void *line_sep(void *args){
    char* item;
    while(1){
        item = get_buff_1();    //get current item
        int i;
        //loop through curr item, when line separator char is discovered, replace with space
        for(i = 0; i < strlen(item); i++){
            if(item[i] == '\n'){
                item[i] = ' ';
            }
        }
        put_buff_2(item);
        //check for end line
        if(strncmp(item, "STOP ", strlen("STOP ")) == 0){
            break;
        }
    }

    return NULL;
}

//get an item from buffer 2
char* get_buff_2(){
    // Lock the mutex before checking if the buffer has data
    pthread_mutex_lock(&mutex_2);
    while (count_2 == 0)
    // Buffer is empty. Wait for the producer to signal that the buffer has data
    pthread_cond_wait(&full_2, &mutex_2);
    char* item = buffer_2[con_idx_2];
    // Increment the index from which the item will be picked up
    con_idx_2 = con_idx_2 + 1;
    count_2--;
    // Unlock the mutex
    pthread_mutex_unlock(&mutex_2);
    // Return the item
    return item;
}

//put an item in buffer 3
void put_buff_3(char* item){
    // Lock the mutex before putting the item in the buffer
    pthread_mutex_lock(&mutex_3);
    // Put the item in the buffer
    strcpy(buffer_3[prod_idx_3], item);
    // Increment the index where the next item will be put.
    prod_idx_3 = prod_idx_3 + 1;
    count_3++;
    // Signal to the consumer that the buffer is no longer empty
    pthread_cond_signal(&full_3);
    // Unlock the mutex
    pthread_mutex_unlock(&mutex_3);
}

//function called by thread 3
void* plus(){
    char* item;
    while(1){
        item = get_buff_2();
        int i = 0;
        int j;
        int itemLen = strlen(item);
        char contPoint; //point to continue from after a replication occurs
        //loop through current item, replace ++ with ^
        while(i != itemLen){
            if(item[i] == '+' && item[i + 1] == '+'){
            item[i] = '^';      
                                
            j = i + 1;
            while(j != itemLen){
                contPoint = item[j + 1];
                item[j] = contPoint;
                j++;
            }
            item[j+1] = '\0';   //make the last char of item string null
            }
            i++;
        }
        put_buff_3(item);
        //check for end line
        if(strncmp(item, "STOP ", strlen("STOP ")) == 0) {
            break;
        }
    }



    return NULL;
}

//gets an item from buffer 3
char* get_buff_3(){
    // Lock the mutex before checking if the buffer has data
    pthread_mutex_lock(&mutex_3);
    while (count_3 == 0)
    // Buffer is empty. Wait for the producer to signal that the buffer has data
    pthread_cond_wait(&full_3, &mutex_3);
    char* item = buffer_3[con_idx_3];
    // Increment the index from which the item will be picked up
    con_idx_3 = con_idx_3 + 1;
    count_3--;
    // Unlock the mutex
    pthread_mutex_unlock(&mutex_3);
    // Return the item
    return item;
}

//function called by thread 4
void *print_line(void *args){
    char* item;
    //string to hold the the current line, including leftovers after a line is printed
    char* maxArr = calloc(sizeof(MAXCHAR + 1), sizeof(char));
    int pos;    //position in maxArr
    while(1){
        item = get_buff_3();
        int i;
        for(int i = 0; i < strlen(item); i++){
            maxArr[pos] = item[i];
            pos++;
            //when position is 80, we have a line ready to print, so print the chars from maxArr
            if(pos == 80){  
                int j;
                for (int j = 0; j < 80; j++) { 
                    printf("%c", maxArr[j]);
                }
                printf("\n");
                pos = 0;    //reset pos to zero to start copying the next line to print into maxArr

            }

        }
        //check for end line
        if(strncmp(item, "STOP ", strlen("STOP ")) == 0){
            break;
        }

    }
    return NULL;
}
int main(){
    srand(time(0));
    pthread_t input_t, line_t, plus_t, output_t;
    // Create the threads
    pthread_create(&input_t, NULL, get_input, NULL);
    pthread_create(&line_t, NULL, line_sep, NULL);
    pthread_create(&plus_t, NULL, plus, NULL);
    pthread_create(&output_t, NULL, print_line, NULL);

    // Wait for the threads to terminate
    pthread_join(input_t, NULL);
    pthread_join(line_t, NULL);
    pthread_join(plus_t, NULL);
    pthread_join(output_t, NULL);

    return EXIT_SUCCESS;
}