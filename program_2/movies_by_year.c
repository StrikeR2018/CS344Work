#define _GNU_SOURCE
#define PREFIX "movies_"
#define SUFFIX ".csv"
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include<dirent.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<time.h>
#include<unistd.h>
#include<fcntl.h>
#include<stdbool.h>
//Sample code from Justin Goins was used as the base for this project. Canvas Modules were also used

typedef struct film{
    char *Title;
    char *Year;
    char *Rating;
    struct film *next;
    char *Languages;
}movie;

movie *head = NULL;
movie *tail = NULL;
int count = 0;  //counter for the number of structs
int found = 0;            //marker for if the file is ever found


//create a instance of the movie struct from one line of the input file
movie* createMovie(char *currentLine){
    //create currnet movie struct
    movie *currMov = malloc(sizeof(movie));
    char *saveptr;
    //get title token, which ends after first comma, so thats the delimeter
    char *token = strtok_r(currentLine, ",", &saveptr);
    currMov->Title = calloc(strlen(token)+1, sizeof(char)); //+1 for terminating character (i think)
    strcpy(currMov->Title, token);

    //get year
    token = strtok_r(NULL, ",", &saveptr);
    currMov->Year = calloc(strlen(token)+1, sizeof(char));
    strcpy(currMov->Year, token);

    //get languages
    token = strtok_r(NULL, ",", &saveptr);
    currMov->Languages = calloc(strlen(token)+1, sizeof(char));
    strcpy(currMov->Languages, token);  

    //get rating
    token = strtok_r(NULL, "\n", &saveptr);                 //delimeter is \n because this is the last thing on the line
    currMov->Rating = calloc(strlen(token)+1, sizeof(char));
    strcpy(currMov->Rating, token);

    return currMov;
}

//parse file line by line creating movie structs in a linked list
movie* processFile(char *filePath){
    FILE *movieFile = fopen(filePath, "r");
    char *currLine = NULL;
    size_t len = 0; //size of input buffer
    ssize_t nread;
    char *token;

    //read first line outside of loop
    nread = getline(&currLine, &len, movieFile);

    //loops until there are no lines to be read
    while((nread = getline(&currLine, &len, movieFile)) != -1){

        //create a struct for currLine
        movie *newNode = createMovie(currLine);
        count++;

        if(head == NULL){
            head = newNode;
            tail = newNode;
        } else{
            tail->next = newNode;
            tail = newNode;
            tail->next = NULL;
        }


    }

    //print msg of process completion
    printf("\nNow processing the chosen file named %s\n", filePath);

    //free and close stuff
    free(currLine);
    fclose(movieFile);
    
    return head;
}

//utiltiy function for writeFiles to check if a year has already been looked at
int isIn(int year, int visited[]){
    int i;
    for(i = 0; i < count+1; ++i){
        if(visited[i] == year){
            return 1;
        }
    }
    return 0;
}

//utility function to free the struct list to prevent bugs, such as when a user enters 1 twice in a session. Linked list should be made fresh each time
//Inspiration: https://www.geeksforgeeks.org/write-a-function-to-delete-a-linked-list/
void freeList(movie *list){
    movie *current = head;
    movie *next;

    while(list != NULL){
        next = current->next;
        free(current);
        current = next;
    }
    head = NULL;
    tail = NULL;
}

//write the information from the movie list to a series of text files in the given directory, one file for each year containing all its movies
void writeFiles(movie *list, char *dirName){
    //visited array
    int visited[count+1];
    int nextIndex = 0;      //next available index in visited
    char filepath[256];    
    //loop through list, at each line, make a txt file for that year and loop through again to print all movies from that year
    while(list != NULL){
        movie *currMovie = head;    //variable to store head, so that inner loop always starts from head
        char *firstTitle = list->Title;   //get starting title, so that inner loop doesn't repeate entries for starting year
        //check if current has been evaluated
        if(isIn(atoi(list->Year), visited) == 1){
            list = list->next;
            continue;   //current year already evaluated
        }
        else{
            visited[nextIndex] = atoi(list->Year);      //add to visited
            nextIndex++;
            //adapted from lecture slides and file module examples. creating files
            int fileDescriptor;
            //create filepath and file name
            sprintf(filepath, "%s/%s.txt", dirName, list->Year);

            fileDescriptor = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0640); //open file, create it or truncate as necessary, user as r and w perms, group has r perm.
            write(fileDescriptor, list->Title, strlen(list->Title));    //write a msg to file
            while(currMovie != NULL){
                //check if years match and if curr title != the first title
                if( (atoi(list->Year) == atoi(currMovie->Year)) && currMovie->Title != firstTitle){
                    //write newline char
                    write(fileDescriptor, "\n", 1);
                    write(fileDescriptor, currMovie->Title, strlen(currMovie->Title));
                    currMovie = currMovie->next;
                }
                else{
                    currMovie = currMovie->next;    //years don't match
                }
            }
        close(fileDescriptor);
        list = list->next;      //increment outer loop
        }
    }

    freeList(list);  //free the list to avoid duplicate file contents if processfile() is called on same file again in same session
}

