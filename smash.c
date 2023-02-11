#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

/// description: Takes a line and splits it into args similar to how argc
///              and argv work in main
/// line:        The line being split up. Will be mangled after completion
///              of the function<
/// args:        a pointer to an array of strings that will be filled and
///              allocated with the args from the line
/// num_args:    a point to an integer for the number of arguments in args
/// return:      returns 0 on success, and -1 on failure
int lexer(char *line, char ***args, int *num_args)
{
    *num_args = 0;
    // count number of args
    char *l = strdup(line);
    if (l == NULL)
    {
        return -1;
    }
    char *token = strtok(l, " \t\n");
    while (token != NULL)
    {
        (*num_args)++;
        token = strtok(NULL, " \t\n");
    }
    free(l);
    // split line into args
    *args = malloc(sizeof(char **) * *num_args);
    *num_args = 0;
    token = strtok(line, " \t\n");
    while (token != NULL)
    {
        char *token_copy = strdup(token);
        if (token_copy == NULL)
        {
            return -1;
        }
        (*args)[(*num_args)++] = token_copy;
        token = strtok(NULL, " \t\n");
    }
    return 0;
}

int read_args_helper(char *args[], FILE *fp)
{
    // use getline() to read in arguments from fp
    char *line = malloc(100 * sizeof(char));
    size_t buf_size;
    int read_args;

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

    // parsing the command line
    char **found;
    int len = 0;
    len = strlen(line);
    lexer(line, &found, &len);
    // printf("%d\n", len);
    // printf("%s\n", *found);
    // while(len){
    //     if(*found[len - 1] == '\n'){
    //         *found[len - 1] = 0;
    //     }
    //     len--;
    // }
    
    if (strcmp(found[0], "exit") == 0)
    {
        // check exit and if there is an argument after exit
        if (found[1] != NULL)
        {
            printf("ERROR: Args after exit\n");
            return -1;
        }
        exit(0);
    }

    if (strcmp(found[0], "cd") == 0)
    {
        if (found[1] == NULL || found[2] != NULL)
        {
            printf("ERROR: Invalid cd args\n");
            return -1;
        }
        // try changing directories
        // chdir() return 0 if it can change, something else otherwise
        int ret = chdir(found[1]);
        if (ret == 0)
        {
            return 1;
        }
        else
        {
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

int main(int argc, char *argv[])
{

    char **arguments;

    if (argc > 1)
    {
        exit(1);
    }

    while (1)
    {
        printf("smash> ");

        arguments = malloc(sizeof(char *) * 50); // max size of arguments???

        // create array of strings
        for (int i = 0; i < 50; i++)
        {
            arguments[i] = malloc(sizeof(char) * 256);
        }
        int canProcess = read_args_helper(arguments, stdin);

        if (canProcess == 1)
        {
            return 1;
        }

        // clearing up array of strings
        for (int i = 0; i < 50; i++)
        {
            free(arguments[i]);
        }
        free(arguments);
    }

    return 0;
}