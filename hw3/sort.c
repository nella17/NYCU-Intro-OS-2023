#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <semaphore.h>
#include <pthread.h>
#include <errno.h>
#include <assert.h>

// #define DEBUG

const char infilename[] = "input.txt";
const char outfileformat[]  = "output_%lu.txt";
#ifdef DEBUG
const size_t START = 1, END = 1;
#else
const size_t START = 1, END = 8;
#endif
const size_t K = 8;
typedef int element_t;

double getsecond() {
    struct timeval time;
    if (gettimeofday(&time, 0) < 0)
        perror("gettimeofday"), exit(EXIT_FAILURE);
    return (double)time.tv_sec + (double)time.tv_usec / 1e6;
}

double getms() {
    return getsecond() * 1e3;
}

typedef enum {
    EMPTY = 0,
    HEAD,
    TAIL,
    SORT,
    MERGE,
} item_type;

typedef struct item item_t;
struct item {
    item_type type;
    item_t *prev, *next;
    size_t id;
    element_t *begin, *mid, *end;
};
item_t* item_new(item_type type) {
    item_t* it = malloc(sizeof(item_t));
    memset(it, 0, sizeof(item_t));
    it->type = type;
    return it;
}
void item_link(item_t *l, item_t *r) {
    l->next = r;
    r->prev = l;
}

typedef struct queue {
    sem_t queue, lock;
    item_t *head, *tail;
} queue_t;

void queue_push(queue_t* queue, item_t* it) {
    if (sem_wait(&queue->lock) < 0)
        perror("sem_wait"), exit(EXIT_FAILURE);
    item_link(queue->tail->prev, it);
    item_link(it, queue->tail);
    sem_post(&queue->queue);
    sem_post(&queue->lock);
}

item_t* queue_pop(queue_t* queue) {
    if (sem_wait(&queue->queue) < 0)
        perror("sem_wait"), exit(EXIT_FAILURE);
    if (sem_wait(&queue->lock) < 0)
        perror("sem_wait"), exit(EXIT_FAILURE);
    item_t* it = queue->head->next;
    if (it == queue->tail) {
        it = NULL;
    } else {
        item_link(it->prev, it->next);
        it->prev = it->next = NULL;
    }
    sem_post(&queue->lock);
    return it;
}

void queue_init(queue_t* queue) {
    memset(queue, 0, sizeof(queue_t));
    if (sem_init(&queue->queue, 0, 0) < 0)
        perror("sem_init"), exit(EXIT_FAILURE);
    if (sem_init(&queue->lock, 0, 1) < 0)
        perror("sem_init"), exit(EXIT_FAILURE);
    queue->head = item_new(HEAD);
    queue->tail = item_new(TAIL);
    item_link(queue->head, queue->tail);
}

void queue_destroy(queue_t* queue) {
    if (sem_destroy(&queue->queue) < 0)
        perror("sem_destroy"), exit(EXIT_FAILURE);
    if (sem_destroy(&queue->lock) < 0)
        perror("sem_destroy"), exit(EXIT_FAILURE);
    for (item_t *it = queue->head, *nxt; it != NULL; it = nxt) {
        nxt = it->next;
        free(it);
    }
}

typedef struct info {
    queue_t task, complete;
} info_t;

void info_init(info_t* info) {
    memset(info, 0, sizeof(info_t));
    queue_init(&info->task);
    queue_init(&info->complete);
}

void info_destroy(info_t* info) {
    queue_destroy(&info->task);
    queue_destroy(&info->complete);
}

void bubble_sort(element_t* begin, element_t* end) {
    for (element_t* it = begin+1; it < end; it++) {
        for (element_t* jt = it; begin < jt; jt--) {
            element_t l = *(jt-1), r = *jt;
            if (l > r) {
                *(jt-1) = r;
                *jt = l;
            }
        }
    }
}

void merge(element_t* begin, element_t* mid, element_t* end) {
    size_t n = (size_t)(end - begin);
    element_t ary[n];
    element_t *kt = ary;
    element_t *it = begin, *jt = mid;
    while (it != mid && jt != end) {
        element_t l = *it, r = *jt;
        if (l <= r) {
            *(kt++) = l;
            it++;
        } else {
            *(kt++) = r;
            jt++;
        }
    }
    if (it != mid)
        memcpy(kt, it, sizeof(element_t) * (size_t)(mid - it));
    else if (jt != end)
        memcpy(kt, jt, sizeof(element_t) * (size_t)(end - jt));
    memcpy(begin, ary, n * sizeof(element_t));
}

