
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>
int pipes(char **args1, char **args2)
{
    int pipefd[2];
    pipe(pipefd);
    int pid = fork();

    if (pid == 0)
    {
        dup2(pipefd[0], 0);
        close(pipefd[1]);
        execv(args2[0], args2);
    }
    else
    {
        dup2(pipefd[1], 1);
        close(pipefd[0]);
        execv(args1[0], args1);
        perror(args1[0]);
    }
    return 0;
}
int main(){
    char * cat1[] = {"/bin/cat","file.txt",NULL};
    char * cat2[] = {"/bin/grep","ap",NULL};
    pipes(cat1, cat2);

}