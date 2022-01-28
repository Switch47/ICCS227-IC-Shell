#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define maxChar 250 //Max character of the command
char command[maxChar];
char* command_before_allDelete; // save previous command to use "!!"

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
    printf("%s",input);
    input = strtok(NULL," ");
    while (input != NULL) {
        printf(" %s",input);
        input = strtok(NULL," ");
    }
    printf("\n");
}

// delete space
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

    // "echo" command
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
            printf("bye");
            exit(atoi(split));
        }
    }

    // any other command
    else {
        printf("bad command\n");
        // printf("%d",strlen(split));
        // for (int i = 0; i < strlen(split); i++) {
        //     printf("%c\n",split[i]);
        // }
    }
}

// if command is empty then the program will loop again
void check_Empty_Command() {

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