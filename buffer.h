#include "stdio.h"
#include "stddef.h"
#include "string.h"

#define BUFSIZE (2*1024*1024)
typedef struct {
    char writebuf[BUFSIZE];
    size_t bufidx;
} buffer_t;

#define BUFTHRESHOLD BUFSIZE*0.98

#define BUFFLUSH(buffer, file) do {if((buffer)->bufidx>=BUFTHRESHOLD) bufflush((buffer), (file));} while(0)

void bufinit(buffer_t * b);

void bufflush(buffer_t * b, FILE* fout);

void buffer_append_char(buffer_t * buffer, char c);

void buffer_append_string(buffer_t * buffer, char * str);