void* worker(void* ptr) {
    info_t* info = (info_t*)ptr;

    for (;;) {
        item_t* item = queue_pop(&info->task);
#ifdef DEBUG
        printf(" task %lu\n ", item->id);
        for (element_t* it = item->begin; it < item->end; it++)
            printf("%d%c", *it, " \n"[it+1==item->end]);
#endif
        switch (item->type) {
        case SORT:
#ifdef DEBUG
            printf(" bubble_sort\n");
#endif
            bubble_sort(item->begin, item->end);
            break;
        case MERGE:
#ifdef DEBUG
            printf(" merge\n");
#endif
            merge(item->begin, item->mid, item->end);
            break;
        default:
            continue;
        }
#ifdef DEBUG
        printf(" ");
        for (element_t* it = item->begin; it < item->end; it++)
            printf("%d%c", *it, " \n"[it+1==item->end]);
#endif
        queue_push(&info->complete, item);
    }

    return ptr;
}

void process(size_t cnt, size_t n, element_t *ary) {
    info_t* info = malloc(sizeof(info_t));
    info_init(info);

    pthread_t *tids = calloc(cnt, sizeof(pthread_t));
    if (tids == NULL)
        perror("calloc"), exit(EXIT_FAILURE);

    size_t idx[K+1];
    idx[0] = 0;
    for (size_t i = 1; i <= K; i++)
        idx[i] = n * i / K;

    for (size_t i = 0; i < K; i++) {
        item_t* it = item_new(SORT);
        it->begin = ary + idx[i];
        it->end = ary + idx[i+1];
        it->id = K + i;
        queue_push(&info->task, it);
    }

    int s;

    for (size_t i = 0; i < cnt; i++) {
        s = pthread_create(&tids[i], NULL, &worker, info);
        if (s != 0 && (errno = s))
            perror("pthread_create"), exit(EXIT_FAILURE);
    }

    int complete[K*2];
    for (size_t i = 1; i < K*2; i++)
        complete[i] = 0;
    for (;;) {
        item_t* it = queue_pop(&info->complete);
        size_t id = it->id;
        free(it);
        complete[id] = 1;
        if (id == 1) break;
        if (complete[id^1]) {
            size_t i = id / 2;
            size_t l = i * 2, r = i * 2 + 1;
            while (r*2+1 < 2*K) {
                l = l * 2;
                r = r * 2 + 1;
            }
            l = l-K, r = r-K+1;
            size_t m = (l + r) / 2;
            item_t* jt = item_new(MERGE);
#ifdef DEBUG
            printf("merge %lu: %lu %lu %lu\n", i, l, m, r);
#endif
            jt->begin = ary + idx[l];
            jt->mid   = ary + idx[m];
            jt->end   = ary + idx[r];
            jt->id = i;
            queue_push(&info->task, jt);
        }
    }

    for (size_t i = 0; i < cnt; i++) {
        s = pthread_cancel(tids[i]);
        if (s != 0 && (errno = s))
            perror("pthread_cancel"), exit(EXIT_FAILURE);
        void* res;
        s = pthread_join(tids[i], &res);
        assert(res == PTHREAD_CANCELED);
    }

    free(tids);
    info_destroy(info);
    free(info);
}

int main(void) {
    FILE* infile = fopen(infilename, "r");
    if (infile == NULL)
        perror("fopen(in)"), exit(EXIT_FAILURE);
    size_t n;
    fscanf(infile, "%lu", &n);
    size_t size = n * sizeof(element_t);
    element_t *initary = malloc(size);
    if (initary == NULL)
        perror("malloc"), exit(EXIT_FAILURE);
    for (size_t i = 0; i < n; i++)
        fscanf(infile, "%d", initary+i);
    if (fclose(infile) < 0)
        perror("fclose(in)"), exit(EXIT_FAILURE);
    element_t *workary = malloc(size);
    if (workary == NULL)
        perror("malloc"), exit(EXIT_FAILURE);

    for (size_t cnt = START; cnt <= END; cnt++) {
        double start = getms();

        memcpy(workary, initary, size);
        process(cnt, n, workary);

        char* outfilename = NULL;
        asprintf(&outfilename, outfileformat, cnt);
        if (outfilename == NULL)
            perror("asprintf"), exit(EXIT_FAILURE);
        FILE* outfile = fopen(outfilename, "w");
        if (outfile == NULL)
            perror("fopen(out)"), exit(EXIT_FAILURE);
        for (size_t i = 0; i < n; i++)
            fprintf(outfile, "%d%c", workary[i], " \n"[i+1==n]);
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
