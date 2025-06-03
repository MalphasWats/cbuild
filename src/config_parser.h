#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include <windows.h>
#include <stdio.h>
#include <stdint.h>

#include "string.h"

typedef struct config_t {
    char* compiler;
    char* output_file_name;
    char* c_flags;
    char* l_flags;

} config_t;

static const config_t DEFAULT_CONFIG = {
    "gcc",
    "out.exe",
    "-Wall -O2",
    ""
};

config_t* load_config();
config_t* load_config_file(const char* filename);

#endif