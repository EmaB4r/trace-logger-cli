#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
//#include "tracerlib/mosaico-trace.h"
#include <string.h>
//#include <windows.h>
#include <stdint.h>
#include <stddef.h>
#include "cJSON/cJSON.h"
#include "arena.h"
#include "buffer.h"
#include <stdbool.h>
#include <sys/time.h>
#include <assert.h>
#define ANSI_ESCAPE "\x1b["
#define ANSI_RED ANSI_ESCAPE "31m"
#define ANSI_YELLOW ANSI_ESCAPE "33m"
#define ANSI_GREEN ANSI_ESCAPE "32m"
#define ANSI_BLUE ANSI_ESCAPE "34m"
#define ANSI_RESET ANSI_ESCAPE "0m"

#define MSG_SIZE 1024*4
#define todo(msg) assert(false && (msg))

typedef struct state_s{
    bool file_output;
    bool outfile_is_csv;
    char outfile_name[256];
}state_t;

struct log_event_s {
    enum log_level_e{dbg, war, err} log_level;
    char msg[MSG_SIZE];
};
struct overflow_event_s {
    char size[32];
};
struct cansniff_event_s {/*TODO*/};
struct channels_event_s {
    uint16_t ch_count;
    uint16_t ch_offset;
    char channels[10][32];
};
struct uint32_event_s {
    char val[32];
};
typedef struct {
    enum event_kind_e{log_e, overflow_e, channels_e, uint32_e, unknown_e} kind;
    char timestamp[32];
    union {
        struct log_event_s log_event;
        struct overflow_event_s overflow_event;
        struct cansniff_event_s cansniff_event;
        struct channels_event_s channels_event;
        struct uint32_event_s uint32_event;
    };
}trace_t;

//converte una trace in stringa e la scrive sulla stream
void save_trace(FILE* fout, const trace_t* const trace){
    if (trace->kind == unknown_e) return;
    fputs(trace->timestamp, fout);
    switch (trace->kind) {
        case log_e: {
            switch (trace->log_event.log_level) {
                case dbg: fputs(" dbg: ", fout);  break;
                case war: fputs(" war: ", fout);  break;
                case err: fputs(" err: ", fout);  break;
            }
            fputs(trace->log_event.msg, fout);
            fputc('\n', fout);
        }break;
        case overflow_e: {
            fputs(ANSI_RED " Overflow: " ANSI_RESET, fout);
            fputs(trace->overflow_event.size, fout);
            fputc('\n', fout);
        }break;
        case channels_e: {
            fputs(" Channels: [", fout);
            int cnt = trace->channels_event.ch_count-1;
            for (int i=0; i<cnt; i++) {
                fputs(trace->channels_event.channels[i], fout);
                fputc(',',fout);
            }
            fputs(trace->channels_event.channels[cnt],fout);
            fputs("]\n", fout);
        }break;
        case uint32_e: {
            fputs(ANSI_BLUE " uint32: " ANSI_RESET, fout);
            fputs(trace->uint32_event.val, fout);
            fputc('\n', fout);
        }break;
        default: {} break;
    }
}

