#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <time.h>
#include <unistd.h>

const char* SHARED_MEMORY_NAME = "/lab9";
const int SIZE = 32;
const int SEMKEY = 9;
const int SEMSZ = 2;
int in_lock = 0;
int semid = 0;

void sigint_handler(int signum) {
    struct sembuf sb = {
        .sem_num = 0,
        .sem_op = 1,
        .sem_flg = 0,
    };

    semop(semid, &sb, 1);

    if (in_lock) {
        sb.sem_num = 1;

        semop(semid, &sb, 1);
    }

    exit(0);
}

int main() {
    int res = 0;
    struct sembuf sb;

    semid = semget(SEMKEY, SEMSZ, S_IRUSR | S_IWUSR);
    if (semid < 0) {
        if (errno != ENOENT) {
            fprintf(stderr, "Failed to get semaphore\n");
            res = -1;
            goto end;
        }

        semid = semget(SEMKEY, SEMSZ, IPC_EXCL | IPC_CREAT | S_IRUSR | S_IWUSR);
        if (semid < 0) {
            fprintf(stderr, "Failed to create semaphore\n");
            res = -1;
            goto end;
        }

        sb = (struct sembuf) {
            .sem_num = 0,
            .sem_op = 1,
            .sem_flg = 0,
        };

        res = semop(semid, &sb, 1);
        if (res < 0) {
            fprintf(stderr, "Failed to create semaphore\n");
            res = -1;
            goto end;
        }

        sb = (struct sembuf) {
            .sem_num = 1,
            .sem_op = 1,
            .sem_flg = 0,
        };

        res = semop(semid, &sb, 1);
        if (res < 0) {
            fprintf(stderr, "Failed to create semaphore\n");
            res = -1;
            goto end;
        }
    }

    sb = (struct sembuf) {
        .sem_num = 0,
        .sem_op = -1,
        .sem_flg = IPC_NOWAIT,
    };

    res = semop(semid, &sb, 1);
    if (res < 0) {
        fprintf(stderr, "You're not allowed to start two sender processes\n");
        res = -1;
        goto end;
    }

    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    res = sigaction(SIGINT, &sa, NULL);
    if (res < 0) {
        goto end;
    }

    int shmfd = shm_open(SHARED_MEMORY_NAME, O_CREAT | O_RDWR, 0600);
    if (shmfd < 0) {
        fprintf(stderr, "Failed to open shmem\n");
        res = -1;
        goto end;
    }

    ftruncate(shmfd, SIZE);

    void* data = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);

    time_t* sender_time = (time_t*) data;
    pid_t* sender_pid = (pid_t*) (data + sizeof(*sender_time));

    *sender_pid = getpid();
    sb = (struct sembuf) {
        .sem_num = 1,
        .sem_op = -1,
    };

    while (1) {
        res = semop(semid, &sb, 1);
        if (res < 0) {
            fprintf(stderr, "Failed to lock semaphore\n");
            res = -1;
            goto end;
        }
        in_lock = 1;

        *sender_time = time(NULL);

        sb.sem_op *= -1;
        res = semop(semid, &sb, 1);
        if (res < 0) {
            fprintf(stderr, "Failed to unlock semaphore\n");
            res = -1;
            goto end;
        }
        in_lock = 0;
        sb.sem_op *= -1;

        usleep(100);
    }

end:
    if (res < 0) {
        fprintf(stderr, "sender failed\n");
        exit(1);
    }

    exit(0);
}
