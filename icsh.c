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
char command[maxChar]; // save original command
char* command_before_allDelete; // save previous command to use "!!"
pid_t pid;
int exit_status = 0; // for "echo $?" command

struct jobs {
    int jobNum; // job id
    int pid; //  pid
    char command[maxChar]; // command such as "sleep 50 &"
    int status; // 0 : nothing, 1 : running, 2 : Done, 3 : Stop, 4 : wait
    char symbol; // '+', '-' , or nothing
};

struct jobs jobLst[maxChar]; // save jobs (sleep number)
int job_count; // count size of jobs in jobLst
// char* history[maxChar]; // keep all command history

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

void csymbol() {    // change the last command that was used '+' -> '-' | '-' -> null
    for (int i = 0; i < maxChar; i++) {
        if (jobLst[i].symbol == '+') {
            jobLst[i].symbol = '-';
        }
        else {
            jobLst[i].symbol = ' ';
        }
    }
}

void handlerDeleteJobLst() { // For SIGCHLD
    pid_t pid;
    int stat;
    pid = wait3(&stat, WNOHANG, (struct rusage *)NULL);
    if (pid == 0) return; else if (pid == -1) return;
    else {
        for (int i = 0; i < maxChar; i++) {
            if (jobLst[i].pid == pid) {
                csymbol();
                jobLst[i].status = 2; // 2 = Done
                jobLst[i].symbol = '+';
                job_count--;
            }
        }
    }
}

void printDone() {  // print Done if the process sleep is success
    for (int i = 0; i < maxChar; i++) {
        if (jobLst[i].status == 2) {
            printf("[%d]%c   Done                   %s\n",jobLst[i].jobNum,jobLst[i].symbol,jobLst[i].command);
            jobLst[i].status = 4; // wait (until jobLst is null)
        }
    }
}

// print icsh $ then get input command
void get_command() {
    signal(SIGCHLD, handlerDeleteJobLst);   // to detect sleep processes
    printf("icsh $ ");
    fgets(command,maxChar,stdin);
}

