#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int read_args_helper(char *args[], FILE *fp){
    // use getline() to read in arguments from fp
    char *line = malloc(100 * sizeof(char));
    size_t buf_size;
    int read_args;

    // read arguments into line from fp (stdin)
    read_args = getline(&line, &buf_size, fp);

    // ERROR: nothing to read
    if (read_args == -1){
        return -1;
    }

    int line_length = strlen(line);
    // line is empty
    if(line_length == 0 || strcmp(line, "") == 0 || strcmp(line, "\n") == 0){
        return 1;
    }

    // parsing the command line
    char *found;
    int idx = 0;
    int len;
    while((found = strsep(&line, " ")) != NULL){
        // printf("%s\n", found);
        len = strlen(found);
        if(found[len-1] == '\n'){
            found[len-1] = 0;
        }
        args[idx++] = strdup(found);
    }
    idx = 0;
    while(args[idx]){
        printf("%s\n", args[idx++]);
    }
    
    //printf("Checking 1st arg: %d\n", strcmp(args[0], "exit"));
    // null terminate args 
    args[idx] = NULL;
    if(strcmp(args[0], "exit") == 0){
        // check exit and if there is an argument after exit
        if(args[1] != NULL){
            printf("ERROR: Args after exit\n");
            return -1;
        } else{
            exit(0);
        }
    }

    if(strcmp(args[0], "cd") == 0){
        if(args[1] == NULL || args[2] != NULL){
            printf("ERROR: Invalid cd args\n");
            return -1;
        }
        // try changing directories
        // chdir() return 0 if it can change, something else otherwise
        int ret = chdir(args[1]);
        if (ret == 0) {
            return 1;
        } else{
            printf("Error: Could not change directories\n");
            return -1;
        }
    }

    /*
    if(strcmp(args[0], "pwd") == 0){
        char *buf = (char*)malloc(100 * sizeof(char));

        int ptr = getcwd()
    }
    */
    free(line);
    return 0;
}
int main(int argc, char *argv[]){

    char** arguments;

    if (argc > 1){
        exit(1); 
    }

    while(1){
        printf("smash> ");

        arguments = malloc(sizeof(char *) * 50); // max size of arguments???

        // create array of strings
        for(int i = 0; i < 50; i++){
            arguments[i] = malloc(sizeof(char) * 256);
        }
        int canProcess = read_args_helper(arguments, stdin);

        if (canProcess == 1){
            return 1;
        }


        // clearing up array of strings
        for(int i = 0; i < 50; i++){
            free(arguments[i]);
        }
        free(arguments);
        
    }

    return 0;
}