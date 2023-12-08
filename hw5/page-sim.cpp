#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <iostream>
#include <vector>
#include <array>

#define MY_LIST
#define MY_HASH

#ifdef MY_LIST
template<typename Value>
class List {
    struct node {
        Value value;
        node *prev = nullptr, *next = nullptr;
    };
    void link(node* prev, node* next) {
        if (prev) prev->next = next;
        if (next) next->prev = prev;
    }
    void unlink(node* it) {
        link(it->prev, it->next);
        it->prev = it->next = nullptr;
    }
    int n;
    node *head, *tail;
  public:
    struct iterator {
        node* ptr;
        iterator(node* p = nullptr): ptr(p) {}
        Value& operator*() { return ptr->value; }
        Value* operator->() { return &ptr->value; }
        bool operator==(iterator it) const { return ptr == it.ptr; }
        bool operator!=(iterator it) const { return ptr != it.ptr; }
        bool operator!=(std::nullptr_t null) const { return ptr != null; }
        iterator& operator++() {
            ptr = ptr->next;
            return *this;
        }
    };
    using const_iterator = iterator;
    iterator begin() const { return head; }
    iterator end() const { return nullptr; }
    int size() const { return n; }
    bool empty() const { return n == 0; }
    iterator insert(iterator p_target, iterator p_it) {
        auto target = p_target.ptr, it = p_it.ptr;
        if (n == 0) {
            head = tail = it;
        } else {
            if (target == nullptr) {
                link(tail, it);
                tail = it;
            } else {
                link(target->prev, it);
                link(it, target);
                if (target == head) head = it;
            }
        }
        n++;
        return it;
    }
    iterator insert(iterator target, Value value) {
        auto it = new node{ value };
        return insert(target, it);
    }
    void move(iterator target, iterator it) {
        if (target == it) return;
        erase(it, false);
        insert(target, it);
    }
    int front() const {
        return head->value;
    }
    void pop_front() {
        erase(head);
    }
    void erase(iterator p_it, bool del = true) {
        auto it = p_it.ptr;
        if (head == it) head = head->next;
        if (tail == it) tail = tail->prev;
        unlink(it);
        if (del) delete it;
        n--;
    }
    void clear() {
        while (head != nullptr) {
            auto nxt = head->next;
            delete head;
            head = nxt;
        }
        n = 0;
        head = tail = nullptr;
    }
};
#else
#include <list>
template<typename Value>
using List = std::list<Value>;
#endif

#ifdef MY_HASH
template<typename Key, typename Value>
class HashTable {
    static constexpr size_t SIZE = 1024;
    using KV = std::pair<Key, Value>;
    using L = List<KV>;
    std::array<L, SIZE> ary;
    size_t hash(Key key) {
        return (size_t)key * 3 % SIZE;
    }
    size_t h;
  public:
    using iterator = typename L::iterator;
    using const_iterator = typename L::const_iterator;
    const_iterator end() const { return ary[h].end(); }
    iterator find(Key key) {
        h = hash(key);
        for (auto it = ary[h].begin(); it != ary[h].end(); ++it) {
            if (it->first == key)
                return it;
        }
        return ary[h].end();
    }
    void emplace(Key key, Value value) {
        h = hash(key);
        ary[h].insert(ary[h].begin(), KV{ key, value });
    }
    void erase(Key key) {
        auto it = find(key);
        if (it == ary[h].end()) return;
        ary[h].erase(it);
    }
    void clear() {
        for (size_t i = 0; i < SIZE; i++)
            ary[i].clear();
    }
};
#else
#include <unordered_map>
template<typename Key, typename Value>
using HashTable = std::unordered_map<Key, Value>;
#endif

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
    void init(int p_size) {
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
        int freq, page;
        node(int _f, int _p): freq(_f), page(_p) {}
        bool operator!=(const node& o) const {
            return page != o.page;
        }
    };
    using L = List<node>;
    using Lit = L::iterator;
    L list;
    HashTable<int, Lit> mp{};
    LFU(): Policy("LFU") {}
    virtual ~LFU() = default;
    void _init(int size) override {
        list.clear();
        mp.clear();
    }
    Lit _find_pos(Lit kt, int freq) {
        while (kt != list.end() and kt->freq <= freq)
            ++kt;
        return kt;
    }
    bool _access(int page) override {
        auto it = mp.find(page);
        if (it != mp.end()) {
            auto& jt = it->second;
            auto freq = jt->freq;
            auto kt = _find_pos(jt, freq+1);
#ifdef MY_LIST
            list.erase(jt, false);
            jt->freq++;
            list.insert(kt, jt);
#else
            node o = *jt;
            o.freq++;
            list.erase(jt);
            it->second = list.insert(kt, o);
#endif
            return true;
        } else {
            node n(1, page);
            auto jt = list.begin();
            if (list.size() == size) {
                mp.erase(jt->page);
                list.erase(jt);
            }
            auto kt = _find_pos(list.begin(), 1);
            jt = list.insert(kt, n);
            mp.emplace(page, jt);
            return false;
        }
    }
};
std::ostream& operator<<(std::ostream& os, LFU::node n) {
    return os << n.freq << ' ' << n.page;
}

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
#ifdef MY_LIST
            list.move(list.end(), it->second);
#else
            list.erase(it->second);
            it->second = list.insert(list.end(), page);
#endif
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

class File {
  public:
    char* mp;
    size_t size, cur;

    File(const char* filename) {
        int fd = open(filename, O_RDONLY);
        if (fd < 0) throw ((perror("open"), -1));
        struct stat buf;
        if (fstat(fd, &buf) < 0) throw ((perror("fstat"), -1));
        size = (size_t)buf.st_size;
        cur = 0;
        mp = (char*)mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (mp == MAP_FAILED) throw ((perror("mmap"), -1));
        close(fd);
    }
    ~File() {
        if (munmap(mp, size) < 0) throw ((perror("munmap"), -1));
    }
    int get() {
        if (cur >= size) return -1;
        int k = 0;
        while (cur < size and !('0' <= mp[cur] and mp[cur] <= '9'))
            cur++;
        if (cur >= size) return -1;
        while (cur < size and ('0' <= mp[cur] and mp[cur] <= '9')) {
            k = k * 10 + mp[cur] - '0';
            cur++;
        }
        return k;
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

    bool first = true;
    for (auto policy: policies) {
        if (first) first = false;
        else printf("\n");
        printf("%s policy:\n", policy->name);
        printf("Frame\tHit\t\tMiss\t\tPage fault ratio\n");
        auto start_time = getsecond();
        for (int size: { 64, 128, 256, 512 }) {
            File in(filename);
            int page;
            policy->init(size);
            while ((page = in.get()) != -1)
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
