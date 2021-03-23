#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

//generates a key of the specified length consisting of capital alphabet letters and the space character
char* generateKey(int keylen){
    //all possible values to be picked from
    char* choices = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";
    //array to store the key
    char* key_string = calloc(keylen + 1, sizeof(char));
    //seed random number generator
    srand(time(NULL));

    //loop through each index of key, generating a random number corresponding to one of the values in choices
    int i;
    for(i = 0; i < keylen; ++i){
        int x = rand() % 27;     //mod 27 to generate a value within the bounds of choices
        //append to key whatever letter is at index x in choices
        key_string[i] = choices[x];
    }
    //add null terminator
    key_string[keylen] = '\0';
    return key_string;
}
int main(int argc, char* argv[]){
    if(argc < 2){
        printf("Error, no key length\n");
        return EXIT_FAILURE;
    }
    else{
        char* key;
        //convert keylen to integer
        int keylen;
        keylen = atoi(argv[1]);
        key = generateKey(keylen);
        printf("%s\n", key);
        return EXIT_SUCCESS;
    }
}