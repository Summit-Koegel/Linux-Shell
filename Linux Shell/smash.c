#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ctype.h>

void specialProcessHelper(char **args, int num_args);

void errorHandler(){
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message)); 
}

void changeDir(char **args, int num_args, int i){

    if(chdir(args[i+1]) != -1){
        ;
    }
    
    else{
        errorHandler();
    }
    
}

void printWD(char **args, int num_args, int i){
    char* buffer = NULL;
    size_t size = 0;

    if(getcwd(buffer, size) != NULL){
        printf("%s\n", getcwd(buffer, size));
    }
    

    else{
        errorHandler();
    }
}

int forkRedirect(char **args, int num_args, int i){
    //printf("Starting from %d\n", getpid());
    int rc = fork();

    if(rc == 0){
        //inside child process
        // printf("Exiting from child, my id is %d and rc is %d\n", getpid(), rc);
        int exec_rc = execv(args[0], args);
        errorHandler();
        // TODO: exit or not??
        exit(1);
        // int exec_rc = execv(args[0], args);
        // printf("Done with execv\n");
        
    }
    else{
        //parent process executes here
        //use RC to wait for child process
        // int wait_rc = wait(NULL);
        int status;
        int wait_rc = waitpid(rc, &status, 0);
        // printf("Exiting from parent, my id is %d and rc is %d\n", getpid(), rc);
    }
    return 0;
}

int redirection(char* cmd1[], char* cmd2[]){
    int status;
	int newfd;	/* new file descriptor */

    int pid = fork();
    
    
    if(pid == 0){
        if ((newfd = open(cmd2[0], O_CREAT|O_TRUNC|O_WRONLY, 0644)) < 0) {
            perror(cmd2[0]);	/* open failed */
            exit(1);
        }
        dup2(newfd, 1);
        close(newfd);
        execv(cmd1[0], cmd1);
        perror(cmd1[0]);
            
        
    }
        

    else{
        int status;
        waitpid(pid, &status, 0);
    }


	return 0;
    

}

// char *cat_args[] = {"/bin/ls", "-la", "/home/summit/private/537/P3", NULL};
// char *grep_args[] = {"/bin/grep", "te", NULL};
// pipeExample(cat_args, grep_args);
void piping(char* cmd1[], char* cmd2[], int redirFlag, int numArgs2){

    // make a pipe (fds go in pipefd[0] and pipefd[1])

    int pid = fork();

    if (pid == 0)
    {
        int pipefd[2];
        pipe(pipefd);
        int pid_grep = fork();
        
        if (pid_grep == 0)
        {
            // child gets here and handles "grep bob"
            // replace standard input with input part of pipe
            dup2(pipefd[0], 0);

            close(pipefd[1]);

            if(redirFlag == 1){
                specialProcessHelper(cmd2, numArgs2);
            }

            else{
                execv(cmd2[0], cmd2);
            }
            exit(1);
        }
        else
        {
            dup2(pipefd[1], 1);

            close(pipefd[0]);

            execv(cmd1[0], cmd1);
            perror(cmd1[0]);
            exit(1);
        }
    }
    else
    {
        int status;
        wait(&status);
    }
}
    
/// description: Takes a line and splits it into args similar to how argc
///              and argv work in main
/// line:        The line being split up. Will be mangled after completion
///              of the function<
/// args:        a pointer to an array of strings that will be filled and
///              allocated with the args from the line
/// num_args:    a point to an integer for the number of arguments in args
/// return:      returns 0 on success, and -1 on failure
int lexer(char *line, char ***args, int *num_args){ // Do I need to make a fix to num_args++?
    *num_args = 0;
    // count number of args
    char *l = strdup(line);
    if(l == NULL){
        return -1;
    }
    char *token = strtok(l, " \t\n");
    while(token != NULL){
        (*num_args)++;
        token = strtok(NULL, " \t\n");
    }
    free(l);
    // split line into args
    *args = malloc(sizeof(char **) * ((*num_args) + 1));
    *num_args = 0;
    token = strtok(line, " \t\n");
    while(token != NULL){
        char *token_copy = strdup(token);
        if(token_copy == NULL){
            return -1;
        }
        (*args)[(*num_args)++] = token_copy;
        token = strtok(NULL, " \t\n");
    }
    (*args)[(*num_args)] = NULL;
    return 0;
}

