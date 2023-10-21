#include <stdio.h>
#include <stdlib.h>
#include <algorithm>
#include <random>
#include <limits>

const char* infilename  = "input.txt";
const char* outfilename = "output.txt";

int main(int argc, char* argv[]) {
    size_t n = 1e6;
    if (argc >= 2)
        n = strtoul(argv[1], NULL, 10);
    if (argc >= 3)
        infilename = argv[2];
    if (argc >= 4)
        infilename = argv[3];

    printf("Generate %lu numbers to %s / %s\n", n, infilename, outfilename);

    std::random_device rd;
    std::uniform_int_distribution<uint32_t> dist(
        std::numeric_limits<uint32_t>::min(),
        std::numeric_limits<uint32_t>::max()
    );
    std::vector<uint32_t> v(n);
    for (auto &x: v) x = dist(rd);

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
