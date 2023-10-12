#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <sys/wait.h>

double gettime() {
    struct timeval time;
    if (gettimeofday(&time, 0) < 0)
        perror("gettimeofday"), exit(EXIT_FAILURE);
    return (double)time.tv_sec + (double)time.tv_usec / 1e6;
}

void calc(uint32_t n, uint32_t (*A)[n], uint32_t (*B)[n], uint32_t (*C)[n], size_t L, size_t R) {
    for (size_t i = L; i < R; i++) {
        for (size_t j = 0; j < n; j++) {
            uint32_t v = 0;
            for (size_t k = 0; k < n; k++)
                v += A[i][k] * B[k][j];
            C[i][j] = v;
        }
    }
}

uint32_t multiply_matrix(void* ptr, void* shmptr, uint32_t n, size_t m) {
    uint32_t (*A)[n] = ptr;
    uint32_t (*B)[n] = ptr;
    uint32_t (*C)[n] = shmptr;

    for (uint32_t i = 0; i < n; i++)
        for (uint32_t j = 0; j < n; j++)
            A[i][j] = (uint32_t)(i * n + j);

    pid_t pids[m];
    size_t idx[m+1];
    size_t block = n / m, remain = n - block * m;
    idx[0] = 0;
    for (size_t id = 0; id < m; id++)
        idx[id+1] = idx[id] + block + (id < remain);

    for (size_t id = 1; id < m; id++) {
        pid_t pid = pids[id] = fork();
        if (pid < 0)
            perror("fork"), exit(EXIT_FAILURE);
        if (pid == 0) {
            calc(n, A, B, C, idx[id-1], idx[id]);
            exit(EXIT_SUCCESS);
        }
    }

    calc(n, A, B, C, idx[m-1], idx[m]);

    for (size_t id = 1; id < m; id++) {
        pid_t pid = pids[id];
        int wstatus;
        if (waitpid(pid, &wstatus, 0) < 0)
            perror("waitpid"), exit(EXIT_FAILURE);
        if (!WIFEXITED(wstatus) || WEXITSTATUS(wstatus) != 0)
            printf("child %d exit with status %d\n", pid, wstatus), exit(EXIT_FAILURE);
    }

    uint32_t checksum = 0;
    for (uint32_t i = 0; i < n; i++)
        for (uint32_t j = 0; j < n; j++)
            checksum += C[i][j];

    return checksum;
}

int main(void) {
    uint32_t n;

    printf("Input the matrix dimension: ");
    scanf("%u", &n);

    void* ptr = calloc(n * n, sizeof(uint32_t));

    size_t size = n * n * sizeof(uint32_t);
    int shmid = shmget(IPC_PRIVATE, size, IPC_CREAT | 0777);
    if (shmid < 0)
        perror("shmget"), exit(EXIT_FAILURE);
    void* shmptr = shmat(shmid, 0, 0);
    if (shmptr < 0)
        perror("shmat"), exit(EXIT_FAILURE);

    size_t START = 1, END = 16;
    for (size_t m = START; m <= END; m++) {
        printf("Multiplying matrices using %lu process%s\n", m, &"es"[(m == START) * 2]);
        double start = gettime();
        uint32_t checksum = multiply_matrix(ptr, shmptr, n, m);
        double end = gettime();
        printf("Elapsed time: %lf sec, Checksum: %u\n", end - start, checksum);
    }

    if (shmdt(shmptr) < 0)
        perror("shmdt"), exit(EXIT_FAILURE);
    if (shmctl(shmid, IPC_RMID, NULL) < 0)
        perror("shmctl"), exit(EXIT_FAILURE);
    free(ptr);

    return 0;
}
