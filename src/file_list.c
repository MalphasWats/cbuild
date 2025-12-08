#include "file_list.h"

file_list_t* file_list_new() {
    file_list_t* list = malloc(sizeof(file_list_t));
    list->files = malloc(sizeof(file_item_t) * FILE_LIST_INITIAL_ITEMS);
    list->num_of_files = 0;
    list->max_no_of_files = FILE_LIST_INITIAL_ITEMS;

    return list;
}

int32_t file_list_extract_extension(char* ext, const char* name) {
    uint32_t len = str_len(name);
    char* buff = malloc(len+1);

    uint32_t i = 0;
    while(i < len && name[(len-1) - i] != '.') {
        buff[i] = name[(len-1) - i];
        i += 1;
    }
    buff[i] = '\0';

    len = str_len(buff);
    i = 0;
    while(buff[i] != '\0') {
        ext[i] = buff[(len-1) - i];
        i++;
    }
    ext[i] = '\0';

    free(buff);

    return 1;
}

int32_t file_list_destroy(file_list_t* list) {
    uint32_t i = 0;
    while(i < list->num_of_files) {
        free(list->files[i].path);
        free(list->files[i].name);
        free(list->files[i].extension);
        i += 1;
    }
    free(list->files);
    free(list);
    return 1;
}

int32_t file_list_add_item(file_list_t* list, const char* path, const char* name, const char* extension, uint64_t last_modified) {
    //TODO: check list size, grow as needed
    list->files[list->num_of_files].path = str_new(str_len(path));//malloc(str_len(path)+1);
    str_cpy(path, list->files[list->num_of_files].path);
    list->files[list->num_of_files].name = str_new(str_len(name));
    str_cpy(name, list->files[list->num_of_files].name);
    list->files[list->num_of_files].extension = str_new(str_len(extension));//malloc(str_len(path)+1);
    str_cpy(extension, list->files[list->num_of_files].extension);
    
    list->files[list->num_of_files].last_modified = last_modified;

    list->num_of_files += 1;
    if (list->num_of_files >= FILE_LIST_INITIAL_ITEMS) { //TODO: grow
        printf("Error: Too many files\n");
        return 0;
    }
    return 1;
}

int32_t file_list_filter_by_extension(file_list_t* filtered, file_list_t* list, const char* extension) {
    for(uint32_t i=0 ; i<list->num_of_files ; i++) {
        if (str_cmp(list->files[i].extension, extension)) {
            file_list_add_item(filtered, list->files[i].path, list->files[i].name, list->files[i].extension, list->files[i].last_modified);
        }
    }
    return 1;
}

int32_t file_list_print(file_list_t* list) {
    uint32_t i = 0;
    while(i < list->num_of_files) {
        printf("%s\\%s [%s] (%llu)\n", list->files[i].path, list->files[i].name, list->files[i].extension, list->files[i].last_modified);
        i += 1;
    }
    return 1;
}