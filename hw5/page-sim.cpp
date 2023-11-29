#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>

double getsecond() {
    struct timeval time;
    if (gettimeofday(&time, 0) < 0)
        perror("gettimeofday"), exit(EXIT_FAILURE);
    return (double)time.tv_sec + (double)time.tv_usec / 1e6;
}

int main(void) {

    ;

    return EXIT_SUCCESS;
}
