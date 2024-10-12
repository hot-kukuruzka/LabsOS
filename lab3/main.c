#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

int got_signal = 0;
int global_num = 2;
int id = -1;

void handler(int signum) {
    printf("Process %d got signal %d\n", id, signum);
    got_signal = 1;
}

void siginfo(int signum, siginfo_t *siginfo, void *ucontext) {
    printf("Process %d got signal %d with code %d\n", id, siginfo->si_signo, siginfo->si_code);
    got_signal = 1;
}

void on_exit_handler(int exitcode, void *arg) {
    printf("Process %d finished with code %d and arg %s\n\n", id, exitcode, (char*) arg);
}

void atexit_handler() {
    printf("Process %d finished gracefully\n\n", id);
}

void func() {
    printf("Function call with id = %d, PID:  %d, PPID: %d, GLOBAL_NUM: %d\n", id, getpid(), getppid(), global_num);
    printf("... Increasing global num by %d (program id %d)\n", id, id);
    global_num += id;
    printf("GLOBAL_NUM (from program with id %d): %d\n  ", id, global_num);
    while (id != 0 && !got_signal); // wait for signal in child processes
}

int main() {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        exit(1);
    }

    if (pid == 0) {
        id = 1;
        signal(SIGINT, handler);
        on_exit(on_exit_handler, (void*) "<arg passed successfully>");
        func(); // child process
        exit(1);
    }

    pid_t pid2 = fork();
    if (pid2 < 0) {
        kill(pid, SIGKILL);
        perror("fork (second) failed");
        exit(1);
    }
    if (pid2 == 0) {
        id = 2;
        struct sigaction act = {
                .sa_sigaction = siginfo,
                .sa_flags = SA_SIGINFO,
        };
        sigaction(SIGTERM, &act, NULL);
        atexit(atexit_handler);
        func(); // child process
        exit(0);
    }

    id = 0;
    func(); // master process

    kill(pid, SIGINT);
    waitpid(pid, NULL, 0);
    kill(pid2, SIGTERM);
    waitpid(pid2, NULL, 0);

    printf("Finishing master process\n");

    return 0;
}