//process the largest file in the working directory
void largestFile(){
    //loop through current directory, adapted from the 3_4  & 3_5 directory programs from the directories module
    DIR *currDir = opendir(".");    //current directory
    struct dirent *aDir;             //current directory entry
    struct stat dirStat;           //struct for dir metadata
    off_t maxSize = 0;              //start max file size at 0
    char entryName[256];           //name of file that will be chosen
    char tempName[256];            //for getting extension
    char *dot;
    int csvCount = 0;                     //number of csv files

    //get largest file
    while( (aDir = readdir(currDir)) != NULL){
        //check if prefix and suffix are 'movies_' and '.csv' respectively
        if( (strncmp(PREFIX, aDir->d_name, strlen(PREFIX))) == 0){
            memset(tempName, '\0', sizeof(tempName));
            strcpy(tempName, aDir->d_name);
            if( (dot = strrchr(tempName, '.')) != NULL){     //get string starting at rightmost '.' Found at https://stackoverflow.com/questions/4849986/how-can-i-check-the-file-extensions-in-c
                if(strcmp(dot, SUFFIX) == 0){
                    csvCount++;
                    //get metadata
                    stat(aDir->d_name, &dirStat);
                    //get size of dirent and compare to max size
                    if(dirStat.st_size >= maxSize){
                        //update maxsize
                        maxSize = dirStat.st_size;
                        //clear buffer
                        //update name of file to process
                        memset(entryName, '\0', sizeof(entryName));
                        strcpy(entryName, aDir->d_name);
                    }
                }
            }
        }

    }
    closedir(currDir);
    if(csvCount == 0){
        printf("\nNo files found to process\n");
    }
    else{
        //create linked list of movie structs from file
        movie *list = processFile(entryName);
        //make directory
        srand(time(NULL));                //initialize random num generator
        int randomNum = rand() % 100000;  //random num from 0-99999
        char numstr[6];
        sprintf(numstr, "%d", randomNum);       //make the random num a string
        char newDirName[256] = "somatiss.movies.";
        strcat(newDirName, numstr);
        if(mkdir(newDirName, 0740) == 0){   //owner has r, w, x and group has r
            printf("\nCreated Directory with name %s\n", newDirName);
            //call function to handle txt file writing
            writeFiles(list, newDirName);   
        }
    }
}

//process the smallest file in the working directory
void smallestFile(){
    //loop through current directory, adapted from the 3_4  & 3_5 directory programs from the directories module
    DIR *currDir = opendir(".");    //current directory
    struct dirent *aDir;             //current directory entry
    struct stat dirStat;           //struct for dir metadata
    off_t minSize = __INT_MAX__;              //start min file size at int max
    char entryName[256];           //name of file that will be chosen
    char tempName[256];            //for getting extension
    char *dot;
    int csvCount = 0;                     //number of csv files

    //get largest file
    while( (aDir = readdir(currDir)) != NULL){
        //check if prefix and suffix are 'movies_' and '.csv' respectively
        if( (strncmp(PREFIX, aDir->d_name, strlen(PREFIX))) == 0){
            memset(tempName, '\0', sizeof(tempName));
            strcpy(tempName, aDir->d_name);
            if( (dot = strrchr(tempName, '.')) != NULL){     //get string starting at rightmost '.' Found at https://stackoverflow.com/questions/4849986/how-can-i-check-the-file-extensions-in-c
                if(strcmp(dot, SUFFIX) == 0){
                    csvCount++;
                    //get metadata
                    stat(aDir->d_name, &dirStat);
                    //get size of dirent and compare to max size
                    if(dirStat.st_size <= minSize){
                        //update minsize
                        minSize = dirStat.st_size;
                        //clear buffer
                        //update name of file to process
                        memset(entryName, '\0', sizeof(entryName));
                        strcpy(entryName, aDir->d_name);
                    }
                }
            }
        }

    }
    closedir(currDir);

    if(csvCount == 0){
        printf("\nNo files found to process\n");
    }
    else{
        //create linked list of movie structs from file
        movie *list = processFile(entryName);
        //make directory
        srand(time(NULL));                //initialize random num generator
        int randomNum = rand() % 100000;  //random num from 0-99999
        char numstr[6];
        sprintf(numstr, "%d", randomNum);       //make the random num a string
        char newDirName[256] = "somatiss.movies.";
        strcat(newDirName, numstr);
        if(mkdir(newDirName, 0740) == 0){   //owner has r, w, x and group has r
            printf("\nCreated Directory with name %s\n", newDirName);
            //call function to handle txt file writing
            writeFiles(list, newDirName);   
        }
    }
}

