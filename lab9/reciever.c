#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/mman.h>

const char* SHARED_MEMORY_NAME = "/lab9";
const int SIZE = 32;

int main() {
    int res = 0;

    int shmfd = shm_open(SHARED_MEMORY_NAME, O_RDONLY, 0600);
    if (shmfd < 0) {
        res = -1;
        goto end;
    }

    void* data = mmap(0, SIZE, PROT_READ, MAP_SHARED, shmfd, 0);

    const time_t* sender_time = (time_t*) data;
    const pid_t* sender_pid = (pid_t*) (data + sizeof(*sender_time));

    while (1) {
        printf("Current time: %ld, current pid: %d\nSender time: %ld, sender_pid: %d\n\n", time(NULL), getpid(),
               *sender_time, *sender_pid);
        sleep(1);
    }

end:
    if (res < 0) {
        fprintf(stderr, "reciever failed\n");
        exit(1);
    }

    exit(0);
}
