#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <random>
#include <limits>

const char* infilename  = "input.txt";
const char* outfilename = "output.txt";
typedef int element_t;

int main(int argc, char* argv[]) {
    size_t n = 1e6;
    if (argc >= 2)
        n = strtoul(argv[1], NULL, 10);
    if (argc >= 3)
        infilename = argv[2];
    if (argc >= 4)
        infilename = argv[3];

    printf("Generate %lu numbers to %s / %s\n", n, infilename, outfilename);

    std::vector<element_t> v(n);
    std::random_device rd;
    if (0) {
        std::uniform_int_distribution<element_t> dist(
            std::numeric_limits<element_t>::min(),
            std::numeric_limits<element_t>::max()
        );
        for (auto &x: v) x = dist(rd);
    } else {
        std::iota(v.begin(), v.end(), 1);
        std::shuffle(v.begin(), v.end(), rd);
    }

    FILE* infile = fopen(infilename, "w");
    fprintf(infile, "%lu \n", n);
    for (size_t i = 0; i < n; i++)
        fprintf(infile, "%u%c", v[i], " \n"[i+1==n]);
    fclose(infile);

    std::sort(v.begin(), v.end());

    FILE* outfile = fopen(outfilename, "w");
    for (size_t i = 0; i < n; i++)
        fprintf(outfile, "%u%c", v[i], " \n"[i+1==n]);
    fclose(outfile);

    return EXIT_SUCCESS;
}