void processHelper(char **args, int num_args){

    char* builtIn [] = {"exit", "cd", "pwd", "loop"};
    int loop_times;
    

    for(int i = 0; i < num_args; i++){
        // Exit
        if(strcmp(args[i], builtIn[0]) == 0){
            if(args[i+1] != NULL){
                errorHandler();
            }
            exit(0);
        }

        // Change Directoring
        else if(strcmp(args[i], builtIn[1]) == 0){
            changeDir(args, num_args, i);
            break;
        }

        // Print Working Directory
        else if(strcmp(args[i], builtIn[2]) == 0){
            printWD(args, num_args, i);
            break;
        }

        // Loop
        else if(strcmp(args[i], builtIn[3]) == 0){
            
            if(num_args < 3 || !isdigit(*args[1])){
                errorHandler();
            }

            if(isdigit(*args[i+1])){
                loop_times = atoi(args[i+1]);
                num_args = num_args - 2;
                for(int a = 0; a < loop_times; a++){
                    processHelper(&args[i+2], num_args);
                }
            }

            break;
            
        }

        // Fork Handler
        else{
            forkRedirect(args, num_args, i);
            break;
        }
    }
}

void specialProcessHelper(char **args, int num_args){

    int buffer = 40;
    char* cmd1[buffer];
    char* cmd2[buffer];
    int redirectionFlag = 0;
    int pipingFlag = 0;
    int loopFlag = 0;
    int loop_times = 0;
    int loop_marker = 0;
    int pipeIndex = 0;

    for(int i = 0; i < num_args; i++){
        if(strcmp(args[i], "loop") == 0){
            loopFlag = 1;
            loop_times = atoi(args[i+1]);
            loop_marker = 2;
        }
    }

    for(int i = loop_marker; i < num_args; i++){
        // Redirection
        if(strcmp(args[i], ">") == 0){
            redirectionFlag = 1;
            for(int a = 0; a < i-loop_marker; a++){
                cmd1[a] = malloc(sizeof(buffer));
                cmd1[a] = args[a+loop_marker];
                //printf("%s\n", cmd1[a]);
            }

            for(int b = i-loop_marker; b < buffer; b++){
                cmd1[b] = NULL;
            }

            cmd2[0] = malloc(sizeof(buffer));
            cmd2[0] = args[i+1];

            for(int b = 1; b < buffer; b++){
                cmd2[b] = NULL;
            }
        }
    }

    for(int i = loop_marker; i < num_args; i++){
        // Piping
        if(strcmp(args[i], "|") == 0){
            pipingFlag = 1;
            pipeIndex = i;
            for(int a = 0; a < i-loop_marker; a++){
                cmd1[a] = malloc(sizeof(1000));
                cmd1[a] = args[a+loop_marker];
            }

            for(int b = i-loop_marker; b < buffer; b++){
                cmd1[b] = NULL;
            }

            int c;
            for(c = 0; c < num_args-i-1; c++){
                cmd2[c] = malloc(sizeof(1000));
                cmd2[c] = args[c+i+1];
            }

            cmd2[c] = NULL;
        }
    }
    
    if(loopFlag ==  1){
        for(int i = 0; i < loop_times; i++){

            if(redirectionFlag == 1 && pipingFlag == 1){
                piping(cmd1, cmd2, redirectionFlag, num_args-pipeIndex-1);
            }

            else if(pipingFlag == 1){
                piping(cmd1, cmd2, redirectionFlag, num_args-pipeIndex-1);
            }

            else{
                redirection(cmd1, cmd2);
            }
        }
    }

    else if(loopFlag == 0){
       if(redirectionFlag == 1 && pipingFlag == 1){
            piping(cmd1, cmd2, redirectionFlag, num_args-pipeIndex-1);
        }

        else if(pipingFlag == 1){
            piping(cmd1, cmd2, redirectionFlag, num_args-pipeIndex-1);
        }

        else{
            redirection(cmd1, cmd2);
        }
    }
    
    
}
        

int main(){

    char** args = NULL;
    int num_args = 0;
    size_t buffer = 100;
    size_t size;
    char* commands;
    char* c = commands;
    size_t input;
    char** tempArgs = malloc(sizeof(buffer));
    int temp_num_args = 0;
    int argc = 1;
    char* temp;
    int multi = 0;
    

    

    while(1){
        fprintf(stdout, "smash> ");
        fflush(stdout);
        int counter = 0;
        int specialFlag = 0;
        input = getline(&c, &buffer, stdin);

        //char* comm;

        if(strchr(c, ';') != NULL)
        {
            c = strtok_r(c, ";", &temp);
            multi = 1;
        }

        while(c != NULL || multi == 0){
            if(input != -1 && lexer(c, &args, &num_args) != -1){

                for(int i = 0; i < num_args; i++){
                    if(strcmp(args[i], ">") == 0 || strcmp(args[i], "|") == 0){
                        specialFlag = 1;
                    }

                    counter++;
                }

                if(specialFlag == 1){
			if(num_args < 3){
			    errorHandler();
			    break;
			}
                    specialProcessHelper(args, num_args);
                }

                else{
                    processHelper(args, num_args);
                }            

                if(multi == 1){
                    c = strtok_r(NULL, ";", &temp);
                }

                if(num_args == counter && multi == 0){
                    break;
                }

            }

            else{
                errorHandler();
            }

        }
          
    }
}

// /bin/cat test2.txt | /bin/sort  
