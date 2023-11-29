#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <list>

template<typename Key, typename Value>
using HashTable = std::unordered_map<Key, Value>;
template<typename Key>
using OrderSet = std::set<Key>;
template<typename Value>
using List = std::list<Value>;

double getsecond() {
    struct timeval time;
    if (gettimeofday(&time, 0) < 0)
        perror("gettimeofday"), exit(EXIT_FAILURE);
    return (double)time.tv_sec + (double)time.tv_usec / 1e6;
}

class Policy {
  public:
    int size, hit, miss, total;
    const char* const name;
    Policy(const char* p_name): name(p_name) {}
    void init(size_t p_size) {
        size = p_size;
        hit = miss = total = 0;
        _init(p_size);
    }
    void access(int page) {
        total++;
        if (_access(page))
            hit++;
        else
            miss++;
    }
    virtual ~Policy() = default;
    virtual void _init(int) {}
    virtual bool _access(int) { return false; }
    double fault() {
        return double(miss) / double(total);
    }
};

class LFU : public Policy {
  public:
    struct node {
        int freq, seq, page;
        node(int _f, int _s, int _p): freq(_f), seq(_s), page(_p) {}
        bool operator<(const node& o) const {
            if (freq != o.freq)
                return freq < o.freq;
            return seq < o.seq;
        }
    };
    using SET = OrderSet<node>;
    SET st{};
    HashTable<int, node> mp{};
    LFU(): Policy("LFU") {}
    virtual ~LFU() = default;
    void _init(int size) override {
        st.clear();
        mp.clear();
    }
    bool _access(int page) override {
        auto it = mp.find(page);
        if (it != mp.end()) {
            st.erase(it->second);
            it->second.freq++;
            it->second.seq = total;
            st.insert(it->second);
            return true;
        } else {
            if ((int)st.size() == size) {
                auto jt = st.begin();
                mp.erase(jt->page);
                st.erase(jt);
            }
            node n(1, total, page);
            st.insert(n);
            mp.emplace(page, n);
            return false;
        }
    }
};

class LRU : public Policy {
  public:
    List<int> list;
    HashTable<int, List<int>::iterator> mp{};
    LRU(): Policy("LRU") {}
    virtual ~LRU() = default;
    void _init(int size) override {
        list.clear();
        mp.clear();
    }
    bool _access(int page) override {
        auto it = mp.find(page);
        if (it != mp.end()) {
            list.erase(it->second);
            it->second = list.insert(list.end(), page);
            return true;
        } else {
            if ((int)list.size() == size) {
                mp.erase(list.front());
                list.pop_front();
            }
            mp.emplace(page, list.insert(list.end(), page));
            return false;
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2)
        return EXIT_FAILURE;

    auto filename = argv[1];
    std::vector<Policy*> policies{
        new LFU,
        new LRU
    };

    for (auto policy: policies) {
        printf("%s policy:\n", policy->name);
        printf("Frame\tHit\t\tMiss\t\tPage fault ratio\n");
        auto start_time = getsecond();
        for (int size: { 64, 128, 256, 512 }) {
            std::ifstream in(filename);
            int page;
            policy->init(size);
            while (in >> page)
                policy->access(page);
            printf("%d\t%d\t\t%d\t\t%.10f\n", size, policy->hit, policy->miss, policy->fault());
        }
        auto end_time = getsecond();
        printf("Total elapsed time %.4f sec\n", end_time - start_time);
    }

    for (auto policy: policies)
        delete policy;

    return EXIT_SUCCESS;
}