//let the user specify a file to process, return a msg if there is no file in directoy with given name
void chooseFile(){
    //loop through current directory, adapted from the 3_4  & 3_5 directory programs from the directories module
    DIR *currDir = opendir(".");    //current directory
    struct dirent *aDir;             //current directory entry
    struct stat dirStat;           //struct for dir metadata
    char entryName[256];           //name of file that will be chosen
    found = 0;
    //get file
    printf("\nEnter the complete file name: ");
    fgets(entryName, 256, stdin);
    entryName[strlen(entryName)-1] = '\0';      //remove \n from input
    //search for name in directory
    while( (aDir = readdir(currDir)) != NULL){
        //check for match
        if(strcmp(aDir->d_name, entryName) == 0){       //match found
            found = 1;
            closedir(currDir);
            //create linked list of movie structs from file
            //memset(entryName, '\0', sizeof(entryName));
        }
        
    }
    if(found == 1){
        movie *list = processFile(entryName);
        //make directory
        srand(time(NULL));                //initialize random num generator
        int randomNum = rand() % 100000;  //random num from 0-99999
        char numstr[6];
        sprintf(numstr, "%d", randomNum);       //make the random num a string
        char newDirName[256] = "somatiss.movies.";
        strcat(newDirName, numstr);
        if(mkdir(newDirName, 0740) == 0){   //owner has r, w, x and group has r
            printf("\nCreated Directory with name %s\n", newDirName);
            //call function to handle txt file writing
            writeFiles(list, newDirName);   
        }
    }
    if(found == 0){     //file never found
        printf("\nError: Could not find file '%s'", entryName);
    }
}


int main(){
    int input1 = 0;     //main menu
    int input2 = 0;     //sub menu
    while(input1 != 2){
        int exitLoop = 0;   //keeps track of when the looping switch should stop
        printf("\n1. Select file to process\n2. Exit the program\n");
        printf("\nEnter a choice of 1 or 2: ");
        scanf("%d", &input1);
        while(('\n' != getchar()));

        //use a looping switch statement to continually print the 3 file options, with the functionality to loop back around to the 3 file options or go back to the main menu, depending on the case.
        while(exitLoop == 0){
            switch(input1){
                case 1:
                    printf("\nWhich file do you want to process?\n");
                    printf("\nEnter 1 to pick the largest file\nEnter 2 to pick the smallest file\nEnter 3 to specify the name of a file\n");
                    printf("\nEnter a choice of 1 2 or 3: ");
                    scanf("%d", &input2);
                    while(('\n' != getchar()));

                    switch(input2){
                        case 1:
                            largestFile();
                            exitLoop = 1;
                            break;
                        case 2:
                            smallestFile();
                            exitLoop = 1;
                            break;
                        case 3:
                            chooseFile();
                            if(found == 1){
                                exitLoop = 1;
                                break;
                            }
                            else{
                                printf(" Try Again\n");
                                continue;       //use continue to loop back to the 3 file options instead of going back to main menu
                            }
                            
                            break;
                        default:
                            printf("\nError: Enter a choice of either 1 2 or 3\n");
                    }

                    break;
                case 2:
                    exit(0);
                default:
                    printf("\nError: Enter a choice of either 1 or 2\n");
                    exitLoop = 1;
            }
        }
    }
    return EXIT_SUCCESS;
}