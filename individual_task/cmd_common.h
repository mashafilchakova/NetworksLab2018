#ifndef CMD_COMMON_H
#define CMD_COMMON_H

#include <stdint.h>

#define CMD_LEN (sizeof(cmd_t))

enum Command {
    CMD_UNKNOWN = 0,
    CMD_QUIT,
    CMD_LS,
    CMD_CD,
    CMD_GET,
    CMD_PUT,
    CMD_GET_BY_AUTHOR
};

typedef int32_t cmd_t;

#endif

