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
#define MY_SET

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
    }
    int n;
    node *head, *tail;
  public:
    struct iterator {
        node* ptr;
        iterator(node* p): ptr(p) {}
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
    iterator begin() const { return head; }
    iterator end() const { return tail; }
    int size() const { return n; }
    bool empty() const { return n == 0; }
    iterator insert(iterator p_target, iterator p_it) {
        auto target = p_target.ptr, it = p_it.ptr;
        if (n == 0) {
            head = tail = it;
        } else {
            link(it, target->next);
            link(target, it);
        }
        if (target == tail) tail = it;
        n++;
        return it;
    }
    iterator insert(iterator target, Value value) {
        auto it = new node{ value };
        return insert(target, it);
    }
    iterator insert_begin(Value value) {
        auto it = new node{ value };
        return insert(begin(), it);
    }
    iterator insert_end(Value value) {
        auto it = new node{ value };
        return insert(end(), it);
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
        return (size_t)key % SIZE;
    }
  public:
    using iterator = typename L::iterator;
    iterator end() const { return nullptr; }
    iterator find(Key key) {
        auto h = hash(key);
        for (auto it = ary[h].begin(); it != nullptr; ++it) {
            if (it->first == key)
                return it;
        }
        return nullptr;
    }
    void emplace(Key key, Value value) {
        auto h = hash(key);
        ary[h].insert_begin(KV{ key, value });
    }
    void erase(Key key) {
        auto it = find(key);
        if (it == nullptr) return;
        auto h = hash(key);
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

#ifdef MY_SET
template<typename Key>
class OrderSet {
    std::vector<Key> v;
  public:
    int size() const { return (int)v.size(); }
    auto begin() { return v.begin(); }
    void insert(Key key) {
        auto it = std::lower_bound(v.begin(), v.end(), key);
        v.insert(it, key);
    }
    void erase(Key key) {
        auto it = std::lower_bound(v.begin(), v.end(), key);
        v.erase(it);
    }
    void erase(typename std::vector<Key>::iterator it) {
        v.erase(it);
    }
    void increase(Key key, Key o) {
        auto it = std::lower_bound(v.begin(), v.end(), key);
        if (it == v.end()) return;
        *it += o;
        while (it != v.end() and *it < *next(it)) {
            std::swap(*it, *next(it));
            it++;
        }
    }
    void clear() {
        v.clear();
    }
};
#else
#include <set>
template<typename Key>
using OrderSet = std::set<Key>;
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
        int freq, seq, page;
        node(int _f, int _s, int _p): freq(_f), seq(_s), page(_p) {}
        bool operator<(const node& o) const {
            if (freq != o.freq)
                return freq < o.freq;
            return seq < o.seq;
        }
        void operator+=(const node& o) {
            freq += o.freq;
            seq = o.seq;
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
            auto& v = it->second;
#ifdef MY_SET
            node n{ 1, total, page };
            st.increase(v, n);
#else
            st.erase(v);
            v.freq++;
            v.seq = total;
            st.insert(v);
#endif
            return true;
        } else {
            if (st.size() == size) {
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

    for (auto policy: policies) {
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
