#include <stdio.h>
#include <stdlib.h>
#include <spawn.h>
#include <sys/wait.h>


int main(int argc, char* argv[])
{
    pid_t pid;
    char test[] = "child";
    char *default_argv[] = {"/bin/mp_server","20009",  NULL};

    int status;
    while(1) {
        status = posix_spawn(&pid,"/bin/mp_server",NULL,NULL,default_argv,NULL);
        if (status==0) {
            printf("Child pid:%d\n",pid);
            if (waitpid(pid,&status,0)!=-1) {
                printf("Child exited with status %d \n",status);
            } else {
                perror("waitpid");
            }
        } else {
            printf("posix /bin/mp_server spawn error\n");
        }
        sleep(30);
    }
}
