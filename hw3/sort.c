#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <semaphore.h>
#include <pthread.h>

const char infilename[] = "input.txt";
const char outfileformat[]  = "output_%lu.txt";
const size_t START = 1, END = 8;

double getsecond() {
    struct timeval time;
    if (gettimeofday(&time, 0) < 0)
        perror("gettimeofday"), exit(EXIT_FAILURE);
    return (double)time.tv_sec + (double)time.tv_usec / 1e6;
}

double getms() {
    return getsecond() / 1e3;
}

void process(size_t cnt, uint32_t *ary) {
    // TODO
}

int main(void) {
    FILE* infile = fopen(infilename, "r");
    if (infile == NULL)
        perror("fopen(in)"), exit(EXIT_FAILURE);
    size_t n;
    fscanf(infile, "%lu", &n);
    size_t size = n * sizeof(uint32_t);
    uint32_t *initary = malloc(size);
    for (size_t i = 0; i < n; i++)
        fscanf(infile, "%u", initary+i);
    if (fclose(infile) < 0)
        perror("fclose(in)"), exit(EXIT_FAILURE);
    uint32_t *workary = malloc(size);

    for (size_t cnt = START; cnt <= END; cnt++) {
        double start = getms();

        memcpy(workary, initary, size);
        process(cnt, workary);

        char* outfilename;
        asprintf(&outfilename, outfileformat, cnt);
        FILE* outfile = fopen(outfilename, "w");
        if (outfile == NULL)
            perror("fopen(out)"), exit(EXIT_FAILURE);
        for (size_t i = 0; i < n; i++)
            fprintf(outfile, "%u%c", workary[i], " \n"[i+1==n]);
        if (fclose(outfile) < 0)
            perror("fclose(out)"), exit(EXIT_FAILURE);
        free(outfilename);

        double end = getms();
        printf("worker thread #%lu, elapsed %lf ms\n", cnt, end - start);
    }

    free(initary);
    free(workary);

    return EXIT_SUCCESS;
}
