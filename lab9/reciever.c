#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/sem.h>
#include <sys/mman.h>

const char* SHARED_MEMORY_NAME = "/lab9";
const int SIZE = 32;
const int SEMKEY = 9;
const int SEMSZ = 2;
int in_lock = 0;
int semid = 0;

void sigint_handler(int signum) {
    if (in_lock) {
        struct sembuf sb = {
            .sem_num = 1,
            .sem_op = 1,
            .sem_flg = 0,
        };

        semop(semid, &sb, 1);
    }

    exit(0);
}

int main() {
    int res = 0;
    struct sembuf sb;

    int shmfd = shm_open(SHARED_MEMORY_NAME, O_RDONLY, 0600);
    if (shmfd < 0) {
        res = -1;
        goto end;
    }

    semid = semget(SEMKEY, SEMSZ, S_IRUSR | S_IWUSR);
    if (semid < 0) {
        fprintf(stderr, "Failed to get semaphore. Check if sender process is running \n");
        res = -1;
        goto end;
    }

    void* data = mmap(0, SIZE, PROT_READ, MAP_SHARED, shmfd, 0);

    const time_t* sender_time = (time_t*) data;
    const pid_t* sender_pid = (pid_t*) (data + sizeof(*sender_time));

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

        printf("Current time: %ld, current pid: %d\nSender time: %ld, sender_pid: %d\n\n", time(NULL), getpid(),
               *sender_time, *sender_pid);

        sb.sem_op *= -1;
        res = semop(semid, &sb, 1);
        if (res < 0) {
            fprintf(stderr, "Failed to unlock semaphore\n");
            res = -1;
            goto end;
        }
        in_lock = 0;
        sb.sem_op *= -1;

        sleep(1);
    }

end:
    if (res < 0) {
        fprintf(stderr, "reciever failed\n");
        exit(1);
    }

    exit(0);
}
