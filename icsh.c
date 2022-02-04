#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>


#define maxChar 250 //Max character of the command
char command[maxChar];
char* command_before_allDelete; // save previous command to use "!!"
pid_t pid;

// To check that command is empty or not
bool is_Empty(char* input) { 
    bool x;
    if(strlen(input) == 0) {
        x = true;           
    }
    else {
        x = false;
    }
    return x;
}

// print icsh $ then get input command
void get_command() {
    printf("icsh $ ");
    fgets(command,maxChar,stdin);
}

// print all argument
void print_argument(char* input) {
    input = strtok(NULL," ");
    if (input == NULL) {
        printf("");
    }
    else {
    printf("%s",input);
    input = strtok(NULL," ");
    while (input != NULL) {
        printf(" %s",input);
        input = strtok(NULL," ");
    }
    printf("\n");
    }
}

// delete space infront of arguments
char* removeLeadingSpaces(char* str)
{
    static char str1[99];
    int count = 0, j, k;
  
    // Iterate String until last
    // leading space character
    while (str[count] == ' ') {
        count++;
    }
  
    // Putting string into another
    // string variable after
    // removing leading white spaces
    for (j = count, k = 0;
         str[j] != '\0'; j++, k++) {
        str1[k] = str[j];
    }
    str1[k] = '\0';
  
    return str1;
}

// delete space behind arguements
void trimTrailing(char * str)
{
    int index, i;
    /* Set default index */
    index = -1;
    /* Find last index of non-white space character */
    i = 0;
    while(str[i] != '\0')
    {
        if(str[i] != ' ' && str[i] != '\t' && str[i] != '\n')
        {
            index= i;
        }
        i++;
    }
    /* Mark next character to last non-white space character as NULL */
    str[index + 1] = '\0';
}

// copy string
char* copyString(char* input) {
    char* output;
    output = (char*)malloc(maxChar);
    strcpy(output, input);
    return (char*)output;
}

// All command
void all_command(char* cmd) {
    char* copy_command = copyString(cmd); // create copy to protect data of command
    char* split = strtok(copy_command," ");
    trimTrailing(split);

    if (strcmp(split,"echo") == 0) {
        command_before_allDelete = copyString(cmd);
        print_argument(split);
    }

    // "!!" command
    else if (strcmp(split,"!!") == 0) {
        if (command_before_allDelete==NULL) {
            printf("");
        }
        else {
            char* copy_command_before_allDelete = copyString(command_before_allDelete);
            char* resplit = strtok(copy_command_before_allDelete," ");
            for (int i = 0; i < strlen(command_before_allDelete); i++) {
                printf("%c", command_before_allDelete[i]);
            }
            printf("\n");
            // recursive to check the previous command
            all_command(command_before_allDelete);
        }
    }

    // "exit" command
    else if (strcmp(split,"exit") == 0) {
        split = strtok(NULL," ");
        if(split==NULL) { // if there is no number then it will loop again
            printf("");
        }
        else {
            printf("bye\n");
            exit(atoi(split));
        }
    }

    // any other commands such as "ls","vim"
    else {
        command_before_allDelete = copyString(cmd); // copy command for using "!!"
        pid = fork();
        // int stat;
        if (pid < 0) {
            printf("fork() failed\n");
        }
        else if (pid == 0) {
            
            // set up a new process group
            if (setpgid(0,0)<0) {
                perror("error!!");
                exit(EXIT_FAILURE);
            }

            // "tcsetpgrp" transfer the terminal control to the new process group
            tcsetpgrp(STDOUT_FILENO,getpid());
            
            // create string array to use execvp
            int index = 0;
            char* arg[4] = {};
            while(split!=NULL) {
                *(arg+index)=split;
                if (*(*(arg+index)+strlen(arg[index])-1)==' ' 
                    || *(*(arg+index)+strlen(arg[index])-1)=='\n') {
                    *((arg+index)+strlen(arg[index])-1) = '\0';
                }
                split = strtok(NULL," ");
                index++;
            }
            arg[index] = NULL; // add null at last index of the array
            
            signal(SIGINT,SIG_DFL);
            signal(SIGTSTP,SIG_DFL);

            int ex = execvp(arg[0],arg);
            if (ex == -1) {
                printf("bad command\n");
            }
        }
        else {
            waitpid(pid,NULL,WUNTRACED);
            tcsetpgrp(STDOUT_FILENO,getpid());            
        }
    }
}

// if command is empty then the program will loop again
void check_Empty_Command() {
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGINT, SIG_IGN);
    while (1) {
        get_command();
        trimTrailing(command);
        if (is_Empty(command)) {
            continue;
        }
        else {
            all_command(command);
        }
    }
}

// "script-mode" read from the file test.sh
void script_Command(char* file) {
    char buff[maxChar];
    FILE *fp = fopen(file, "r");
    if (fp) {
        while(fscanf(fp,"%[^\n]\n", buff)!=EOF) {
            trimTrailing(buff);
            if (is_Empty(buff)) {
                continue;
            }
            else {
                all_command(buff);
            }
        }
    }
}
int main(int* argc, char *argv[]) {
    if (argv[1]) {
        script_Command(argv[1]);
    }
    else {
        printf("Starting IC shell\n");
        check_Empty_Command();
    }
    return 0;
}