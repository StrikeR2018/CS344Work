#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <limits.h>
#define MAXCHARS 2048
#define MAXARGS 512

int lastExitStatus = 0;
int lastTermStatus = 0;
int wasTerm = 0;    //indicates that there was a termination
int foregroundOnly = 0;     //toggle for foreground only mode. initially set to 0 or 'off'

//a command line entry
typedef struct process{
    char args[MAXCHARS];    //array that stores the command line entry
    int isBG;   //1 if process is background, else foreground
    int outRedir;               //if the output is being redirected
    int inRedir;                //if input is being redirected
    char inputFile[256];        //input to be redirected to
    char outputFile[256];       //output to be redirected to
    int exitStatus;
    char *parsedArgs[MAXARGS];
}command;

//background pid array
pid_t pids[100];
//child status array for pid array
int statusArr[100];
//next available index for pids and statusArr
int nextIndex = 0;
//count of bg process
int bgCount = 0;

//utiltiy function to check if background processes are running and print a msg when they are done
void bgProcCheck(pid_t pids[], int statusArr[]){
    //loop through list and check status
    int i;
    for(i = 0; i < bgCount; ++i){
        if(pids[i] != -5){
            int childStatus = statusArr[i];
            pids[i] = waitpid(-1, &childStatus, WNOHANG);
            if(pids[i] > 0){
                printf("background process %d is done: ", pids[i]);
                pids[i] = -5;    //indicating that the process is done
                ++i;
                if(WIFEXITED(childStatus)){
                    lastExitStatus = WEXITSTATUS(childStatus);       //update childstatus
                    printf("exit value %d\n", lastExitStatus);
                    fflush(stdout);
                }
                if(WIFSIGNALED(childStatus)){
                    lastExitStatus = WTERMSIG(childStatus);
                    printf("terminated by signal %d\n", lastExitStatus);
                }
            }
            else{
            //process has not terminated yet
                ++i;
            }
        }
    }
}
//kills all currently running background processes
void killProcesses(pid_t pids[]){
    int i;
    for(i = 0; i<bgCount; ++i){
        if(pids[i] != -5){
            //weird trick I found to discard the 'terminated' msg that the kill function prints. Fount at https://stackoverflow.com/questions/47494267/remove-terminal-mesage-after-kill-system-call
            char str[256];
            sprintf(str, "kill -SIGTERM %d >> /dev/null 2>&1",pids[i]); 
            system(str);
        }
    }
}

//utility function that initializes all values in parsedargs to NULL straight from the get go. 
//Only assigning the element after the last cmd argument causes weird bug where parsedArgs[0] gets set to an empty string (wtf?)
void makeALLNULL(command *currCommand){
    int i;
    for(i=0; i<512; ++i){
		currCommand->parsedArgs[i] = NULL;
	}
}

//utility function to remove the newline character from the args list
void removeNewLine(command *currCommand){
    int i;
    for(i = 0; i < strlen(currCommand->args); ++i){
        if(currCommand->args[i] == '\n'){
            currCommand->args[i] = '\0';
            break;
        }
    }
}

//expand any instance of '$$' to the process id of the shell
void expandVar(char *args, command *currCommand){
    //result string after variable expansion
    char result[2048];
    char *insertPoint = &result[0];
    //search for '$$' in input, adapted from https://stackoverflow.com/questions/32413667/replace-all-occurrences-of-a-substring-in-a-string-in-c
    int pid = getpid();
    char pidstr[20];
    sprintf(pidstr, "%d", pid);
    size_t pidstrLen = strlen(pidstr);
    while(1){
        char *p = strstr(args, "$$");   //find occurence of $$

        if(p == NULL){
            strcpy(insertPoint, args);      //copy the rest of the string after there are no more occurences of $$
            break;
        }

        //copy string before $$
        memcpy(insertPoint, args, p - args);
        insertPoint += p - args;

        //copy pid into result
        memcpy(insertPoint, pidstr, pidstrLen);
        insertPoint += pidstrLen;

        //update pointers
        args = p + strlen("$$");
    }
    //copy result back to, specifically, currCommand->args, not just args, hence why the struct is a parameter to the function as well as the args
    strcpy(currCommand->args, result);     
    
}

