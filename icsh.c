/* ICCS227: Project 1: icsh
 * Name: Pearploy Chaicharoensin
 * StudentID: 6381278
 * Tag : 0.6.0
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>


#define MAX_CMD_BUFFER 255
#define MAX_LINE_LENGTH 255 

/* Global variables */
pid_t foregroundJob = 0; //keep track of foregroundJob & process ID
char** prevPrevBufferArr;
int isRunningBG = 0 ;
int currentJobID = 1; //the one that is working on
int jobID = 0; //ID starts at 0 and so on //count

/* Functions must be declared before being called inside other functions */
void command(char** , char** );
void readScripts(char*);

typedef struct BG
{
    int jobID;
    char* processStatus;
    char* commands;
    int pid;
} job;

job jobList[1000]; //limit 1000 jobs only

char** toTokens(char* buffer) {
    char** toReturn = malloc(MAX_CMD_BUFFER * sizeof(char*));
    char* token = strtok(buffer, " \t\n");
    int i = 0;
    while (token != NULL) {
        toReturn[i] = token;
        if(strcmp(token,"&")){
            isRunningBG++;
        }
        token = strtok(NULL, " \t\n");
        i++;
    }
    toReturn[i] = NULL;
    return toReturn;
}

char** getLines(char* buffer) {
    char** toReturn = malloc(MAX_CMD_BUFFER * sizeof(char*));
    char* token = strtok(buffer, "\n");
    int i = 0;
    while (token != NULL) {
        toReturn[i] = token;
        token = strtok(NULL, "\n");
        i++;
    }
    toReturn[i] = NULL;
    return toReturn;
}

char** copyTokens(char** tokens) {
    char** toReturn = malloc(MAX_CMD_BUFFER * sizeof(char*));
    int i = 0;
    while (tokens[i] != NULL) {
        int len = strlen(tokens[i]);
        toReturn[i] = malloc((len + 1) * sizeof(char));
        strcpy(toReturn[i], tokens[i]);
        i++;
    }
    toReturn[i] = NULL;
    return toReturn;
}

void printToken(char** token, int start) {
    for (int i = start; token[i] != NULL; i++) {
        printf("%s ", token[i]);
    }
    printf("\n");
}

char* tokenStr(char** token, int from) {
    char* toReturn = calloc(MAX_LINE_LENGTH, sizeof(char));
    strcpy(toReturn, "");
    for (int i = from; token[i] != NULL; i++) {
        strcat(toReturn, token[i]);
        strcat(toReturn, " ");
    }
    return toReturn;
}

/* I/O Redirection */
void redir(char** args){
    int in;
    int out;
    char* fileName;
    char buffer[1024];
    int i = 0 ;
    while ( args[i] != NULL )
    {
        if (strcmp(args[i],"<") == 0 && args[i+1] != NULL) {
            in = open(args[i+1],O_RDONLY); 
            if (in <= 0) {
                fprintf (stderr, "Couldn't open a file\n");
                exit (1);
            } 
            fileName = args[i+1];
            dup2(in,0);
            close(in);
        }else if (strcmp(args[i],">") == 0 && args[i+1] != NULL) {
            out = open(args[i+1],O_TRUNC | O_CREAT | O_WRONLY, 0666); 
            if (out <= 0) {
                fprintf (stderr, "Couldn't open a file\n");
                exit(1);
            } 
            dup2(out,1);
            close(out);
        }
        i++;
    } 

    /* Read from inputFile write on Buffer and print out to outputFile */
    size_t got = fread (buffer, 1, 1024, stdin); 
    while (got > 0)
    {
        readScripts(fileName);
        got = fread (buffer, 1, 1024, stdin); 
    }
    fflush(stdout);
    return;
}


/* Handle command that already exist */
void externalRunning(char** args){ //commandArr
    int status;
    pid_t pid;
    /* Create a process space for the ls */
    if ((pid=fork()) < 0)
    {
        perror ("Fork failed");
        exit(1);
    }
    if (!pid)
    {
    /* This is the child, so execute the ls */

        status = execvp (args[0], args);
        if (status < 0) {
            printf("bad command\n");
        }
        exit(1);
    }

    if (pid)
    {
        /* 
         * We're in the parent; let's wait for the child to finish
         */
        foregroundJob = pid; //update 
        waitpid(pid, &status, 0);
        foregroundJob = 0;
    }
}

void printJobList(int from){
    printf("");
    char sign = '-';
    for (int i = from; i <= jobID; i++)
    {          
        printf("[%d]%c %s                 %s\n",i,sign,jobList[i].processStatus,jobList[i].commands); 
        sign = '+';
    }
}
    