buffer_t buf;
// converte una trace in stringa e la salva in un buffer
// quando quest'ultimo è quasi pieno lo scrive sulla stream
void save_trace_buff(FILE* fout, const trace_t* const trace){
    if (trace->kind == unknown_e) return;
    buffer_append_string(&buf, trace->timestamp);
    buffer_append_char(&buf, ' ');
    switch (trace->kind) {
        case log_e: {
            char * log_level;
            switch (trace->log_event.log_level) {
                case dbg: log_level = "dbg: "; break;
                case war: log_level = "war: "; break;
                case err: log_level = "err: "; break;
            }
            buffer_append_string(&buf, log_level);
            buffer_append_string(&buf, trace->log_event.msg);

        }break;
        case overflow_e: {
            buffer_append_string(&buf, "overflow: ");
            buffer_append_string(&buf, trace->overflow_event.size);
        }break;
        case channels_e: {
            buffer_append_string(&buf, "channels: [");
            int cnt = trace->channels_event.ch_count-1;
            for (int i=0; i<cnt; i++) {
                buffer_append_string(&buf, trace->channels_event.channels[i]);
                buffer_append_char(&buf, ',');
            }
            buffer_append_string(&buf, trace->channels_event.channels[cnt]);
            buffer_append_char(&buf, ']');
        }break;
        case uint32_e: {
            buffer_append_string(&buf, "uint32: ");
            buffer_append_string(&buf, trace->uint32_event.val);
        }break;
        default: {} break;
    }
    buffer_append_char(&buf, '\n');
    BUFFLUSH(&buf, fout);
}
void save_trace_buff_csv(FILE* fout, const trace_t* const trace){
    if (trace->kind == unknown_e) return;
    buffer_append_string(&buf, trace->timestamp);
    buffer_append_char(&buf, ',');
    switch (trace->kind) {
        case log_e: {
            char * log_level;
            switch (trace->log_event.log_level) {
                case dbg: log_level = "dbg,"; break;
                case war: log_level = "war,"; break;
                case err: log_level = "err,"; break;
            }
            buffer_append_string(&buf, log_level);
            buffer_append_string(&buf, trace->log_event.msg);
        }break;
        case overflow_e: {
            buffer_append_string(&buf, "overflow,");
            buffer_append_string(&buf, trace->overflow_event.size);
        }break;
        case channels_e: {
            buffer_append_string(&buf, "channels,");
            int cnt = trace->channels_event.ch_count-1;
            for (int i=0; i<cnt; i++) {
                buffer_append_string(&buf, trace->channels_event.channels[i]);
                buffer_append_char(&buf, ',');
            }
            buffer_append_string(&buf, trace->channels_event.channels[cnt]);
        }break;
        case uint32_e: {
            buffer_append_string(&buf, "uint32,");
            buffer_append_string(&buf, trace->uint32_event.val);
        }break;
        default: {} break;
    }
    buffer_append_char(&buf, '\n');
    BUFFLUSH(&buf, fout);
}

// riempie la tagged union con informazioni di tipo log
void trace_log(trace_t* trace, cJSON* json){
    json=json->next;
    trace->kind = log_e;
    strcpy(trace->timestamp,json->valuestring);
    json=json->next->child;
    if(memcmp("dbg", json->valuestring, 3)      == 0) trace->log_event.log_level = dbg;
    else if(memcmp("war", json->valuestring, 3) == 0) trace->log_event.log_level = war;
    else if(memcmp("err", json->valuestring, 3) == 0) trace->log_event.log_level = err;
    json = json->next;
    strcpy(trace->log_event.msg, json->valuestring);
}
void trace_overflow(trace_t* trace, cJSON* json) {
    json=json->next;
    trace->kind = overflow_e;
    strcpy(trace->timestamp,json->valuestring);
    json=json->next->child;
    strcpy(trace->overflow_event.size,json->valuestring);
}

// riempie la tagged union con informazioni di tipo channel
void trace_channels(trace_t* trace, cJSON* json) {
    json=json->next;
    trace->kind = channels_e;
    strcpy(trace->timestamp,json->valuestring);
    json=json->next->child;
    trace->channels_event.ch_count = json->valueint;
    json=json->next;
    trace->channels_event.ch_offset = json->valueint;
    json=json->next->child;
    for (int i = 0; i < trace->channels_event.ch_count; i++) {
        strcpy(trace->channels_event.channels[i],json->valuestring);
        json=json->next;
    }
}
void trace_uint32(trace_t* trace, cJSON* json) {
    json=json->next;
    trace->kind = uint32_e;
    strcpy(trace->timestamp,json->valuestring);
    json=json->next->child;
    strcpy(trace->uint32_event.val,json->valuestring);
}