// print all argument
void print_argument(char* input) {
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

void ioRedirection(char** input){ // I/O Redirection "<" and ">"
    int index = 0;
    while(input[index]!= NULL){
        if(input[index] == NULL){break;}
        else if(strcmp(input[index],"<")==0 && input[index+1] != NULL){
            int fds = open(input[index+1],O_RDONLY);
            if(fds < 0){
                perror ("error");
                exit(EXIT_FAILURE);
            }
            else{
                dup2(fds,STDIN_FILENO);
                input[index] = NULL;
                close(fds);
                break;
            }
        }
        else if(strcmp(input[index],">")==0 && input[index+1] != NULL){
            int fds = open(input[index+1],O_WRONLY|O_CREAT);
            if(fds < 0){
                perror ("error!!");
                exit(EXIT_FAILURE);
            }else{
                dup2(fds,STDOUT_FILENO);
                input[index] = NULL;
                input[index+1] = NULL;
                close(fds);
                break;
            }
        }
        else{ index++; }
    }
}

void fg(int num) {  // foreground (fg %num)
    
    int pid = getpid();
    if (job_count == 0) {
        printf("-bash: fg: %%"); 
        printf("%d: no such job\n",num);
    }
    else {
        
    for (int i = 1; i < maxChar; i++) {
        if (jobLst[i].status == 1 && jobLst[i].jobNum == num) { // 1 = running
            csymbol();
            int stat;
            jobLst[i].symbol = '+';
            printf("%s",jobLst[i].command);
            printf("\n");
            tcsetpgrp(STDIN_FILENO, jobLst[i].pid);
            
            kill(jobLst[i].pid, SIGCONT);
            waitpid(jobLst[i].pid, &stat, WUNTRACED);

            if (WIFSIGNALED(stat)) {
                exit_status = WTERMSIG(stat);
            }        
            if (WIFSTOPPED(stat)) {
                jobLst[i].status = 3; // 3 = Done
                printf("[%d]%c   Stopped                %s\n",jobLst[i].jobNum,jobLst[i].symbol,jobLst[i].command);
                tcsetpgrp(STDIN_FILENO,pid);
                exit_status = WSTOPSIG(stat);
                return;
            } 
            if (WIFEXITED(stat)) {
                exit_status = WEXITSTATUS(stat);
            }
        }
        tcsetpgrp(STDIN_FILENO, pid);
    }
    }
}

void bg(int num) {  // background (bg %num)
    if (job_count == 0) {
        printf("-bash: bg: %%"); 
        printf("%d: no such job\n",num);
    }
    else {
        for (int i = 1; i < maxChar; i++) {
            if (jobLst[i].status == 3 && jobLst[i].jobNum == num) { // 3 = stopped
                csymbol();
                int stat;
                jobLst[i].status = 1;
                jobLst[i].symbol = '+';
                printf("[%d]%c %s\n",jobLst[i].jobNum,jobLst[i].symbol,jobLst[i].command);
                kill(jobLst[i].pid,SIGCONT);
            }
        
        }
    }
}

void isemptyJob() { // to check that jobLst is empty or not if empty then job will change to null
    if (job_count == 0) {
        for (int i = 0; i < maxChar; i++) {
            if (jobLst[i].status == 4) { 
                jobLst[i].status = 0;
            }
        }
    }
}

// All command
void all_command(char* cmd) {
    char* copy_command = copyString(cmd); // create copy to protect data of command
    char* split = strtok(copy_command," ");
    trimTrailing(split);

    if (strcmp(split,"echo") == 0) {
        command_before_allDelete = copyString(cmd);
        split = strtok(NULL," ");
        if (strcmp(split,"$?") == 0) {
            printf("%d\n",exit_status);
        }
        else {
            print_argument(split);
            exit_status = 0;
        }
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
            printf("bye\n");
            exit(0);
        }
        else {
            printf("bye\n");
            exit(atoi(split));
        }
    }
    else if (strcmp(split,"fg") == 0) {
        struct jobs* copyjobLst = jobLst;
        split = strtok(NULL," ");
        if (split[0]=='%') {
            split[0]=' ';
            int num = atoi(split);
            fg(num);
        }
        else {
            printf("bad command\n");
        }
    }
    else if (strcmp(split,"bg") == 0) {
        struct jobs* copyjobLst = jobLst;
        split = strtok(NULL," ");
        if (split[0]=='%') {
            split[0]=' ';
            int num = atoi(split);
            bg(num);
        }
        else {
            printf("bad command\n");
        }
    }
    // else if (strcmp(split,"history") == 0) {
    //     printf("");
    // } 
    else if (strcmp(split,"jobs") == 0) {
        if (job_count == 0) {
            printf("");
        }
        else {
            for (int i = 0; i < maxChar; i++) {
                if (jobLst[i].jobNum != 0 && jobLst[i].status == 1) {
                    int count = job_count;
                    char* cpyJob;
                    strcpy(cpyJob,jobLst[i].command);
                    trimTrailing(cpyJob);     
                    printf("[%d]%c   Running                %s\n",jobLst[i].jobNum,jobLst[i].symbol,cpyJob);
                }
                else if (jobLst[i].jobNum != 0 && jobLst[i].status == 3) {
                    printf("[%d]%c   Stopped                %s\n",jobLst[i].jobNum,jobLst[i].symbol,jobLst[i].command);
                }
                else if (jobLst[i].jobNum != 0 && jobLst[i].status == 2) {
                    printf("[%d]%c   Done                   %s\n",jobLst[i].jobNum,jobLst[i].symbol,jobLst[i].command);
                    jobLst[i].status = 4;
                }
            }
        }
    }

    // any other commands such as "ls","vim"
    else {
        command_before_allDelete = copyString(cmd); // copy command for using "!!"
        pid = fork();
        int stat = 0;
        int backgroundCheck = 0; // check background or foreground
        // create string array to use execvp
        int index = 0;
        char* arg[4] = {};
        while(split!=NULL) {
            if (strcmp(split,"&") == 0) {
                backgroundCheck = 1;
                break;
            }
            *(arg+index)=split;
            if (*(*(arg+index)+strlen(arg[index])-1)==' ' 
                || *(*(arg+index)+strlen(arg[index])-1)=='\n') {
                *((arg+index)+strlen(arg[index])-1) = '\0';
            }
            split = strtok(NULL," ");
            index++;
        }
        arg[index] = NULL; // add null at last index of the array
        
        if (pid < 0) {
            printf("fork() failed\n");
            exit(EXIT_FAILURE);
        }
        else if (pid == 0) {
            // child process
            // set up a new process group
            if (setpgid(0,0)<0) {
                perror("error!!");
                exit(EXIT_FAILURE);
            }
            signal(SIGTTOU, SIG_IGN);
            signal(SIGINT,SIG_DFL);
            signal(SIGTSTP,SIG_DFL);

            // "tcsetpgrp" transfer the terminal control to the new process group
            if (backgroundCheck == 0) {
                tcsetpgrp(STDIN_FILENO,getpid());
            }
            
            ioRedirection(arg);
            int ex = execvp(arg[0],arg);
            if (ex == -1) {
                printf("bad command\n");
                kill(getpid(),SIGINT);
            }
        }

        else {
            //parent process
            setpgid(pid, pid);
            if (backgroundCheck == 0) {
                waitpid(pid,&stat,WUNTRACED); 
                signal(SIGTTOU, SIG_IGN);
                tcsetpgrp(STDOUT_FILENO,getpid()); 
                signal(SIGTSTP, SIG_IGN); // ignore ctrl z
                signal(SIGINT, SIG_IGN); // ignore ctrl c  
            
                if (WIFSIGNALED(stat)) {
                    exit_status = WTERMSIG(stat);
                }        
                if (WIFSTOPPED(stat)) {
                    exit_status = WSTOPSIG(stat);
                } 
                if (WIFEXITED(stat)) {
                    exit_status = WEXITSTATUS(stat);
                }
            }
            else {
                int cpy_i;
                for (int i = 1; i < maxChar; i++) {
                    if (jobLst[i].status == 0) {
                        csymbol();
                        jobLst[i].jobNum = i;
                        cpy_i = i;
                        jobLst[i].pid = pid;
                        strcpy(jobLst[i].command,cmd);
                        jobLst[i].status = 1;
                        jobLst[i].symbol = '+';
                        break;
                    }
                }
                job_count++;
                printf("[%i] %i\n", jobLst[cpy_i].jobNum, jobLst[cpy_i].pid);
            }
        }
    }
}

// if command is empty then the program will loop again
void check_Empty_Command() {
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTSTP, SIG_IGN); // ignore ctrl z
    signal(SIGINT, SIG_IGN); // ignore ctrl c
    while (1) {
        printDone();
        isemptyJob();
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