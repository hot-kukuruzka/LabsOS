#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <pthread.h>

#define N_THR 10
#define LEN 20

char shared_data[LEN];
int run_flag = 1;
pthread_rwlock_t rwlock;

void* reader_thread(void* arg) {
    int id = *((int*) arg);
    pthread_t tid = pthread_self();
    while (run_flag) {
        pthread_rwlock_rdlock(&rwlock);
        printf("id: %d, tid: %lu, shared_data: %s\n", id, tid, shared_data);
        pthread_rwlock_unlock(&rwlock);
        usleep(1000);
    }

    return NULL;
}

void sigint_handler(int signum) { run_flag = 0; }

int main() {
    int tid_int[N_THR];
    pthread_t tid[N_THR];

    pthread_rwlock_init(&rwlock, NULL);

    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    int res = sigaction(SIGINT, &sa, NULL);
    if (res < 0) {
        fprintf(stderr, "failed to set sigaction");
        return 1;
    }

    for (int i = 0; i < N_THR; i++) {
        tid_int[i] = i + 1;
        int res = pthread_create(&tid[i], NULL, reader_thread, (void*) &tid_int[i]);
        if (res < 0) {
            fprintf(stderr, "failed to start %d reader thread\n", i + 1);
            return 1;
        }
    }

    int counter = 0;
    while (run_flag) {
        pthread_rwlock_wrlock(&rwlock);
        snprintf(shared_data, sizeof(shared_data) / sizeof(*shared_data), "%d", counter);
        pthread_rwlock_unlock(&rwlock);
        counter++;
        usleep(10000);
    }

    for (int i = 0; i < N_THR; i++) {
        pthread_join(tid[i], NULL);
    }

    pthread_rwlock_destroy(&rwlock);
    return 0;
}
