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

pthread_mutex_t m;
pthread_cond_t cv;

void* reader_thread(void* arg) {
    int id = *((int*) arg);
    pthread_t tid = pthread_self();
    while (run_flag) {
        pthread_mutex_lock(&m);
        pthread_cond_wait(&cv, &m);
        printf("id: %d, tid: %lu, shared_data: %s\n", id, tid, shared_data);
        pthread_mutex_unlock(&m);
    }

    return NULL;
}

void sigint_handler(int signum) { run_flag = 0; }

int main() {
    int tid_int[N_THR];
    pthread_t tid[N_THR];

    pthread_mutex_init(&m, NULL);
    pthread_cond_init(&cv, NULL);

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
        pthread_mutex_lock(&m);
        snprintf(shared_data, sizeof(shared_data) / sizeof(*shared_data), "%d", counter);
        pthread_cond_broadcast(&cv);
        pthread_mutex_unlock(&m);
        counter++;
        usleep(10000);
    }

    pthread_cond_broadcast(&cv);
    pthread_cond_destroy(&cv);

    for (int i = 0; i < N_THR; i++) {
        pthread_join(tid[i], NULL);
    }

    pthread_mutex_destroy(&m);
    return 0;
}
