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
#define MAX_TIME 25
char buf[MAX_BUF];

char* format_time(time_t time) {
    char* buf = calloc(MAX_TIME, sizeof(*buf));

    struct tm* time_info = localtime(&time);
    strftime(buf, MAX_TIME, "%d.%m.%Y %H:%M:%S", time_info);

    return buf;
}

// PIPES

int child4pipe(int rfd) {
    sleep(5);

    int res = 0;
    time_t cur_time = time(NULL);
    if (read(rfd, buf, MAX_BUF) < 0) {
        res = -1;
        goto end;
    }

    char* cur_time_format = format_time(cur_time);

    printf("-> Child\n");
    printf("  Current proccess: time = %s, PID = %d, PPID = %d\n", cur_time_format, getpid(), getppid());
    printf("  From sender: %s\n", buf);

    free(cur_time_format);

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

    char* cur_time_format = format_time(time(NULL));

    int n = snprintf(buf, MAX_BUF, "time = %s, PID = %d", cur_time_format, getpid());
    n++; // '\0'
    if (write(fd[1], buf, n) != n) {
        res = -1;
        goto end;
    }
    close(fd[1]);

    free(cur_time_format);

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

    char* cur_time_format = format_time(time(NULL));

    printf("-> Child\n");
    printf("  Current proccess: time = %s, PID = %d, PPID = %d\n", cur_time_format, getpid(), getppid());
    printf("  From sender: %s\n", buf);

    free(cur_time_format);

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

    char* cur_time_format = format_time(time(NULL));
    int n = snprintf(buf, MAX_BUF, "time = %s, PID = %d", cur_time_format, getpid());
    n++; // '\0'
    if (write(fd, buf, n) != n) {
        res = -1;
        goto end;
    }
    free(cur_time_format);

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
