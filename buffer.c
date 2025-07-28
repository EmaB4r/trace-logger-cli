#include "buffer.h"
#include <string.h>

void bufinit(buffer_t * b){
    b->bufidx = 0;
}

void bufflush(buffer_t * b, FILE* fout){
    if(!b->bufidx) return;
    fwrite(b->writebuf, 1, b->bufidx, fout);
    b->bufidx=0;
}

void buffer_append_char(buffer_t * buffer, char c){
    buffer->writebuf[buffer->bufidx++]=c;
}

void buffer_append_string(buffer_t * buffer, char * str){
    size_t len = strlen(str);
    memcpy(&(buffer->writebuf[buffer->bufidx]), str, len);
    buffer->bufidx+=len;
}