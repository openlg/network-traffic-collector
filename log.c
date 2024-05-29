//
// Created by Leon on 2024/5/26.
//

#include <printf.h>
#include <stdio.h>
#include <time.h>
#include "log.h"

void log_(char level,const char *format, va_list args) {
    char datetime[80];
    time_t now = time(NULL);
    strftime(datetime, sizeof(datetime), "%Y-%m-%d %H:%M:%S", localtime(&now));
    printf("[%s] [%c] ", datetime, level);
    vprintf(format, args);
    printf("\n");
}
void log_debug(const char *msg, ...){
    va_list args;
    va_start(args, msg);
    log_('D', msg, args);
    va_end(args);
}
void log_info(const char* msg, ...) {
    va_list args;
    va_start(args, msg);
    log_('I', msg, args);
    va_end(args);
}
void log_warn(const char *msg, ...){
    va_list args;
    va_start(args, msg);
    log_('W', msg, args);
    va_end(args);
}
void log_error(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    log_('E', msg, args);
    va_end(args);
}