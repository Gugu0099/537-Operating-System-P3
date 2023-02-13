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
            if (args[i + 1] != NULL)
            {
                printf("ERROR: Args after exit\n");
                return -1;
            }
            else
            {
                exit(0);
            }
        }
        if (strcmp(args[i], "cd") == 0)
        {
            if (args[i + 1] == NULL || args[i + 2] != NULL)
            {
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
        if (strcmp(args[i], "pwd") == 0)
        {
            char buf[1024];
            if (getcwd(buf, sizeof(buf)) == NULL)
            {
                printf("Error: pwd is NULL\n");
                return -1;
            }
            printf("%s\n", buf);
        }

        if (strcmp(args[i], "loop") == 0)
        {
            int loopNum;
            if (isdigit(*args[i + 1]))
            { // why dereference it ???
                loopNum = atoi(args[i + 1]);
            }
            else
            {
                printf("Error: arguments after loop is not int\n");
                return -1;
            }
            for(int i = 0; i < loopNum; i++){
                process_cmds(&args[i + 2], num_args - i - 2);
            }
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
    char **found;
    int len = 0;
    len = strlen(line);
    lexer(line, &found, &len);
    int rc = process_cmds(found, len);
    // printf("%d\n", len);
    // printf("%s\n", *found);
    for (int i = 0; i < len; i++)
    {
        printf("%s\n", found[i]);
    }
    /*
    for(int i = 0; i < len; i++){
        printf("idx %d: %s\n", i, found[i]);
    }
    */
    /*
    if (strcmp(found[0], "exit") == 0)
    {
        // printf("Error\n");
        //  check exit and if there is an argument after exit
        if (found[1] != NULL)
        {
            printf("ERROR: Args after exit\n");
            return -1;
        }
        else
        {
            exit(0);
        }
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
    if (strcmp(found[0], "pwd") == 0)
    {
        char buf[1024];
        if (getcwd(buf, sizeof(buf)) == NULL)
        {
            printf("Error: pwd is NULL\nSpecifically, w");
            return -1;
        }
        printf("%s\n", buf);
    }

    if (strcmp(args[0], "loop") == 0)
    {
        int loopNum;
        if (isdigit(*found[1]))
        { // why dereference it ???
            loopNum = atoi(found[1]);
        }
        else
        {
            printf("Error: arguments after loop is not int\n");
            return -1;
        }
        char **for_args;
        for_args = malloc(sizeof(char *) * 50);

        // create array of strings for the for loop arguments
        for (int i = 0; i < 50; i++)
        {
            for_args[i] = malloc(sizeof(char) * 256);
        }
        int x = 0;
        // copy for loop arguments
        while(1){
            if (found[x+2] == NULL || strcmp(found[x+2], ";")){ // TODO: maybe change to ';'
                break;
            }
            for_args[x] = strdup(found[x+2]);
            x++;

        }
        // execute for loop args repeatedly
        for (int i = 0; i < loopNum; i++)
        {
            continue;
        }
        // freeing the for loop arguments
        for (int i = 0; i < 50; i++)
        {
            free(for_args[i]);
        }
        free(for_args);
    }

    // forkredirct(found, len);
    */
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