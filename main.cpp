#include <stdio.h>

#include "sim_cache.h"
#define NOISE

int main(int argc, char *argv[]){
    if(argc < 3){
        printf("%s [config] [address input]\n",argv[0]);
        return 0;
    }

    FILE *setfile,*opfile, *addrfile;
    if((setfile = fopen(argv[1],"r")) == NULL){
        printf("open %s error\n",argv[1]);
        return 1;
    }
    int capacity,blk_size,nway,l_addr;
    ADDR_t addr=0;

    cache_policy way_rep;
    char s[30],r[20];
    fscanf(setfile,"%s %d",s,&capacity);
    fscanf(setfile,"%s %d",s,&blk_size);
    fscanf(setfile,"%s %d",s,&nway);
    fscanf(setfile,"%s %d",s,&l_addr);
    fscanf(setfile,"%s %s",s,r);
    fclose(setfile);
    way_rep = str2policy(r);

    #ifdef NOISE
    printf("begin create cache\n");
    #endif 
    cache_t *cachea = cache_create(capacity, blk_size, nway, l_addr, way_rep);
    #ifdef NOISE
    printf("end create cache\n");
    #endif

    if((addrfile = fopen(argv[2],"r")) == NULL){
        printf("open %s error\n",argv[2]);
        return 1;
    }
    if((opfile = fopen("output","w")) == NULL){
        printf("open %s error\n","output");
        return 1;
    }

    while(fscanf(addrfile, "%llx", &addr) != EOF){
        access_cache(cachea, addr);
        addr = 0;
    }



    print_cache(opfile, cachea);
    del_cache(cachea);
    fclose(opfile);
}