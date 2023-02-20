#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>

int errorHandler(int i)
{
    char error_message[30] = "An error has occurred\n";
    // error_message[strlen(error_message)-3] = i%10 + '0';
    // error_message[strlen(error_message)-4] = i/10 + '0';
    // printf("\n%d\n\n\n", i);
    write(STDERR_FILENO, error_message, strlen(error_message));
}

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

int forkredirct(char **args, int num_args, FILE *fp)
{
   // fprintf(stderr,"start of redirection\n");
    args[num_args] = NULL;
    int rc = fork();
    if (fp)
    {
        // redo args
       // printf("num_args: %d\n", num_args);
        char **args2 = malloc(sizeof(char *) * (num_args - 1));
        for (int i = 0; i < num_args - 1; i++)
        {
            args2[i] = malloc(100);
        }
        // copy over old args into new args
        for (int i = 0; i < num_args - 2; i++)
        {
            strcpy(args2[i], args[i]);
        }
        args2[num_args - 2] = NULL;
        /*
        for (int i = 0; i < num_args - 1; i++)
            {
                printf("args[%d]: %s\n", i, args2[i]);
            }
*/
        if (rc == 0)
        {
            int oldfile = fileno(fp);
            dup2(oldfile, STDOUT_FILENO);
            
            // printf("args[0]: %s\n", args[0]);
            /*
            for (int i = 0; i < num_args; i++)
            {
                printf("args[%d]: %s\n", i, args[i]);
            }
            */
            int erec_rc = execv(args2[0], args2);
            errorHandler(1);
            _exit(1);
            // fprintf(fp, "An error has occurred\n");
            // printf("return code: %d\n", rc);
            // printf("Fork error has occured\n");
        }
        else
        {
            int status;
            int wait_rc = waitpid(rc, &status, 0);
            int exitStatus = WEXITSTATUS(status);
            // free new args
            /*
            for (int i = 0; i < num_args - 2; i++)
            {
                if (args2[i] != NULL){
                printf("aaaaaaa: %s     %d\n",args2[i], i);
                free(args2[i]);
                }
            }
            if (args2){
                free(args2);
            }
            */
          //  fprintf(stderr,"end of redirection\n");
            return exitStatus;
        }
    }
    else
    {

        if (rc == 0)
        {
            // printf("args[0]: %s\n", args[0]);
            /*
            for (int i = 0; i < num_args; i++)
            {
                printf("args[%d]: %s\n", i, args[i]);
            }
            */
            

            int erec_rc = execv(args[0], args);
            errorHandler(2);
            _exit(1); // _exit with execv, otherwise funcky behavior;
            // printf("return code: %d\n", rc);
            // printf("Fork error has occured\n");
        }
        else
        {
            int status;
            int wait_rc = waitpid(rc, &status, 0);
            int exitStatus = WEXITSTATUS(status);
            return exitStatus;
        }
    }
    return 0;
}

void pipes(char *args1[], char *args2[])
{
    // Compute number of arguments in args1
    int num_args1 = 0;
    while (args1[num_args1] != NULL)
    {
        num_args1++;
    }

    // Compute number of arguments in args2
    int num_args2 = 0;
    while (args2[num_args2] != NULL)
    {
        num_args2++;
    }

    // Rest of the function remains the same
    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        printf("error");
        exit(1);
    }

    int pid1 = fork();
    if (pid1 < 0)
    {
        exit(1);
    }
    if (pid1 == 0)
    {
        // parent gets here and handles "cat scores"
        // replace standard output with output part of pipe
        dup2(pipefd[1], 1);
        // close unused input half of pipe
        close(pipefd[0]);

        // execute child process 1
        execv(args1[0], args1);
        perror(args1[0]);
        _exit(1);
    }
    else
    {
        int status;
        waitpid(pid1, &status, 0);
    }

    int pid2 = fork();
    if (pid2 < 0)
    {
        exit(1);
    }
    if (pid2 == 0)
    {
        // replace standard input with input part of pipe
        dup2(pipefd[0], 0);
        close(pipefd[1]);

        // execute child process 2
        execv(args2[0], args2);
        perror(args2[0]);
        _exit(1);
    }
    else
    {
        close(pipefd[0]);
        close(pipefd[1]);
        int status;
        waitpid(pid2, &status, 0);
    }
}

