
/*
    not cmake

    > mkdir build
    > gcc -Wall -O2 -c src/string.c -o ./build/string.c.o
    > gcc -Wall -O2 -c src/file_list.c -o ./build/file_list.c.o
    > gcc -Wall -O2 -c src/config_parser.c -o ./build/config_parser.c.o
    > gcc -Wall -O2 -c src/cbuild.c -o ./build/cbuild.c.o
    > gcc -Wall -O2 ./build/cbuild.c.o ./build/string.c.o ./build/file_list.c.o ./build/config_parser.c.o -o ./build/cbuild.exe

*/
#include <windows.h>
#include <stdio.h>
#include <stdint.h>

#include "string.h"
#include "file_list.h"

#include "config_parser.h"

// https://stackoverflow.com/questions/2314542/listing-directory-contents-using-c-and-windows
int32_t load_directory(const char* directory_path, file_list_t* list) {
    WIN32_FIND_DATA f;
    HANDLE handle = NULL;

    size_t len = snprintf(NULL, 0, "%s\\*.*", directory_path) + 1;
    char* path = str_new(len); //malloc(len);
    snprintf(path, len, "%s\\*.*", directory_path);

    if((handle = FindFirstFile(path, &f)) == INVALID_HANDLE_VALUE)
    {
        printf("DEBUG: Path not found: [%s]\n", path);
        // https://learn.microsoft.com/en-us/windows/win32/debug/system-error-codes--0-499-
        int32_t err = GetLastError();
        printf("DEBUG: Error Code: %d\n", err);
        free(path);
        return 0;
    }

    do {
        if (!str_cmp(f.cFileName, ".") && !str_cmp(f.cFileName, "..")) {
       
            if (f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                len = snprintf(NULL, 0, "%s\\%s", directory_path, f.cFileName) + 1;
                char* new_path = str_new(len); //malloc(len);
                snprintf(new_path, len, "%s\\%s", directory_path, f.cFileName);
                load_directory(new_path, list);
                free(new_path);
            }
            else {
                uint64_t last_mod = (((uint64_t)f.ftLastWriteTime.dwHighDateTime) << 32) + f.ftLastWriteTime.dwLowDateTime;
                char* extension = str_new(str_len(f.cFileName));
                file_list_extract_extension(extension, f.cFileName);
                file_list_add_item(list, directory_path, f.cFileName, extension, last_mod);
                free(extension);
                //printf("debug: %s\\%s [%s] (%llu)\n", p, f.cFileName, extension, last_mod);
            }
        }

    }
    while (FindNextFile(handle, &f));

    free(path);
    FindClose(handle);
    return 1;
}

// int32_t build_file_list(file_list_t* list) {
//     return 1;
// }

int32_t make_directory_path(const char* path) {
    
    uint32_t path_len = str_len(path);
    char* buffer = str_new(path_len);

    uint32_t path_i = 0;

    //strip off any relative directory stuff
    //TODO: this breaks directory names starting with '.'
    while(path[path_i] == '.' || path[path_i] == '\\') {
        path_i += 1;
    }

    // first folder
    uint32_t buff_i = 0;
    while(1) {
        buffer[buff_i] = path[path_i];
        if (path[path_i] == '\\' || path[path_i] == '\0') {
            buffer[buff_i+1] = '\0';
            // make dir with buffer
            if (!CreateDirectory(buffer, NULL)) {
                int32_t err = GetLastError();
                if (err != 183) { // error 183 is file already exists, ignore that one.
                    // TODO: soon time to declare war on random printf errors. Need better error handling.
                    printf("Error: Unable to create build directory %s. Error Code: %d\n", buffer, err);
                    free(buffer);
                    return 0;
                }
            }
        }
        if (path[path_i] == '\0') break;
        path_i += 1;
        buff_i += 1;
    }


    free(buffer);
    return 1;
}

