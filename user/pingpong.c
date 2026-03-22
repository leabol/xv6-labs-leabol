#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char* argv[])
{
    if (argc > 1){
        fprintf(2, "Usage: pingpong\n");
        exit(1);
    }

    int ptoc_pipe[2];
    int ctop_pipe[2];
    pipe(ptoc_pipe);
    pipe(ctop_pipe);

    if (fork() == 0){
        //child 
        close(ptoc_pipe[1]);
        close(ctop_pipe[0]);

        int cpid = getpid();
        char buff[10];
        if (read(ptoc_pipe[0], buff, sizeof(buff)) < 0){
            fprintf(2, "child read error\n");
            exit(1);
        }
        fprintf(1, "%d: received ping\n", cpid);

        strcpy(buff, "pong");
        if(write(ctop_pipe[1], buff, sizeof(buff)) < 0){
            fprintf(2, "child write error\n");
            exit(1);
        }
        exit(0);
    }else{
        close(ptoc_pipe[0]);
        close(ctop_pipe[1]);

        int pid = getpid();
        char buff[10] = "ping";
        if (write(ptoc_pipe[1], buff, sizeof(buff)) < 0){
            fprintf(2, "parent write error\n");
            exit(1);
        }
        if(read(ctop_pipe[0], buff, sizeof(buff)) < 0){
            fprintf(2, "parent read error\n");
            exit(1);
        }
        fprintf(1, "%d: received pong\n", pid);
        exit(0);
    }
}