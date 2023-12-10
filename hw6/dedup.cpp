#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <openssl/sha.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <filesystem>
#include <iostream>
#include <string>
#include <sstream>
#include <unordered_map>
namespace fs = std::filesystem;
using ustring = std::basic_string<unsigned char>;

template<> struct std::hash<ustring> {
    std::size_t operator()(ustring const& u) const noexcept {
        std::string s((const char*)u.c_str(), u.size());
        return std::hash<std::string>{}(s);
    }
};

ustring sha1(fs::path path) {
    unsigned char md[SHA_DIGEST_LENGTH];
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) throw ((perror("open"), -1));
    struct stat buf;
    if (fstat(fd, &buf) < 0) throw ((perror("fstat"), -1));
    auto size = (size_t)buf.st_size;
    auto mp = (unsigned char*)mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mp == MAP_FAILED) throw ((perror("mmap"), -1));
    close(fd);
    SHA1(mp, size, md);
    if (munmap(mp, size) < 0) throw ((perror("munmap"), -1));
    return { md, SHA_DIGEST_LENGTH };
}

std::string hex(ustring hash) {
    std::stringstream ss;
    for (auto c: hash)
        ss << std::hex << (int)c;
    return ss.str();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
        return EXIT_FAILURE;
    }

    auto directory = argv[1];
    std::unordered_map<ustring, fs::path> mp{};
    for (auto& f: fs::recursive_directory_iterator(directory)) {
        if (!f.is_regular_file()) continue;
        auto h = sha1(f);
        auto it = mp.find(h);
        if (it == mp.end()) {
            mp.emplace(h, f);
        } else {
            assert(fs::remove(f));
            fs::create_hard_link(it->second, f);
        }
    }

    return EXIT_SUCCESS;
}
