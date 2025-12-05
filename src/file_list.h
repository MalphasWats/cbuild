#ifndef FILELIST_H
#define FILELIST_H

#include <windows.h>
#include <stdint.h>
#include <stdio.h>

#include "string.h"

static const uint32_t FILE_LIST_INITIAL_ITEMS = 200;

typedef struct file_item_t {
    char* path;
    char* name;
    char* extension;
    uint64_t last_modified;
} file_item_t;

typedef struct file_list_t {
    uint32_t num_of_files;
    uint32_t max_no_of_files;
    file_item_t* files;
} file_list_t;

file_list_t* file_list_new();
int32_t file_list_extract_extension(char* ext, const char* name);
int32_t file_list_destroy(file_list_t* list);
int32_t file_list_add_item(file_list_t* list, const char* path, const char* name, const char* extension, uint64_t last_modified);
int32_t file_list_filter_by_extension(file_list_t* filtered, file_list_t* list, const char* extension);
int32_t file_list_find_by_filename(file_list_t* list, const char* filename);

int32_t file_list_print(file_list_t* list);

#endif