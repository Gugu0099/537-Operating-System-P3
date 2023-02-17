#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>

/// description: Takes a line and splits it into args similar to how argc
///              and argv work in main
/// line:        The line being split up. Will be mangled after completion
///              of the function<
/// args:        a pointer to an array of strings that will be filled and
///              allocated with the args from the line
/// num_args:    a point to an integer for the number of arguments in args
/// return:      returns 0 on success, and -1 on failure
 int lexer(char *line, char ***args, int *num_args){
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
     *args = malloc(sizeof(char **) * *num_args);
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
     return 0;
 }

int forkredirct(char **args, int num_args)
{
    int rc = fork();

    if (rc == 0)
    {
        int erec_rc = execv(args[0], args);
        printf("Fork error has occured\n");
    }
    else
    {
        int status;
        int wait_rc = waitpid(rc, &status, 0);
    }
    return 0;
}

int process_cmds(char **args, int num_args)
{
    int i = 0;
    while (i < num_args)
    {   
        if (strcmp(args[i], "exit") == 0)
        {
            // printf("Error\n");
            //  check exit and if there is an argument after exit
            if (i + 1 < num_args && args[i + 1] != NULL)
            {
                printf("ERROR: Args after exit\n");
                return -1;
            }
            else
            {
                exit(0);
            }
        }
        else if (strcmp(args[i], "cd") == 0)
        {   
            //printf("n_args: %d, i: %d\n", num_args, i);
            if ((i + 1 < num_args && args[i + 1] == NULL) ||( i + 2 < num_args && strcmp(args[i + 2],";")))
            {
                printf("A: %s %d\n", args[i + 2], strcmp(args[i + 2],";"));
                printf("ERROR: Invalid cd args\n");
                return -1;
            }
            // try changing directories
            // chdir() return 0 if it can change, something else otherwise
            int ret = chdir(args[i + 1]);
            if (ret != 0)
            {
                printf("Error: Could not change directories\n");
                return -1;
            }
            else
            {
                return 1;
            }
        }

        // printf("%d\n", strcmp(found[0], "pwd"));
        else if (strcmp(args[i], "pwd") == 0)
        {
            char buf[1024];
            if (getcwd(buf, sizeof(buf)) == NULL)
            {
                printf("Error: pwd is NULL\n");
                return -1;
            }
            printf("%s\n", buf);
        }

        else if (strcmp(args[i], "loop") == 0)
        {
            int loopNum;
            if (i + 1 >= num_args){
                printf("Error: not enough args for the loop function");
            }
            if (isdigit(*args[i + 1])) // strlen(str) == strlen( itoa(atoi(str)) <- for numbers greater than 9
            { // why dereference it ???
                loopNum = atoi(args[i + 1]);
                
                if (loopNum == 0){
                    printf("Error: cannot execute loop 0 times"); // todo skip over args??
                    process_cmds(&args[i + 3], num_args - i - 3);
                }
            }
            else
            {
                printf("Error: arguments after loop is not int\n");
                return -1;
            }
            // execute command loopNum times
            for(int j = 0; j < loopNum; j++){
                //printf("A1: %s\n", args[i + 2]);
                if (process_cmds(&args[i + 2], num_args - i - 2) == -1){
                    printf("ERROR: Couldn't execute loop command\n");
                }
            }
            i+=2; // skip over args and go to top of loop again
            continue;
        }
        i++;
    }
    return 0;
}

int read_args_helper(FILE *fp)
{
    // int contains_loop = 0;
    //  use getline() to read in arguments from fp
    char *line = malloc(100 * sizeof(char));
    size_t buf_size;
    int read_args;
    int c_mult_cmds = 0;
    // read arguments into line from fp (stdin)
    read_args = getline(&line, &buf_size, fp);

    // ERROR: nothing to read
    if (read_args == -1)
    {
        return -1;
    }

    int line_length = strlen(line);
    // line is empty
    if (line_length == 0 || strcmp(line, "") == 0 || strcmp(line, "\n") == 0)
    {
        return 1;
    }

    // check if we have multiple commands
    for (int i = 0; i < line_length; i++){
        if (line[i] == ';'){
            c_mult_cmds = 1;
            break;
        }
    }
    
    if (c_mult_cmds){
        // split line by white space and semicolon and repeatedly process commands 

        char ** found = malloc(sizeof(char *) * 50);
        for (int k = 0; k < 50; k++){
            found[k] = malloc(100);
        }
        int len = 0;
        len = strlen(line);
        lexer(line, &found, &len);
        int i = 0;
        for(int k = 0; k < len; k++){
            //printf("ffff: %s\n", found[k]); 
        }
        while (i < len){
            int arr_size = 0;
            // grab the size of a command
            for(int j = i; j < len && strcmp(found[j],";") != 0; j++){
                arr_size++;
            }
            // base case empty commands
            if (arr_size == 0){
                i++;
                continue;
            }
            //printf("cmd size: %d\n", arr_size);
            // malloc array and process a command
            char ** currCommand = malloc(sizeof(char *) * 50);
            for (int k = 0; k < 50; k++){
                currCommand[k] = malloc(100);
            }
            // fill the array with current command
            for (int k = 0; k < arr_size; k++){
                strcpy(currCommand[k],found[i++]);
                //printf("cccc: %s\n", currCommand[k]); 
                //printf("dddd: %s\n", found[i-1]); 
            }
            // run the current command
            int rc = process_cmds(currCommand,arr_size);
            // free the array for the current command
            for (int k = 0; k < 50; k++){
                free(currCommand[k]);
            }
            free(currCommand);
        }
        /*
        // process the first command

        if (currCommandRaw){
            char **found;
            int len = 0;
            len = strlen(currCommandRaw);
            theRest = strdup(currCommandRaw);
            lexer(currCommandRaw, &found, &len);
            int rc = process_cmds(found, len);
            currCommandRaw = strdup(theRest); 
            printf("cccc: %s\n", currCommandRaw);     
        }
        // process 2nd commands and onwards
        while (currCommandRaw != NULL)
        {   
            printf("%s\n", currCommandRaw);
            currCommandRaw2 = strdup(currCommandRaw);
            char **found;
            int len = 0;
            len = strlen(currCommandRaw);
            lexer(currCommandRaw, &found, &len);

            int rc = process_cmds(found, len);
            idx++;
            printf("idx: %d ; %s\n", idx,currCommandRaw2);
            currCommandRaw = strdup(currCommandRaw2);
            currCommandRaw = strtok(NULL, ";");
            //printf("currcmd: %s\n", currCommandRaw);
        }
        */
    } else{ // just need to process a singular command
        char **found;
        int len = 0;
        len = strlen(line);
        lexer(line, &found, &len);
        int rc = process_cmds(found, len);
    }
    free(line);
    return 0;
}

int main(int argc, char *argv[])
{

    if (argc > 1)
    {
        printf("Too many number of args in smash\n");
        exit(1);
    }

    while (1)
    {
        printf("smash> ");

        int canProcess = read_args_helper(stdin);

        if (canProcess == 1)
        {
            continue;
        }
    }
    return 0;
}