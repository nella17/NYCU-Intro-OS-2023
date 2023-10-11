#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/shm.h>

double gettime() {
    struct timeval time;
    if (gettimeofday(&time, 0) < 0)
        perror("gettimeofday"), exit(EXIT_FAILURE);
    return time.tv_sec + time.tv_usec / 1e6;
}

uint32_t multiply_matrix(void* ptr, void* shmptr, uint32_t n, size_t m) {
    uint32_t (*A)[n] = ptr;
    uint32_t (*B)[n] = ptr;
    uint32_t (*C)[n] = shmptr;

    for (uint32_t i = 0; i < n; i++)
        for (uint32_t j = 0; j < n; j++) {
            A[i][j] = (uint32_t)(i * n + j);
            C[i][j] = 0;
        }

    pid_t pids[m];
    size_t block = (n-1) / m + 1;

    for (size_t id = 1; id < m; id++) {
        pid_t pid = fork();
        if (pid < 0)
            perror("fork"), exit(EXIT_FAILURE);
        if (pid == 0) {
            for (size_t i = block * (id-1); i < block * id; i++) {
                for (size_t k = 0; k < n; k++)
                    for (size_t j = 0; j < n; j++)
                        C[i][j] += A[i][k] * B[k][j];
            }
            exit(EXIT_SUCCESS);
        }
        pids[id] = pid;
    }

    for (size_t i = block * (m-1); i < n; i++)
        for (size_t k = 0; k < n; k++)
            for (size_t j = 0; j < n; j++)
                C[i][j] += A[i][k] * B[k][j];

    for (size_t id = 1; id < m; id++)
        if (waitpid(pids[id], NULL, 0) < 0)
            perror("waitpid"), exit(EXIT_FAILURE);

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

    size_t pagesize = (size_t)sysconf(_SC_PAGESIZE);
    size_t size = n * n * sizeof(uint32_t);
    size = ((size - 1) / pagesize + 1) * pagesize;
    int shmid = shmget(IPC_PRIVATE, size, IPC_CREAT | 0777);
    if (shmid < 0)
        perror("shmget"), exit(EXIT_FAILURE);
    void* shmptr = shmat(shmid, 0, 0);
    if (shmptr < 0)
        perror("shmat"), exit(EXIT_FAILURE);

    size_t START = 1, END = 16;
    for (size_t m = START; m <= END; m++) {
        printf("Multiplying matrices using %lu process\n", m);
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