void command(char** current, char** prev) {
    /* Turns prev to a string */
    char* prev_output = tokenStr(prev,1);
    char* second_last_output = tokenStr(prevPrevBufferArr,1);
    char* current_input = tokenStr(current,0);
    /* !! */
    if (strcmp(current[0], "!!") == 0 && current[1] == NULL) {
        if (strcmp(prev_output, "") != 0) {
            printf("%s\n", prev_output);
        } else {
            printf("No previous command\n");
        }
    /* !!!! Extra Feature: show second last command */ 
    } else if (strcmp(current[0], "!!!!") == 0 && current[1] == NULL) {
        if (strcmp(second_last_output, "") != 0) {
            printf("%s\n", second_last_output);
        } else {
            printf("No second last command\n");
        }
    } else if (strcmp(current[0], "jobs") == 0 && current[1] == NULL) {
        printJobList(currentJobID);
    } else {
        
        if (strcmp(current[0], "echo") == 0 && current[1] != NULL) {
            /* echo $? */
            if (strcmp(current[1], "$?") == 0 && current[2] == NULL) {
                 printf("%d\n", 0);
            } 
            else if (strcmp(current[1], "$?") == 0 && current[2] != NULL) {
                printToken(current, 1);
            }
            /* echo */
            else if (strcmp(current[1], "$?") != 0){
                printToken(current, 1);
            }           
        /* exit */
        } else if (strcmp(current[0], "exit") == 0) {
            if (current[1] == NULL) {
                printf("$ echo $?\n0\n$\n");
                exit(0);
            }
            else if (current[2] == NULL)
            {
                printf("$ echo $?\n%s\n$\n",current[1]);
                int num = atoi(current[1]);
                if (num > 255) {
                    num = num >> 8;
                }
                exit(num);
            }
        } else {
            int isRedir = 0;
            int isJobBG = 0;
            int i = 0;
            while (current[i] != NULL) {
                if (strcmp(current[i], "<") == 0 || strcmp(current[i], ">") == 0) {
                    isRedir = 1;
                    break;
                } else if (strcmp(current[i], "&") == 0 ) {
                    isJobBG = 1;
                    current[i] = NULL; //remove last
                    break;
                }
                i++;
            }
            if (isRedir) {
                redir(current);
            } else {
                if(isJobBG) {
                    pid_t pid;
                    if ((pid=fork()) < 0){
                        perror ("Fork failed");
                        exit(1);
                    } else if (pid == 0){ //child
                        externalRunning(current);
                        exit(1);
                    } else { //parent
                        // Implement jobList
                        jobID++;
                        jobList[jobID].jobID = jobID;
                        jobList[jobID].processStatus = "Running";
                        jobList[jobID].commands = current_input;
                        jobList[jobID].pid = pid;
                        printf("[%d] %d\n", jobList[jobID].jobID,jobList[jobID].pid);  //print PID for the current JOB
                    }
                } else {
                    externalRunning(current);
                }
            }
        }
    }
    free(prev_output);
}


//similar to main in tag 0.1.0
void readScripts(char* fileName){
    FILE *file;
	file = fopen(fileName, "r");
    if (file == NULL) { printf("Invalid Filename\n");}
    else {
        char buffer[MAX_CMD_BUFFER];
        char** prevBufferArr = toTokens(buffer); 
        while (fgets(buffer, MAX_LINE_LENGTH, file) != NULL) {
            char** curBufferArr = toTokens(buffer); 
            command(curBufferArr, prevBufferArr);
            prevBufferArr = copyTokens(curBufferArr);
            free(curBufferArr);
        }
        fclose(file);
    }
    return;
}


/* SIGNAL */
void signalHandler(int signum){
    if (signum == SIGINT && foregroundJob) {
        kill(foregroundJob,SIGINT);
        printf("The current foreground job killed\n");
    }
        if (signum == SIGTSTP && foregroundJob) {
        kill(foregroundJob,SIGTSTP);
        printf("The current foreground job suspended\n");
    }
    printf("\n");
}

void signalHandlerSetUP(){
    struct sigaction sa;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = signalHandler;
    /* ctrl+C */
    sigaction(SIGINT, &sa, NULL);
    /* ctrl+Z */
    sigaction(SIGTSTP, &sa, NULL);
}


int main(int arg, char *argv[]) {
    signalHandlerSetUP();
    /* Script */
    if (arg > 1) { 
        readScripts(argv[1]);
    }
    /* User Input */
    else {
        char buffer[MAX_CMD_BUFFER];
        char** prevBufferArr = toTokens(buffer);
        prevPrevBufferArr = toTokens(buffer); 
        printf("Starting IC shell\n");
        while (1) {
            printf("icsh $ ");
            fgets(buffer, MAX_CMD_BUFFER, stdin);
            char** curBufferArr = toTokens(buffer); 
            command(curBufferArr, prevBufferArr);
            prevPrevBufferArr = copyTokens(prevBufferArr);
            prevBufferArr = copyTokens(curBufferArr);
            free(curBufferArr);        
        }
    }
    return 0;
}
