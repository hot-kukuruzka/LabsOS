#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

const char* SHARED_MEMORY_NAME = "/shmem_test";
const int SIZE = 32;

int main() {
    int res = 0;

    create_shm:
    int shmfd = shm_open(SHARED_MEMORY_NAME, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (shmfd < 0) {
        shmfd = shm_open(SHARED_MEMORY_NAME, O_RDONLY, 0600);
        if (shmfd >= 0) {
            void *data = mmap(0, SIZE, PROT_READ, MAP_SHARED, shmfd, 0);
            time_t *sender_time = (time_t *) data;

            if (abs(*sender_time - time(NULL)) > 10) {
                shm_unlink(SHARED_MEMORY_NAME);
                munmap(data, SIZE);
                close(shmfd);
                goto create_shm;
            }
        }

        fprintf(stderr, "You're not allowed to start two sender processes\n");
        res = -1;
        goto end;
    }

    ftruncate(shmfd, SIZE);

    void* data = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);

    time_t* sender_time = (time_t*) data;
    pid_t* sender_pid = (pid_t*) (data + sizeof(*sender_time));

    *sender_pid = getpid();

    while (1) {
        *sender_time = time(NULL);
        usleep(100);
    }

    end:
    if (res < 0) {
        fprintf(stderr, "sender failed\n");
        exit(1);
    }

    exit(0);
}