int32_t main(int32_t argc, char* argv[]) {

    file_list_t* source_files = file_list_new();
    if (!load_directory(".\\src", source_files)) {
        printf("No source directory found.\n");
        file_list_destroy(source_files);

        return 1;
    }

    config_t* current_config = load_config(); //DEFAULT_CONFIG;

    printf("Source directory Loaded [%d files]\n", source_files->num_of_files);
    file_list_print(source_files);

    file_list_t* build_files = file_list_new();
    file_list_filter_by_extension(build_files, source_files, "c");

    file_list_t* build_directory = file_list_new();
    file_list_t* object_files = file_list_new();
    if (!load_directory(".\\build", build_directory)) { // || object_files->num_of_files == 0) {
        printf("Creating .\\build directory.\n");
        if (!CreateDirectory(".\\build", NULL)) {
            int32_t err = GetLastError();
            printf("Error: Unable to create build directory. Error Code: %d", err);
            return 1;
        }
        // build all
    }
    else {
        // build changed
        file_list_filter_by_extension(object_files, build_directory, "o");

        file_list_t* files_to_build = file_list_new();

        uint32_t i = 0;
        uint64_t latest_build_date = 0;
        for(i=0 ; i<object_files->num_of_files ; i++) {
            if (object_files->files[i].last_modified > latest_build_date) latest_build_date = object_files->files[i].last_modified;
        }

        for(i=0 ; i<source_files->num_of_files ; i++) {
            if (source_files->files[i].last_modified > latest_build_date) {
                file_list_add_item(files_to_build, source_files->files[i].path, source_files->files[i].name, source_files->files[i].extension, source_files->files[i].last_modified);
            }
        }
        printf("Files Modified since last build [%d files]:\n", files_to_build->num_of_files);
        file_list_print(files_to_build);
    }
    file_list_destroy(object_files);
    file_list_destroy(build_directory);

    // testing build all
    printf("building with [%d files]\n", build_files->num_of_files);

    uint32_t errors = 0;
    char* obj_files = str_new(1);
    char* build_command;
    uint32_t build_command_len;

    for(uint32_t i=0 ; i<build_files->num_of_files ; i++) {
        printf(" building file: %s\\%s\n", build_files->files[i].path, build_files->files[i].name);
        char* build_path;
        int32_t new_len = str_substr(NULL, build_files->files[i].path, 6, str_len(build_files->files[i].path));
        if (new_len > 0) {
            char* buffer = str_new(new_len);
            str_substr(buffer, build_files->files[i].path, 6, str_len(build_files->files[i].path));
            uint32_t path_len = str_concat(NULL, ".\\build\\", buffer);
            build_path = str_new(path_len);
            str_concat(build_path, ".\\build\\", buffer);
            free(buffer);
            if (!make_directory_path(build_path)) {
                int32_t err = GetLastError();
                printf("Error: Unable to create directory %s. Error Code: %d", build_path, err);
                free(build_path);
                return 1;
            }
        }
        else {
            build_path = str_new(8);
            str_cpy(".\\build", build_path);
        }
        // do the compile.
        
        const char* obj_file_template = "%s\\%s.o ";
        uint32_t obj_files_buffer_len = 1 + snprintf(NULL, 0, obj_file_template, build_path, build_files->files[i].name);
        char* obj_files_buffer = malloc(obj_files_buffer_len);
        snprintf(obj_files_buffer, obj_files_buffer_len, obj_file_template, build_path, build_files->files[i].name);

        char* new_obj_files;
        uint32_t len1 = str_len(obj_files);
        new_obj_files = str_new(len1 + obj_files_buffer_len);
        str_concat(new_obj_files, obj_files, obj_files_buffer);
        free(obj_files);
        free(obj_files_buffer);
        obj_files = new_obj_files;

        const char* command_template_obj = "%s %s -c %s\\%s -o %s\\%s.o -MMD";
        build_command_len = 1 + snprintf(NULL, 0, command_template_obj, current_config->compiler, current_config->c_flags, build_files->files[i].path, build_files->files[i].name, build_path, build_files->files[i].name);
        build_command = str_new(build_command_len); //malloc(build_command_len);
        snprintf(build_command, build_command_len, command_template_obj, current_config->compiler, current_config->c_flags, build_files->files[i].path, build_files->files[i].name, build_path, build_files->files[i].name);
        printf(" %s\n", build_command);
        
        if (system(build_command)) {
            errors += 1;
        }

        free(build_command);
        free(build_path);
    }

    if (errors > 0) {
        printf("%d errors in build", errors);
    }
    else {
        // do the final build part.
        const char* command_template_exe = "%s %s %s -o .\\build\\%s %s";
        build_command_len = 1 + snprintf(NULL, 0, command_template_exe, current_config->compiler, current_config->c_flags, obj_files, current_config->output_file_name, current_config->l_flags);
        build_command = str_new(build_command_len); //malloc(build_command_len);
        snprintf(build_command, build_command_len, command_template_exe, current_config->compiler, current_config->c_flags, obj_files, current_config->output_file_name, current_config->l_flags);
        printf("\n %s\n", build_command);

        if (system(build_command)) {
            printf("Error building executable.");
        }

        free(build_command);
    }

    free(obj_files);
    config_destroy(current_config);

    file_list_destroy(source_files);
    file_list_destroy(build_files);
    return 0;
}