//crea una tagged union seguendo la stringa json
void parse_trace(trace_t* trace, cJSON* json) {
    json=json->child;
    if (memcmp(json->valuestring, "channels", 8)==0) {
        trace_channels(trace, json);
    }
    else if (memcmp(json->valuestring, "log", 3)==0) {
        trace_log(trace, json);
    }
    else if (memcmp(json->valuestring, "overflow", 8)==0) {
        trace->kind=overflow_e;
        trace_overflow(trace, json);
    }
    else if (memcmp(json->valuestring, "uint32", 6)==0) {
        trace->kind=uint32_e;
        trace_uint32(trace, json);
    }
    else {
        trace->kind=unknown_e;
        //printf("Unknown trace type: %s\n", json->valuestring);
    }
}

int readline(FILE* fin, char*buf){
    char c;
    uint16_t i=0;
    while((c=fgetc(fin))!='\n'){
        if(c == EOF) return 0;
        buf[i++]=c;
    }
    buf[i]=0;
    return 1;
}

state_t program_state = {0};

void set_program_state(int argc, char* argv[]){
    //TODO: INCOMPLETA
    int i = 1;
    while(i < argc){
        if(!strcmp("-h", argv[i])){
            fputs(
                "Usage: data_logger.exe [options]\n"
                "Options:\n"
                "   -h                                              Display this information.\n"
                "   -d <file.csv | file.*>                          Dumps all logs into as csv .csv file or normally with any other extension\n"
                "   TODO: -w <log, can, channel, overflow, uint32>  Whitelists all these events\n"
                "   TODO: -b <log, can, channel, overflow, uint32>  Blacklists all these events\n"
                "   TODO: -i <10.0.X.X>                             sets IP\n",
            stdout);
            exit(0);
        };
        if(!strcmp("-d", argv[i])){
            if(i<argc) i++;
            char extension[5]={0};
            int len = strlen(argv[i]);
            if(len>4){
                extension[0] = argv[i][len-4];
                extension[1] = argv[i][len-3];
                extension[2] = argv[i][len-2];
                extension[3] = argv[i][len-1];
            }
            if(!strcmp(".csv", extension)) program_state.outfile_is_csv = 1;
            program_state.file_output = 1;
            strcpy(program_state.outfile_name, argv[i]);
        };
        if(!strcmp("-w", argv[i])){};
        if(!strcmp("-b", argv[i])){};
        i++;
    }
}

int main(int argc, char* argv[]) {
    char readlinebuf[1024*2];
    size_t lostPkts = 0;
    char ip[16] = {0};
    trace_t trace;
    cJSON *json;
    bufinit(&buf);
    set_program_state(argc, argv);
    printf("ip: ");
    scanf("%s", ip);
    struct mosaicotraceapi *h = mosaicotrace_create();
    struct mt_conn_api *conn = mosaicotrace_connect(h, ip, "metadata", 30);
    cJSON_Hooks hooks = {.malloc_fn = arena_malloc, .free_fn=arena_free};
    cJSON_InitHooks(&hooks);
    switch (program_state.file_output) {
        case 1:
            FILE* fout = fopen(program_state.outfile_name, "w");
            switch (program_state.outfile_is_csv) {
                case 0:while(1) {
                    mosaicotrace_next(conn, readlinebuf, sizeof(readlinebuf), &lostPkts);
                    fprintf(fout , "%s\n", readlinebuf);
                    //json = cJSON_Parse(readlinebuf);
                    //parse_trace(&trace, json);
                    //save_trace_buff(fout, &trace);
                    //arena_reset();
                }break;
                case 1:while(1) {
                    mosaicotrace_next(conn, readlinebuf, sizeof(readlinebuf), &lostPkts);
                    //fprintf(fout , "%s\n", readlinebuf);
                    json = cJSON_Parse(readlinebuf);
                    parse_trace(&trace, json);
                    save_trace_buff_csv(fout, &trace);
                    arena_reset();
                }break;
            }
            fclose(fout);
        break;
        case 0:while(1) {
            mosaicotrace_next(conn, readlinebuf, sizeof(readlinebuf), &lostPkts);
            //fprintf(fout , "%s\n", readlinebuf);
            json = cJSON_Parse(readlinebuf);
            parse_trace(&trace, json);
            save_trace(stdout, &trace);
            arena_reset();
        }
            break;
    }
    return 0;
}