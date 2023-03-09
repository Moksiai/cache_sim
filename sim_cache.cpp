#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "sim_cache.h"
#define NOISE

#define Getindex(cp, addr) ((addr>>((cp)->noffsetbit))&((cp)->indexmask))
#define Gettag(cp, addr) (addr>>((cp)->noffsetbit + (cp)->nindexbit))
#define PNT(pt,i,type) ((type *)(((char *)pt)+i*(sizeof(type)))) // int **s [i], => PNT(s,i,int)

enum cache_policy str2policy(char *str){
    if(str[0] == 'F' || str[0] == 'f') return FIFO;
    if(str[0] == 'L' || str[0] == 'l') return LRU;
    if(str[0] == 'R' || str[0] == 'r') return Random;
    printf("cannot read policy\n");
    exit(0);
}

cache_t *cache_create(int capacity, int blk_size, int nway, int l_addr, enum cache_policy r){
    cache_t *cp = (cache_t *)calloc(1,sizeof(cache_t));
    if (!cp) {
        printf("out of virtual memory");
        exit(0);
    }

    int nblk = capacity/blk_size;
    cp->nway = nway;
    cp->nsets = nblk / nway;
    cp->noffsetbit = num2bit(blk_size);
    cp->nindexbit = num2bit(cp->nsets);
    cp->ntagbit = l_addr - cp->noffsetbit - cp->nindexbit;
    cp->bsize = blk_size;

    cp->indexmask = (1<<(cp->nindexbit)) - 1;
    cp->ttsize = (nblk)*((cp->ntagbit) + 1 + blk_size * 8 );
    cp->set = (struct set_t **)calloc(cp->nsets, sizeof(set_t));
    
    switch(r){
    case LRU:
        cp->policy = LRUchose;
        break;
    case FIFO:
        cp->policy = FIFOchose;
        break;
    default:
        srand(time(NULL));
        cp->policy = randomchose;
    }

    for(int i=0; i < (cp->nsets); i++){
        struct set_t *sp = PNT(cp->set, i, struct set_t);
        sp->nway = nway;
        sp->valid = (char **)calloc(nway,sizeof(char));
        sp->tag = (ADDR_t **)calloc(nway,sizeof(ADDR_t));
        sp->LRU = (int **)calloc(nway, sizeof(int));
    }

    return cp;
}

void access_cache(struct cache_t *cp, ADDR_t addr){
    int index = Getindex(cp,addr);
    int tag = Gettag(cp,addr);
    cp->access++;
#ifdef NOISE
printf("index=%d\ttag=%x\t",index,tag);
#endif
    struct set_t *sp = PNT(cp->set, index, set_t);
    if (read_set(sp, tag) == 0){
        cp->miss++;
        if(sp->ndata == sp->nway) cp->miss2++;
        else cp->miss1++;
        int u = cp->policy(sp);
        replace_set(sp, tag, u);
        #ifdef NOISE
        printf("miss");
        #endif
        #ifdef NOISE2
        print_all(cp);
        #endif
    }
        #ifdef NOISE
        printf("\n");
        #endif
}


int read_set(struct set_t *sp, ADDR_t tag){
    for(int i=0; i < (sp->nway); i++){
        #ifdef NOISE
        printf("%d\t", *(PNT(sp->valid, i, char)));
        #endif
        if(*(PNT(sp->valid, i, char)))
            if(*(PNT(sp->tag, i, ADDR_t)) == tag){
                *PNT(sp->LRU, i, int) = ++(sp->accesstime);
                return 1;
            }
    }
    return 0;
}

void replace_set(struct set_t *sp, ADDR_t tag, int waynum){
    if(*(PNT(sp->valid, waynum, char)) == 0){
        *(PNT(sp->valid, waynum, char)) = 1;
        sp->ndata++;
    }
    *PNT(sp->tag, waynum, ADDR_t) = tag;
    *PNT(sp->LRU, waynum, int) = ++(sp->accesstime);
}


int FIFOchose(struct set_t *sp){
    if(sp->ndata < sp->nway) return sp->ndata++;
    int temp = sp->FIFOc;
    sp->FIFOc = (sp->FIFOc+1)%(sp->nway);
    return temp;
}

int randomchose(struct set_t *sp){
    if(sp->ndata < sp->nway) return sp->ndata++;
    else return rand()%(sp->nway);
}

int LRUchose(struct set_t *sp){
    if(sp->ndata < sp->nway) return sp->ndata++;
    else{
        int min = INT_MAX;
        int tar = 0;
        for(int i=0; i<(sp->nway); i++){
            if(*PNT(sp->LRU, i, int) < min){
                min = *PNT(sp->LRU, i, int);
                tar = i;
            }
        }
        return tar;
    }
}

int num2bit(int a){
    int t = 0;
    while((1<<t) < a){
        t++;
    }
    return t;
}

void del_cache(struct cache_t *cp){
    for(int i=0; i < (cp->nsets); i++){
        free(PNT(cp->set, i, set_t)->valid);
        free(PNT(cp->set, i, set_t)->tag);
        free(PNT(cp->set, i, set_t)->LRU);
    }
    free(cp->set);
    free(cp);
}

void print_cache(FILE *stream, struct cache_t *cp){
    /*sample output
- num_set: 256
- num_block_offset_bit: 10
- num_index_bit: 8
- num_tag_bit: 46
- num_total_access: 99
- num_hit: 77
- num_miss: 22
- hit_rate: 77.78%
- miss_rate: 22.220000000000002%
- compulsory_miss:22
- conflict_miss:0
*/
    fprintf(stream, "- num_set: %d\n"               ,cp->nsets);
    fprintf(stream, "- num_block_offset_bit: %d\n"  ,cp->noffsetbit);
    fprintf(stream, "- num_index_bit: %d\n"         ,(cp->nindexbit));
    fprintf(stream, "- num_tag_bit: %d\n"           ,(cp->ntagbit));
    fprintf(stream, "- num_total_access: %d\n"      ,(cp->access));
    fprintf(stream, "- num_hit: %d\n"               ,(cp->access)-(cp->miss));
    fprintf(stream, "- hit_rate: %f\n"              ,((float)((cp->access)-(cp->miss)))/(cp->access));
    fprintf(stream, "- miss_rate : %f\n"            ,((float)(cp->miss))/(cp->access));
    fprintf(stream, "- compulsory_miss: %d\n"       ,(cp->miss1));
    fprintf(stream, "- conflict_miss: %d\n"         ,(cp->miss2));

}

void print_all(struct cache_t *cp){
    printf("---------------------------------------------------------\n");
    for(int i =0; i<cp->nsets; i++){
        struct set_t *sp = PNT(cp->set, i, set_t);
        for(int j=0; j<sp->nway; j++){
            if(*(PNT(sp->valid, j, char)))
                printf("\t\tindex : %d\t%d\ttag : %X\n",i,j,*(PNT(sp->tag,j,ADDR_t)));
        }
    }
    printf("---------------------------------------------------------\n");
}