//parse args string into an array of commands separated by spaces
void parseLine(command *currCommand){
    char s[2] = " ";    //space
    char *saveptr;        //for strtok_r
    char *token = strtok_r(currCommand->args, " ", &saveptr);       //get first token
    //loop through args, each time a new token is created, add it to parsed args
    int i = 0;
    while(token != NULL){
        //check for i/o redirection
        if(strcmp(token, ">") == 0){
            currCommand->outRedir = 1;
            char *temptoken = token;
            temptoken = strtok_r(NULL, " ", &saveptr);      //get next token
            strcpy(currCommand->outputFile, temptoken);     //whatever is after > is the output
            token = strtok_r(NULL, " ", &saveptr);          //continue with loop
        }
        else if(strcmp(token, "<") == 0){
            currCommand->inRedir = 1;
            char *temptoken = token;
            temptoken = strtok_r(NULL, " ", &saveptr);      //get next token
            strcpy(currCommand->inputFile, temptoken);     //whatever is after > is the output
            token = strtok_r(NULL, " ", &saveptr);          //continue with loop
        }
        else{
            currCommand->parsedArgs[i] = calloc(strlen(token)+1, sizeof(char));
            strcpy(currCommand->parsedArgs[i], token);
            token = strtok_r(NULL, " ", &saveptr);
            ++i;
        }
    }
    //check for background command, if we know & is the last thing entered, we set isBG then remove & from the acutal list of commands
    if(strcmp(currCommand->parsedArgs[i-1], "&") == 0){
        currCommand->isBG = 1;
        currCommand->parsedArgs[i-1] = NULL;
    }
}