int process_cmds(char **args, int num_args)
{
    // printf("process_cmd called:\n");
    // for(int j=0; j< num_args; j++){
    //     printf("%s ", args[j]);
    // }
    // printf("\n");
    int i = 0;
    char *output;
    //  iterate the command;
    int flag = 0; // high if redirection sign is present
    int pipeFlag = 0;
    int pipeNum = -1;
    for (int x = 0; x < num_args; x++)
    {
        if (strcmp(args[x], ">") == 0)
        {
            flag = 1;
            if (x + 1 < num_args)
            {
                output = strdup(args[x + 1]);
            }
        }
        if (strcmp(args[x], "|") == 0)
        {
            pipeFlag = 1;
            pipeNum = x;
        }
    }
    if (flag)
    {
        FILE *fp;
        fp = fopen(output, "w");
          // while (args[i] != output)
          // {
            if (strcmp(args[0], "exit") == 0)
            {
                // printf("Error\n");
                //  check exit and if there is an argument after exit
                if (0 + 1 < num_args && args[0 + 1] != NULL)
                {
                    errorHandler(3);
                    // fprintf(fp, "ERROR: Args after exit\n");
                    return -1;
                }
                else
                {
                    exit(0);
                }
            }
            else if (strcmp(args[0], "cd") == 0)
            {
                // printf("n_args: %d, i: %d\n", num_args, i);
                if ((0 + 1 < num_args && args[0 + 1] == NULL) || (0 + 2 < num_args && strcmp(args[0 + 2], ";")))
                {
                    errorHandler(4);
                    // fprintf(fp, "A: %s %d\n", args[i + 2], strcmp(args[i + 2], ";"));
                    // fprintf(fp, "ERROR: Invalid cd args\n");
                    return -1;
                }
                // try changing directories
                // chdir() return 0 if it can change, something else otherwise
                int ret = chdir(args[0 + 1]);
                // printf("args[i+1]: %s\n", args[i+1]);

                if (ret != 0)
                {
                    errorHandler(5);
                    // fprintf(fp, "Error no: %d\n", ret);
                    // fprintf(fp, "Error: Could not change directories\n");
                    return -1;
                }
                else
                {
                    return 1;
                }
            }

            // printf("%d\n", strcmp(found[0], "pwd"));
            else if (strcmp(args[0], "pwd") == 0)
            {
                char buf[1024];
                if (getcwd(buf, sizeof(buf)) == NULL)
                {
                    errorHandler(6);
                    // fprintf(fp, "Error: pwd is NULL\n");
                    return -1;
                }
                fprintf(fp, "%s\n", buf);
            }

            else if (strcmp(args[0], "loop") == 0)
            {
                int loopNum;
                if (0 + 1 >= num_args)
                {
                    errorHandler(7);
                    // fprintf(fp, "Error: not enough args for the loop function");
                }
                if (isdigit(*args[0 + 1])) // strlen(str) == strlen( itoa(atoi(str)) <- for numbers greater than 9
                {                          // why dereference it ???
                    loopNum = atoi(args[0 + 1]);

                    if (loopNum == 0)
                    {
                        errorHandler(8);
                        // fprintf(fp, "Error: i,cannot execute loop 0 times"); // todo skip over args??
                        //  process_cmds(&args[i + 3], num_args - i - 3);
                        return 1;
                    }
                }
                else
                {
                    errorHandler(8);
                    // fprintf(fp, "Error: arguments after loop is not int\n");
                    return -1;
                }
                // execute command loopNum times
                if (loopNum == 1)
                {
                    process_cmds(&args[0 + 2], num_args - 0 - 2);
                }
                else
                {
                    for (int j = 0; j < loopNum; j++)
                    {
                        // printf("A1: %s\n", args[i + 2]);
                        if (process_cmds(&args[0 + 2], num_args - 0 - 2) == -1)
                        {
                            errorHandler(9);
                            // fprintf(fp, "ERROR: Couldn't execute loop command\n");
                        }
                    }
                    // i += 2; // skip over args and go to top of loop again
                    // continue;
                }
                // break;
            }
            else if (pipeFlag == 1)
            {
                //fprintf(stderr,"start of pipe1");
                int i;
                int delimiter_found = 0;
                int input_size = num_args;
                char *delimiter = "|";
                char **output_array1 = malloc(sizeof(char *) * input_size);
                char **output_array2 = malloc(sizeof(char *) * input_size);

                for (i = 0; i < input_size; i++)
                {
                    if (strcmp(args[i], delimiter) == 0)
                    {
                        delimiter_found = 1;
                        break;
                    }
                    output_array1[i] = args[i];
                }
                output_array1[i] = NULL;

                if (delimiter_found)
                {
                    int j;
                    for (j = 0, i = i + 1; i < input_size; i++, j++)
                    {
                        output_array2[j] = args[i];
                    }
                    output_array2[j] = NULL;
                }
                else
                {
                    output_array2[0] = NULL;
                }
                pipes(output_array1,output_array2);
                // Free memory allocated for the output arrays
                free(output_array1);
                free(output_array2);
                //fprintf(stderr,"end of pipe1");
            }
            else
            {
                return forkredirct(args, num_args, fp);
            }
            // i++;
            fclose(fp);
        }
        // fclose(fp);
    else
    {
         // while (i < num_args)
         // {
            if (strcmp(args[0], "exit") == 0)
            {
                // printf("Error\n");
                //  check exit and if there is an argument after exit
                if (0 + 1 < num_args && args[0 + 1] != NULL)
                {
                    errorHandler(10);
                    // printf("ERROR: Args after exit\n");
                    return -1;
                }
                else
                {
                    exit(0);
                }
            }
            else if (strcmp(args[0], "cd") == 0)
            {
                // printf("n_args: %d, i: %d\n", num_args, i);
                if ((0 + 1 < num_args && args[0 + 1] == NULL) || (0 + 2 < num_args && strcmp(args[0 + 2], ";")))
                {
                    errorHandler(11);
                    // printf("A: %s %d\n", args[i + 2], strcmp(args[i + 2], ";"));
                    // printf("ERROR: Invalid cd args\n");
                    return -1;
                }
                // try changing directories
                // chdir() return 0 if it can change, something else otherwise
                int ret = chdir(args[0 + 1]);
                // printf("args[i+1]: %s\n", args[i+1]);

                if (ret != 0)
                {
                    errorHandler(12);
                    // printf("Error no: %d\n", ret);
                    // printf("Error: Could not change directories\n");
                    return -1;
                }
                else
                {
                    return 1;
                }
            }

            // printf("%d\n", strcmp(found[0], "pwd"));
            else if (strcmp(args[0], "pwd") == 0)
            {
                char buf[1024];
                if (getcwd(buf, sizeof(buf)) == NULL)
                {
                    errorHandler(13);
                    // printf("Error: pwd is NULL\n");
                    return -1;
                }
                printf("%s\n", buf);
            }

            else if (strcmp(args[0], "loop") == 0)
            {
                int loopNum;
                if (0 + 1 >= num_args)
                {
                    errorHandler(14);
                    // printf("Error: not enough args for the loop function");
                }
                if (isdigit(*args[0 + 1])) // strlen(str) == strlen( itoa(atoi(str)) <- for numbers greater than 9
                {                          // why dereference it ???
                    loopNum = atoi(args[0 + 1]);

                    if (loopNum == 0)
                    {
                        errorHandler(15);
                        // printf("Error: cannot execute loop 0 times"); // todo skip over args??
                        //  process_cmds(&args[i + 3], num_args - i - 3);
                        return 1;
                    }
                }
                else
                {
                    errorHandler(16);
                    // printf("Error: arguments after loop is not int\n");
                    return -1;
                }
                // execute command loopNum times
                if (loopNum == 1)
                {
                    process_cmds(&args[0 + 2], num_args - 0 - 2);
                }
                else
                {
                    for (int j = 0; j < loopNum; j++)
                    {
                        // printf("A1: %s\n", args[i + 2]);
                        if (process_cmds(&args[0 + 2], num_args - 0 - 2) == -1)
                        {
                            errorHandler(17);
                            // printf("ERROR: Couldn't execute loop command\n");
                        }
                    }
                   // i += 2; // skip over args and go to top of loop again
                  //  continue;
                }
              //  break;
             }
            else if (pipeFlag == 1)
            {
                // fprintf(stderr,"start of pipe2");
                int i;
                int delimiter_found = 0;
                int input_size = num_args;
                char *delimiter = "|";
                char **output_array1 = malloc(sizeof(char *) * input_size);
                char **output_array2 = malloc(sizeof(char *) * input_size);

                for (i = 0; i < input_size; i++)
                {
                    if (strcmp(args[i], delimiter) == 0)
                    {
                        delimiter_found = 1;
                        break;
                    }
                    output_array1[i] = args[i];
                }
                output_array1[i] = NULL;

                if (delimiter_found)
                {
                    int j;
                    for (j = 0, i = i + 1; i < input_size; i++, j++)
                    {
                        output_array2[j] = args[i];
                    }
                    output_array2[j] = NULL;
                }
                else
                {
                    output_array2[0] = NULL;
                }
                //fprintf(stderr,"bruh");

                pipes(output_array1,output_array2);
                //fprintf(stderr,"bruh 2");

                // Free memory allocated for the output arrays
                free(output_array1);
                free(output_array2);
                //fprintf(stderr,"end of pipe2");
            }
            else
            {
                // printf("here\n");
                return forkredirct(args, num_args, NULL);
            }
        //i++;
  //  }
}
return 0;
}
// if check the exev call; exev only return if it fails;
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
    for (int i = 0; i < line_length; i++)
    {
        if (line[i] == ';')
        {
            c_mult_cmds = 1;
            break;
        }
    }

    if (c_mult_cmds)
    {
        // split line by white space and semicolon and repeatedly process commands

        char **found = malloc(sizeof(char *) * 50);
        for (int k = 0; k < 50; k++)
        {
            found[k] = malloc(100);
        }
        int len = 0;
        len = strlen(line);
        lexer(line, &found, &len);
        int i = 0;
        // for(int k = 0; k < len; k++){
        //     //printf("ffff: %s\n", found[k]);
        // }
        while (i < len)
        { // iterating from start to end command;
            int arr_size = 0;
            // grab the size of a command
            for (int j = i; j < len && strcmp(found[j], ";") != 0; j++)
            {
                arr_size++;
            }
            // base case empty commands
            if (arr_size == 0)
            {
                i++;
                continue;
            }
            // printf("cmd size: %d\n", arr_size);
            //  malloc array and process a command
            char **currCommand = malloc(sizeof(char *) * 50);
            for (int k = 0; k < 50; k++)
            {
                currCommand[k] = malloc(100);
            }
            // fill the array with current command
            for (int k = 0; k < arr_size; k++)
            {
                strcpy(currCommand[k], found[i++]);
                // printf("cccc: %s\n", currCommand[k]);
                // printf("dddd: %s\n", found[i-1]);
            }
            // run the current command
            int rc = process_cmds(currCommand, arr_size);
            // free the array for the current command
            for (int k = 0; k < 50; k++)
            {
                free(currCommand[k]);
            }
            free(currCommand);
        }
    }
    else
    { // just need to process a singular command
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
        errorHandler(20);
        // printf("Too many number of args in smash\n");
        exit(1);
    }

    while (1)
    {
        fprintf(stdout, "smash> ");
        fflush(stdout);
        int canProcess = read_args_helper(stdin);

        if (canProcess == 1)
        {
            continue;
        }
    }
    return 0;
}