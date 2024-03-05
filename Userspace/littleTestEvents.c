#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

int main(int argc, char ** argv)
{
    printf("Ha!\n\n");
    getchar();
    int *temp = mmap(NULL, 8192, PROT_READ | PROT_WRITE  | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (temp==MAP_FAILED) {
        printf("Nope 1 bruh..\n");
    }
    /*int a = 0;
    while(a<8192) {
        *(temp+a)++;
        a++;
    }*/
    memset(temp,'A',8192);
    munmap(temp,8192);
    printf("Should've gotten some consumption followed by freeing.\n");
    getchar();
    temp = mmap(NULL, 4096, PROT_READ | PROT_WRITE  | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    memset(temp,'A',4096);
    printf("Should've gotten a fault/consumption\n");
    getchar();
    void * temp2=mremap(temp,4096,4096,MREMAP_FIXED | MREMAP_MAYMOVE /*MREMAP_DONTUNMAP makes tempd change from temp*/,temp+2*4096);
    if (temp2==MAP_FAILED) {
        printf("Nope 2 bruh..\n");
    }
    printf("Should've gotten (a weirdly blocking) mremap\n");
    void * tempd=mmap(temp, 4096, PROT_READ | PROT_WRITE  | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (tempd==MAP_FAILED) {
        printf("Nope 2 bruh..\n");
    }
    if(tempd!=temp) {
        printf("It changed..! Still trying to touch the old address though..\n");
        getchar();
    }
    memset(temp,'A',4096);
    printf("Hmm.. shouldn't've gotten more consumption\n");
    getchar();
    printf("All gud!?\n");
    getchar();
    if (madvise(temp2,4096,MADV_FREE)!=0)
        printf("Nope madvise\n");
    if (madvise(temp,4096,MADV_FREE)!=0)
        printf("Nope madvise\n");
    //munmap(temp2,8192);  // 4096 try
    //munmap(temp,8192);
    getchar();
    return 0;
}
