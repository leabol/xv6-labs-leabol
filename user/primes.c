#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winfinite-recursion"

void primes_fork(int fd)
{
    int prime;
    int n = read(fd, &prime, sizeof(int));
    if (n == 0) {
        close(fd);
        exit(0);
    }
    if (n != sizeof(int)) {
        close(fd);
        exit(1);
    }

    fprintf(1, "prime %d\n", prime);

    int mpipe[2];
    if (pipe(mpipe) < 0) {
        close(fd);
        exit(1);
    }

    int pid = fork();
    if (pid < 0) {
        close(fd);
        close(mpipe[0]);
        close(mpipe[1]);
        exit(1);
    }

    if (pid == 0) {
        close(fd);
        close(mpipe[1]);
        primes_fork(mpipe[0]);
        exit(0);
    }

    close(mpipe[0]);
    int num;
    while ((n = read(fd, &num, sizeof(int))) == sizeof(int)) {
        if (num % prime != 0) {
            if (write(mpipe[1], &num, sizeof(int)) != sizeof(int)) {
                close(fd);
                close(mpipe[1]);
                wait(0);
                exit(1);
            }
        }
    }

    close(fd);
    close(mpipe[1]);
    if (n < 0) {
        wait(0);
        exit(1);
    }
    wait(0);
    exit(0);
}

#pragma GCC diagnostic pop

int
main(int argc, char* argv[])
{
    if (argc > 1){
        fprintf(2, "Usage: primes\n");
        exit(0);
    }

    int p[2];
    if(pipe(p) < 0){
        exit(1);
    }

    if (fork() == 0){
        close(p[1]);
        primes_fork(p[0]);
    }else{
        close(p[0]);
        for (int i = 2; i <= 35; i++){
           if(write(p[1], &i, sizeof(int)) < 0) {
                fprintf(2, "first write error\n");
                close(p[1]);
                exit(1);
           }
        }
        close(p[1]);
        wait(0);
        exit(0);
    }
    exit(0);
}