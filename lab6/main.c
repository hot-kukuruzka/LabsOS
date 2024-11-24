#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>

#define MAX_BUF 1024
char buf[MAX_BUF];

// PIPES

int child4pipe(int rfd) {
    sleep(5);

    int res = 0;
    time_t cur_time = time(NULL);
    if (read(rfd, buf, MAX_BUF) < 0) {
        res = -1;
        goto end;
    }

    printf("-> Child\n");
    printf("  Current proccess: timestamp = %ld, PID = %d, PPID = %d\n", cur_time, getpid(), getppid());
    printf("  From sender: %s\n", buf);

end:
    if (res < 0) {
        fprintf(stderr, "child4pipe failed\n");
        exit(1);
    }

    exit(0);
}

void test_pipes() {
    int res = 0;

    int fd[2];
    if (pipe(fd) < 0) {
        res = -1;
        goto end;
    }

    int n = snprintf(buf, MAX_BUF, "timestamp = %ld, PID = %d", time(NULL), getpid());
    n++; // '\0'
    if (write(fd[1], buf, n) != n) {
        res = -1;
        goto end;
    }
    close(fd[1]);

    pid_t child_pid = fork();
    if (child_pid < 0) {
        res = -1;
        goto end;
    }

    if (child_pid == 0) {
        memset(buf, 0, MAX_BUF); // buf is shared with child
        child4pipe(fd[0]);
        exit(42); // we should never get there
    }
    close(fd[0]);

    printf("-> Parent\n");
    printf("  %s, Child PID = %d\n", buf, child_pid);

end:
    if (res < 0) {
        fprintf(stderr, "test_pipe failed\n");
        exit(1);
    }

    // Wait for child to finish test;
    waitpid(child_pid, NULL, 0);
}

// FIFO
const char* FIFO_NAME = "/tmp/fifo_test";

int child4fifo() {
    int res = 0;

    int rfd;
    if ((rfd = open(FIFO_NAME, O_RDONLY)) < 0) {
        res = -1;
        goto end;
    };

    sleep(5);

    time_t cur_time = time(NULL);
    if (read(rfd, buf, MAX_BUF) < 0) {
        res = -1;
        goto end;
    }

    printf("-> Child\n");
    printf("  Current proccess: timestamp = %ld, PID = %d, PPID = %d\n", cur_time, getpid(), getppid());
    printf("  From sender: %s\n", buf);

    end:
    if (res < 0) {
        fprintf(stderr, "child4fifo failed\n");
        exit(1);
    }

    exit(0);
}

void test_fifo() {
    int res = 0;

    if (mkfifo(FIFO_NAME, 0600) < 0) {
        res = -1;
        goto end;
    }

    pid_t child_pid = fork();
    if (child_pid < 0) {
        res = -1;
        goto end;
    }

    if (child_pid == 0) {
        child4fifo();
        exit(42); // we should never get there
    }

    int fd;
    if ((fd = open(FIFO_NAME, O_WRONLY)) < 0) {
        goto end;
    }

    int n = snprintf(buf, MAX_BUF, "timestamp = %ld, PID = %d", time(NULL), getpid());
    n++; // '\0'
    if (write(fd, buf, n) != n) {
        res = -1;
        goto end;
    }

    printf("-> Parent\n");
    printf("  %s, Child PID = %d\n", buf, child_pid);

    end:
    if (res < 0) {
        fprintf(stderr, "test_fifo failed\n");
        exit(1);
    }

    // Wait for child to finish test;
    waitpid(child_pid, NULL, 0);
    remove(FIFO_NAME); // Remove FIFO
}

int main() {
    printf("<=== PIPES ===>\n");
    test_pipes();
    printf("\n\n");

    printf("<=== FIFO ===>\n");
    test_fifo();
    return 0;
}
