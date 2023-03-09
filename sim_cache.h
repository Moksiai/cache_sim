
#ifndef _CACHE
#define _CACHE
#include <stdio.h>

#define ADDR_t long long unsigned
enum cache_policy {
  LRU,		/* replace least recently used block (perfect LRU) */
  Random,	/* replace a random block */
  FIFO		/* replace the oldest block in the set */
};

struct cache_t
{
  /* parameters */
    int ttsize;
    int nway;
    int ntagbit;
    int nindexbit;
    int noffsetbit;

    int nsets;			/* number of sets */
    int bsize;			/* block size in bytes */
    int indexmask;
    int (*policy)(struct set_t *sp);

    int access;
    int miss;
    int miss1;
    int miss2;
    struct set_t **set;
};

struct set_t
{
    int ndata;
    char **valid;
    int FIFOc;
    int nway;
    ADDR_t **tag;
    int accesstime;
    int **LRU;
};

enum cache_policy str2policy(char *str);

struct cache_t *
cache_create(int capacity, 
            int blk_size, 
            int nway, 
            int l_addr, 
            enum cache_policy r);

void access_cache(struct cache_t *cp, ADDR_t addr);
int read_set(struct set_t *sp, ADDR_t tag);
void replace_set(struct set_t *sp, ADDR_t tag, int waynum);

int num2bit(int a); /*1024 = 10*/

int FIFOchose(struct set_t *sp);
int randomchose(struct set_t *sp);
int LRUchose(struct set_t *sp);

void del_cache(struct cache_t *cp);
void print_cache(FILE *stream, struct cache_t *cp);

void print_all(struct cache_t *cp);

#endif