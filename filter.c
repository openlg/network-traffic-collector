//
// Created by Leon on 2024/5/25.
//

#include <string.h>
#include <printf.h>
#include "options.h"

extern options_t options;

int filter_by_addr(char *addr) {

    if (options.filter == NULL)
        return 0;

    char *ret = strstr(options.filter, addr);
    return ret == NULL ? 0 : 1;
}