//function to execute non built-in commands in the foreground. Adapted from the pages in modules 4 & 5
void foregroundCmd(command *currCommand, struct sigaction SIGINT_action){
    wasTerm = 0;        //reset wasterm
    int childStatus;    //return status of child process
    pid_t spawnPid = fork();    //fork process

    //cases for return value of fork
    switch(spawnPid){
        case -1:
            perror("fork error\n");
            fflush(stdout);
            exit(1);
            break;
        case 0:
            //allow foreground processes to be terminated with ctrl c
            SIGINT_action.sa_handler = SIG_DFL;
            sigaction(SIGINT, &SIGINT_action, NULL);
            //handle input redirection case
            if(currCommand->inRedir == 1){
                int sourceFD = open(currCommand->inputFile, O_RDONLY);      //open file for read only
                //make sure file is opened successfully
                if(sourceFD == -1){
                    fprintf(stderr, "cannot open %s for input\n", currCommand->inputFile);
                    fflush(stdout);
                    exit(1);
                }
                //redirect stdin to source file
                int inResult = dup2(sourceFD, 0);
                //check for dup2 error
                if(inResult == -1){
                    perror("source dup2 error\n");
                    fflush(stdout);
                    exit(2);
                }
                //trigger the file to close on exec
                fcntl(sourceFD, F_SETFD, FD_CLOEXEC);
            }

            //handle output redirection case
            if(currCommand->outRedir == 1){
                int targetFD = open(currCommand->outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);   //open the file
                //make sure file is opened successfully
                if(targetFD == -1){
                    fprintf(stderr, "cannot open %s for output\n", currCommand->outputFile);
                    lastExitStatus = 1;
                    exit(1);
                }
                //redirect stdout to targetFD
                int outResult = dup2(targetFD, 1);
                if(outResult == -1){
                    perror("target dup2 error\n");
                    lastExitStatus = 1;
                    exit(2);
                }
                fcntl(targetFD, F_SETFD, FD_CLOEXEC);

            }
            //execute the process using execvp
            execvp(currCommand->parsedArgs[0], currCommand->parsedArgs);
            //if execvp returns due to error
            fprintf(stderr, "%s: no such file or directory\n", currCommand->parsedArgs[0]);
            exit(2);
            break;
        default:
            //wait for child termination
            spawnPid = waitpid(spawnPid, &childStatus, 0);
            //get exit status
            if(WIFEXITED(childStatus)){
                lastExitStatus = WEXITSTATUS(childStatus);       //update childstatus
            }
            if(WIFSIGNALED(childStatus)){
                //print msg indicating signal termination
                printf("terminated by signal %d\n", childStatus);
                wasTerm = 1;
                lastTermStatus = WTERMSIG(childStatus);
            }
            break;
    }
}
//execute non built in cmds in background
void backgroundCmd(command *currCommand){
    wasTerm = 0;        //reset wasterm
    int childStatus;    //return status of child process
    pid_t spawnPid = fork();    //fork process
    pid_t spawnPid2;

    //cases for return value of fork
    switch(spawnPid){
        case -1:
            perror("fork error\n");
            fflush(stdout);
            exit(1);
            break;
        case 0:
            //handle input redirection case
            if(currCommand->inRedir == 1){
                int sourceFD = open(currCommand->inputFile, O_RDONLY);      //open file for read only
                //make sure file is opened successfully
                if(sourceFD == -1){
                    fprintf(stderr, "cannot open %s for input\n", currCommand->inputFile);
                    fflush(stdout);
                    exit(1);
                }
                //redirect stdin to source file
                int inResult = dup2(sourceFD, 0);
                //check for dup2 error
                if(inResult == -1){
                    perror("source dup2 error\n");
                    exit(2);
                }
                fcntl(sourceFD, F_SETFD, FD_CLOEXEC);
            }

            //handle output redirection case
            if(currCommand->outRedir == 1){
                int targetFD = open(currCommand->outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);   //open the file
                //make sure file is opened successfully
                if(targetFD == -1){
                    fprintf(stderr, "cannot open %s for output\n", currCommand->outputFile);
                    fflush(stdout);
                    exit(1);
                }
                //redirect stdout to targetFD
                int outResult = dup2(targetFD, 1);
                if(outResult == -1){
                    perror("target dup2 error\n");
                    fflush(stdout);
                    exit(2);
                }
                fcntl(targetFD, F_SETFD, FD_CLOEXEC);
            }

            //if no i/O redirection specified
            if(currCommand->inRedir != 1){
                int sourceFD = open("/dev/null", O_RDONLY);
                if(sourceFD == -1){
                    fprintf(stderr, "cannot open /dev/null for input\n");
                    fflush(stdout);
                    exit(1);
                }
                int inResult = dup2(sourceFD, 0);
                if(inResult == -1){
                    perror("source dup2 error\n");
                    exit(2);
                }
                fcntl(sourceFD, F_SETFD, FD_CLOEXEC);
            }
            if(currCommand->outRedir != 1){
                int targetFD = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0644);   //open the file
                //make sure file is opened successfully
                if(targetFD == -1){
                    fprintf(stderr, "cannot open /dev/null for output\n");
                    fflush(stdout);
                    exit(1);
                }
                //redirect stdout to targetFD
                int outResult = dup2(targetFD, 1);
                if(outResult == -1){
                    perror("target dup2 error\n");
                    fflush(stdout);
                    exit(2);
                }
                fcntl(targetFD, F_SETFD, FD_CLOEXEC);
            }

            //execute the process using execvp
            execvp(currCommand->parsedArgs[0], currCommand->parsedArgs);
            //if execvp returns due to error
            fprintf(stderr, "%s: no such file or directory\n", currCommand->parsedArgs[0]);
            fflush(stdout);
            exit(2);
            break;
        default:
            //don't wait for child termination, just print a msg about processes pid
            spawnPid2 = waitpid(spawnPid, &childStatus, WNOHANG);
            printf("background pid is: %d\n", spawnPid);
            ++bgCount;
            //add to pid array
            pids[nextIndex] = spawnPid;
            statusArr[nextIndex] = childStatus;
            ++nextIndex;
            bgProcCheck(pids, statusArr);
            fflush(stdout);
            break;
    }

}
//function to toggle foreground only mode, adapted from examples in module 5
void SIGTSTPToggle(int signo){
    //check status, and write appropriate messages
    if(foregroundOnly == 0){
        char *message = "\nEntering foreground-only mode (& is now ignored)\n";
        write(1, message, 50);      //number of bits to write = 49 = number of chars in message + 1. the first arg being 1 indicates stdout
        fflush(stdout);
        foregroundOnly = 1;
    }
    else{
        //its currently set to 1
        char *message = "\nExiting foreground-only mode\n";
        write(1, message, 30);
        fflush(stdout);
        foregroundOnly = 0;
    }
}

