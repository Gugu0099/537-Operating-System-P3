#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>

int errorHandler()
{
    char error_message[30] = "An error has occurred\n";
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
    args[num_args] = NULL;
    int rc = fork();
    if (fp)
    {
        // redo args
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

        if (rc == 0)
        {
            int oldfile = fileno(fp);
            dup2(oldfile, STDOUT_FILENO);

            int erec_rc = execv(args2[0], args2);
            errorHandler();
            _exit(1);
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
            return exitStatus;
        }
    }
    else
    {

        if (rc == 0)
        {
            int erec_rc = execv(args[0], args);
            errorHandler();
            _exit(1); // _exit with execv, otherwise funcky behavior;
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

int pipes(char *args1[], char *args2[], FILE *fp)
{
    int hasRedir = 0;
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
        if (strcmp(args2[num_args2], ">") == 0)
        {
            hasRedir = 1;
        }
        num_args2++;
    }

    if (hasRedir == 0)
    {
        int pipefd[2];
        if (pipe(pipefd) == -1)
        {
            printf("error");
            exit(1);
        }

        int rc1 = fork();
        if (rc1 < 0)
        {
            exit(1);
        }
        if (rc1 == 0)
        {
            dup2(pipefd[1], 1);
            close(pipefd[0]);

            execv(args1[0], args1);
            perror(args1[0]);
            _exit(1);
        }
        else
        {
            int status;
            waitpid(rc1, &status, 0);
        }

        int rc2 = fork();
        if (rc2 < 0)
        {
            exit(1);
        }
        if (rc2 == 0)
        {
            dup2(pipefd[0], 0);
            close(pipefd[1]);

            execv(args2[0], args2);
            perror(args2[0]);
            _exit(1);
        }
        else
        {
            close(pipefd[0]);
            close(pipefd[1]);
            int status;
            waitpid(rc2, &status, 0);
        }
    }
    else
    {
        // need to shorten args2
        char **args3 = malloc(sizeof(char *) * (num_args2 - 1));
        for (int i = 0; i < num_args2 - 2; i++)
        {
            args3[i] = strdup(args2[i]);
        }

        args3[num_args2 - 2] = NULL;
        int pipefd[2];
        if (pipe(pipefd) == -1)
        {
            errorHandler();
            exit(1);
        }

        int rc1 = fork();
        if (rc1 < 0)
        {
            exit(1);
        }
        if (rc1 == 0)
        {
            int oldfile = fileno(fp);
            dup2(oldfile, STDOUT_FILENO);

            dup2(pipefd[1], 1);
            close(pipefd[0]);

            execv(args1[0], args1);
            perror(args1[0]);
            _exit(1);
        }
        else
        {
            int status;
            waitpid(rc1, &status, 0);
        }

        int rc2 = fork();
        if (rc2 < 0)
        {
            exit(1);
        }
        if (rc2 == 0)
        {
            int oldfile = fileno(fp);
            dup2(oldfile, STDOUT_FILENO);

            dup2(pipefd[0], 0);
            close(pipefd[1]);

            execv(args3[0], args3);
            perror(args3[0]);
            _exit(1);
        }
        else
        {
            close(pipefd[0]);
            close(pipefd[1]);
            int status;
            waitpid(rc2, &status, 0);
        }
    }
    return 0;
}

int process_cmds(char **args, int num_args)
{
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

        if (strcmp(args[0], "exit") == 0)
        {
            //  check exit and if there is an argument after exit
            if (0 + 1 < num_args && args[0 + 1] != NULL)
            {
                errorHandler();
                return -1;
            }
            else
            {
                exit(0);
            }
        }
        else if (strcmp(args[0], "cd") == 0)
        {
            if ((0 + 1 < num_args && args[0 + 1] == NULL) || (0 + 2 < num_args && strcmp(args[0 + 2], ";")))
            {
                errorHandler();
                return -1;
            }
            // try changing directories
            // chdir() return 0 if it can change, something else otherwise
            int ret = chdir(args[0 + 1]);

            if (ret != 0)
            {
                errorHandler();
                return -1;
            }
            else
            {
                return 1;
            }
        }

        else if (strcmp(args[0], "pwd") == 0)
        {
            char buf[1024];
            if (getcwd(buf, sizeof(buf)) == NULL)
            {
                errorHandler();
                return -1;
            }
            fprintf(fp, "%s\n", buf);
        }

        else if (strcmp(args[0], "loop") == 0)
        {
            int loopNum;
            if (0 + 1 >= num_args)
            {
                errorHandler();
            }
            if (isdigit(*args[0 + 1])) // strlen(str) == strlen( itoa(atoi(str)) <- for numbers greater than 9
            {                          // why dereference it ???
                loopNum = atoi(args[0 + 1]);

                if (loopNum == 0)
                {
                    errorHandler();
                    return 1;
                }
            }
            else
            {
                errorHandler();
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
                    if (process_cmds(&args[0 + 2], num_args - 0 - 2) == -1)
                    {
                        errorHandler();
                    }
                }
            }
        }
        else if (pipeFlag == 1)
        {
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

            pipes(output_array1, output_array2, fp);
            // Free memory allocated for the output arrays
            free(output_array1);
            free(output_array2);
        }
        else
        {
            return forkredirct(args, num_args, fp);
        }
        fclose(fp);
    }
    else
    {
        if (strcmp(args[0], "exit") == 0)
        {
            //  check exit and if there is an argument after exit
            if (0 + 1 < num_args && args[0 + 1] != NULL)
            {
                errorHandler();
                return -1;
            }
            else
            {
                exit(0);
            }
        }
        else if (strcmp(args[0], "cd") == 0)
        {
            if ((0 + 1 < num_args && args[0 + 1] == NULL) || (0 + 2 < num_args && strcmp(args[0 + 2], ";")))
            {
                errorHandler();
                return -1;
            }
            // try changing directories
            // chdir() return 0 if it can change, something else otherwise
            int ret = chdir(args[0 + 1]);

            if (ret != 0)
            {
                errorHandler();
                return -1;
            }
            else
            {
                return 1;
            }
        }
        else if (strcmp(args[0], "pwd") == 0)
        {
            char buf[1024];
            if (getcwd(buf, sizeof(buf)) == NULL)
            {
                errorHandler();
                return -1;
            }
            printf("%s\n", buf);
        }

        else if (strcmp(args[0], "loop") == 0)
        {
            int loopNum;
            if (0 + 1 >= num_args)
            {
                errorHandler();
            }
            if (isdigit(*args[0 + 1])) // strlen(str) == strlen( itoa(atoi(str)) <- for numbers greater than 9
            {                          // why dereference it ???
                loopNum = atoi(args[0 + 1]);

                if (loopNum == 0)
                {
                    errorHandler();
                    return 1;
                }
            }
            else
            {
                errorHandler();
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
                    if (process_cmds(&args[0 + 2], num_args - 0 - 2) == -1)
                    {
                        errorHandler();
                    }
                }
            }
        }
        else if (pipeFlag == 1)
        {
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

            pipes(output_array1, output_array2, NULL);

            // Free memory allocated for the output arrays
            free(output_array1);
            free(output_array2);
        }
        else
        {
            return forkredirct(args, num_args, NULL);
        }
    }
    return 0;
}

int read_args_helper(FILE *fp)
{
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
        errorHandler();
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