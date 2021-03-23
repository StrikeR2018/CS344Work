#define _GNU_SOURCE
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
//Sample code from Justin Goins was used as the base for this project

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
    printf("\nProcessed file %s and parsed data for %d movies\n", filePath, count);

    //free and close stuff
    free(currLine);
    fclose(movieFile);
    
    return head;
}

//prints a movie struct
void printMovieInfo(movie *aMovie){
    printf("%s, %s, %s, %s\n", aMovie->Title, aMovie->Year, aMovie->Languages, aMovie->Rating);
}

//print the linked list of movie structs
void printMovieList(movie *list){
    while(list != NULL){
        printMovieInfo(list);
        list = list->next;
    }
}

void moviesInYear(movie *list){
    printf("\nEnter the year: ");
    //buffer
    char buff[10];
    fgets(buff, 5, stdin);
    //counter for structs with matching year
    int counter = 0;
    //loop through list, checking int value of year token against int value of the user input
    while(list != NULL){
        if(atoi(buff) == atoi(list->Year)){
            printf("%s\n", list->Title);
            counter++;
        }
        list = list->next;
    }
    if(counter == 0){
        printf("No data about movies from the year %s\n", buff);
    }
    
    
}

//utiltiy function for highestRated to check if a year has already been looked at
int isIn(int year, int visited[]){
    int i;
    for(i = 0; i < count+1; ++i){
        if(visited[i] == year){
            return 1;
        }
    }
    return 0;

}

void highestRated(movie *list){
    //visited array for the years that have already been evaluated. Will have enough spaces for the number of structs
    int visited[count+1];
    int nextIndex = 0;      //next available index in visited
    //loop through the list, at each line, loop through the list again, checking for other structs that have the same year. If they do, compare rating
    while(list != NULL){
        movie *currMovie = head;
        //check if current has been evaluated
        if(isIn(atoi(list->Year), visited) == 1){
            list = list->next;
            continue;   //if the current struct year has already been evaluated, skip this iteration
        }else{
            visited[nextIndex] = atoi(list->Year);      //add to visited
            nextIndex++;
            char *ptr = NULL;   //pointer for strtod
            double currentMax = strtod(list->Rating, &ptr);    //variable to store the current max for given year
            //values to be updated if needed then printed
            char* yearToPrint = list->Year;
            char* ratingToPrint = list->Rating;
            char* titleToPrint = list->Title;
            while(currMovie != NULL){    //maybe keep a counter = #of structs to see when to terminate this while loop
                //check if years match
                if(atoi(yearToPrint) == atoi(currMovie->Year)){
                    char *ptr2 = NULL;
                    double currentRating = strtod(currMovie->Rating, &ptr2);
                    if(currentRating > currentMax){
                        currentMax = currentRating;
                        titleToPrint = currMovie->Title;
                        currMovie = currMovie->next;
                    }else{
                        currMovie = currMovie->next;
                    }
                }else{   //if years don't match move on
                    currMovie = currMovie->next;
                }
                
            }
            //print the values
        printf("%s %.1f %s\n", yearToPrint, currentMax, titleToPrint);
        list = list->next;
        }
    }
}
//utility function for moviesInLang to find the amount of languages in a particular struct
int langCount(movie *list){
    int i;
    int semis = 0;       //amount of semicolons in the string
    for(i = 0; i<strlen(list->Languages); ++i){
        if(list->Languages[i] == ';'){
            semis++;
        }
    }
    return semis;      //semis+1 = the number of languages, we want to loop through amount of languages - 1,
}

void moviesInLang(movie *list){
    int counter = 0;
    printf("\nEnter a Language: ");
    //buffer, input should be < 20 chars
    char buff[20];  
    fgets(buff, 20, stdin);
    buff[strlen(buff)-1] = '\0';    //remove \n from input
    char *saveptr;
    int langs = 0;
    //loop through the list, extract language tokens from each struct, and check if they match user input
    while(list != NULL){
        //get # of semis in language
        langs = langCount(list);
        char *temp;     //use a temp to store the value of list->languages and use that for tokenizing. Otherwise, the acutal language fields get changed, which breaks future calls to this function.
        temp = calloc(strlen(list->Languages)+1, sizeof(char));
        strcpy(temp, list->Languages);
        //strip initial '['  & ']' off of languages
        char *stripToken = strtok_r(temp, "[", &saveptr);
        stripToken = strtok_r(stripToken, "]", &saveptr);
        //get first language
        char *langToken = strtok_r(stripToken, ";", &saveptr);
        //check if first is match
        if(strcmp(buff, langToken) == 0){
            printf("%s %s\n", list->Year, list->Title);
            counter++;
        }
        else{
            //loop through amount of languages - 1
            int i;
            for(i = 0; i<langs; ++i){
                //get next language
                langToken = strtok_r(NULL, ";", &saveptr);
                //check for match
                if(strcmp(buff, langToken) == 0){
                    printf("%s %s\n", list->Year, list->Title);
                    counter++;
                    break;
                }
            }

        }
    
        list = list->next;
    }

    //no movies check
    if(counter == 0){
        printf("No movies found for specified Language\n");
    }
}

int main(int argc, char *argv[]){
    if (argc < 2){
        printf("You must provide the name of the file to process\n");
        printf("Example usage: ./students student_info1.txt\n");
        return EXIT_FAILURE;
    }

    movie *list = processFile(argv[1]);
    int input = 0;
    while(input != 4){

        //display menu    
        printf("\n1. Show movies released in the specified year\n2. Show highest rated movie for each year\n3. Show the title and year of release of all movies in a specific language\n4. Exit from the program\n");
        printf("\nEnter a choice from 1 to 4:");

        scanf("%d", &input);
        while(('\n' != getchar()));

        switch(input){
            case 1:
                moviesInYear(list);
                break;
            case 2:
                highestRated(list);
                break;
            case 3:
                moviesInLang(list);
                break;
            case 4:
                exit(0);    //exits the program
            default:
                printf("\nError: Please Enter 1, 2, 3, or 4.\n");
        }
    }
    return EXIT_SUCCESS;
}