int main(){

    while(1){
        //create struct
        command *currCommand = malloc(sizeof(command));
        makeALLNULL(currCommand);
        bgProcCheck(pids, statusArr);

        //handling for SIGINT, adapted from the examples in module 5
        struct sigaction SIGINT_action = {0};
        SIGINT_action.sa_handler = SIG_IGN;     //ignore signal, since sigint needs to be ignored
        sigfillset(&SIGINT_action.sa_mask);
        SIGINT_action.sa_flags = 0;
        sigaction(SIGINT, &SIGINT_action, NULL);

        //handling for SIGSTP
        struct sigaction SIGTSTP_action = {0};
        SIGTSTP_action.sa_handler = SIGTSTPToggle;
        sigfillset(&SIGTSTP_action.sa_mask);
        SIGTSTP_action.sa_flags = 0;
        sigaction(SIGTSTP, &SIGTSTP_action, NULL);

        printf(": ");       //start of line
        fflush(stdout);
        fgets(currCommand->args, 2048, stdin);       //read up to 2048 chars
        removeNewLine(currCommand);    //remove newline

        //check if char limit exceeded, if so, print error and keep looping
        if(strlen(currCommand->args) > MAXCHARS){
            printf("Error: max char limit of 2048 exceeded.");
            fflush(stdout);     //flush output buffer
            continue;
        }

        //continue on without doing anything if the user presses enter or if the first char entered is '#'
        if(currCommand->args[0] == '\0' || currCommand->args[0] == '#'){
            fflush(stdout);     //flushing output buffer
            continue;           //skip this iteration
        }

        //variable expansion for $$
        if(strstr(currCommand->args, "$$") != NULL){         //check if $$ appears in the string
            expandVar(currCommand->args, currCommand);
        }

        //parse input into array of words
        parseLine(currCommand);

        // //status command, commands that require exec() functions will update the value of lastExitStatus
        if(strcmp(currCommand->parsedArgs[0], "status") == 0){
            //check if last cmd was killed by term signal
            if(wasTerm == 1){
                printf("terminated by signal %d\n", lastTermStatus);
                fflush(stdout);
            }
            else{
                printf("exit value %d\n", lastExitStatus);
                fflush(stdout);
            }
        }
        
        // //exit command, may need to update to kill all running processes
        else if(strcmp(currCommand->parsedArgs[0], "exit") == 0){
            killProcesses(pids);
            exit(0);
        }

        // //cd command
        else if(strcmp(currCommand->parsedArgs[0], "cd") == 0){
            //dir in home environment variable
            char *homeDir = getenv("HOME");
            //buffer for cwd
            char cwd[PATH_MAX];

            //if there was no argument passed to cd, cd to home environment directory
            if(currCommand->parsedArgs[1] == '\0'){
                if(chdir(homeDir) == 0){
                    getcwd(cwd, sizeof(cwd));   //delete this maybe
                    fflush(stdout);
                }else{
                    perror("chdir() failure, directory not found");
                    fflush(stdout);     //not sure if needed here
                }
            }
            //if there was one argument, a path, specified in the shell
            else if(currCommand->parsedArgs[1] != '\0'){
                if(chdir(currCommand->parsedArgs[1]) == 0){
                    getcwd(cwd, sizeof(cwd));
                    fflush(stdout);
                }
                else{
                    perror("chdir() failure, directory not found");
                    fflush(stdout);
                }
            }
        }
        //execute built in command
        else{
            if(currCommand->isBG == 1 && foregroundOnly == 0){
                backgroundCmd(currCommand);
            }
            else{
                foregroundCmd(currCommand, SIGINT_action);
            }
        }
        

    }
    
    return EXIT_SUCCESS;
}