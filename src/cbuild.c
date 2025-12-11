
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

#include "util/string.h"
#include "file_list.h"

#include "config_parser.h"

static const uint32_t BUILD_MODIFIED = 0;
static const uint32_t BUILD_ALL = 1;

// https://stackoverflow.com/questions/2314542/listing-directory-contents-using-c-and-windows
int32_t load_directory(const char* directory_path, file_list_t* list) {
    WIN32_FIND_DATA f;
    HANDLE handle = NULL;

    size_t len = snprintf(NULL, 0, "%s/*.*", directory_path) + 1;
    char* path = str_new(len);
    snprintf(path, len, "%s/*.*", directory_path);

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
                len = snprintf(NULL, 0, "%s/%s", directory_path, f.cFileName) + 1;
                char* new_path = str_new(len);
                snprintf(new_path, len, "%s/%s", directory_path, f.cFileName);
                load_directory(new_path, list);
                free(new_path);
            }
            else {
                uint64_t last_mod = (((uint64_t)f.ftLastWriteTime.dwHighDateTime) << 32) + f.ftLastWriteTime.dwLowDateTime;
                char* extension = str_new(str_len(f.cFileName));
                file_list_extract_extension(extension, f.cFileName);
                file_list_add_item(list, directory_path, f.cFileName, extension, last_mod);
                free(extension);
            }
        }

    }
    while (FindNextFile(handle, &f));

    free(path);
    FindClose(handle);
    return 1;
}

int32_t make_directory_path(const char* path) {
    
    uint32_t path_len = str_len(path);
    char* buffer = str_new(path_len);

    uint32_t path_i = 0;

    //strip off any relative directory stuff
    //TODO: this breaks directory names starting with '.'
    //while(path[path_i] == '.' || path[path_i] == '/') {
    //    path_i += 1;
    //}

    // first folder
    uint32_t buff_i = 0;
    while(1) {
        buffer[buff_i] = path[path_i];
        if (path[path_i] == '/' || path[path_i] == '\0') {
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

int32_t make_build_path_from_source_path(char* build_path, const char* source_path) {
    int32_t source_path_len = str_len(source_path);
    int32_t new_len = str_substr(NULL, source_path, 6, source_path_len);
    uint32_t path_len = 0;
    if (new_len > 0) {
        char* buffer = str_new(new_len);
        str_substr(buffer, source_path, 6, source_path_len);
        path_len = str_concat(NULL, "./build/", buffer);
        if (build_path != NULL) str_concat(build_path, "./build/", buffer);
        free(buffer);
        return path_len;
    }
    else {
        path_len = 7;
        if (build_path != NULL) str_cpy("./build", build_path);    
    }
    return path_len;
}

int32_t main(int32_t argc, char* argv[]) {

    uint32_t build_mode = BUILD_MODIFIED;

    if (argc > 1) {
        if ( str_cmp_ignore_case(argv[1], "ALL") ) build_mode = BUILD_ALL;
    }

    file_list_t* source_files = file_list_new();
    if (!load_directory("./src", source_files)) {
        printf("No source directory found.\n");
        file_list_destroy(source_files);

        return 1;
    }

    config_t* current_config = load_config(); //DEFAULT_CONFIG;

    printf("Source directory Loaded [%d files]\n", source_files->num_of_files);

    file_list_t* build_files = file_list_new();

    file_list_t* build_directory = file_list_new();
    file_list_t* object_files = file_list_new();

    file_list_filter_by_extension(build_files, source_files, "c");

    if (!load_directory("./build", build_directory)) {
        printf("Creating ./build directory.\n");
        if (!CreateDirectory("./build", NULL)) {
            int32_t err = GetLastError();
            printf("Error: Unable to create build directory. Error Code: %d", err);
            return 1;
        }
        // build all
        build_mode = BUILD_ALL;
    }

    uint32_t modified_files = 0;
    if (build_mode == BUILD_ALL) {
        for (uint32_t i = 0 ; i < build_files->num_of_files ; i++) build_files->files[i].flags = 1;
        modified_files = build_files->num_of_files;
    }
    else {
        file_list_filter_by_extension(object_files, build_directory, "o");
        // build changed
        uint64_t latest_build_date = 0;

        for(uint32_t i=0 ; i<object_files->num_of_files ; i++) {
            if (object_files->files[i].last_modified > latest_build_date) latest_build_date = object_files->files[i].last_modified;
        }

        //TODO: check for header files too!!!
        for(uint32_t i=0 ; i<build_files->num_of_files ; i++) {
            if (build_files->files[i].last_modified > latest_build_date) {
                build_files->files[i].flags = 1;
                modified_files++;
            }
        }

        for(uint32_t i=0 ; i<source_files->num_of_files ; i++) {
            if ( str_cmp_ignore_case(source_files->files[i].extension, "h") && source_files->files[i].last_modified > latest_build_date) {

                //TODO: cute hack to normalise path names. .d files can have mixed / \ path symbols
                uint32_t source_filename_len = snprintf(NULL, 0, "%s/%s", source_files->files[i].path+2, source_files->files[i].name) + 1;
                char* source_filename_path = str_new(source_filename_len);
                snprintf(source_filename_path, source_filename_len, "%s/%s", source_files->files[i].path+2, source_files->files[i].name);

                for(uint32_t b=0 ; b<build_files->num_of_files ; b++) {

                    uint32_t build_path_len = make_build_path_from_source_path(NULL, build_files->files[b].path) + 1;
                    char* build_path = str_new(build_path_len);
                    build_path_len = make_build_path_from_source_path(build_path, build_files->files[b].path);

                    build_path[build_path_len++] = '/';
                    build_path[build_path_len] = '\0';

                    uint32_t dep_filename_len = str_concat(NULL, build_path, build_files->files[b].name) + 2;
                    char* dep_filename = str_new(dep_filename_len);
                    dep_filename_len = str_concat(dep_filename, build_path, build_files->files[b].name);
                    dep_filename[dep_filename_len++] = '.';
                    dep_filename[dep_filename_len++] = 'd';
                    dep_filename[dep_filename_len] = '\0';

                    FILE *f;
                    f = fopen(dep_filename, "r");
                    if (!f) {
                        printf("Dependency file not found, rebuilding module [%s].\n", dep_filename);
                        if (build_files->files[b].flags != 1) {
                            build_files->files[b].flags = 1;
                            modified_files++;
                        }
                    }
                    else {
                        char c;
                        while( (c = fgetc(f)) != EOF && c != ':'); //Skip to list of files.
                        fgetc(f); // leading space.
                        char* buffer = str_new(255); //TODO: allow for longer file paths!
                        uint32_t ci = 0;
                        while( (c = fgetc(f)) != EOF ) {
                            if (ci > 254) { //TODO: allow for longer file paths!
                                printf("error processing dependency file: %s\n path too long.\n", dep_filename);
                                return 1;
                            }
                            if (c == ' ' || c == '\r' || c == '\n') {
                                buffer[ci] = '\0';

                                if (str_cmp_ignore_case(source_filename_path, buffer)) {
                                    if (build_files->files[b].flags != 1) {
                                        build_files->files[b].flags = 1;
                                        modified_files++;
                                    }
                                }
                                ci = 0;
                                buffer[ci] = '\0';
                            }
                            else {
                                if (c == '\\') c = '/'; //TODO: another cute hack to normalise any backslashes.
                                                        //      GCC can still produce some weird paths though.
                                buffer[ci++] = c;
                            }
                        }

                        fclose(f);
                        free(buffer);
                    }
                    free(build_path);
                    free(dep_filename);
                }

                free(source_filename_path);
            }
        }
    }

    printf("Building [%d files]:\n", modified_files);

    file_list_destroy(build_directory);

    if ( build_files->num_of_files > 0 ) {

        uint32_t errors = 0;
        char* obj_files = str_new(1);
        char* build_command;
        uint32_t build_command_len;

        for(uint32_t i=0 ; i<build_files->num_of_files ; i++) {
            uint32_t build_path_len = make_build_path_from_source_path(NULL, build_files->files[i].path) + 1;
            char* build_path = str_new(build_path_len);
            build_path_len = make_build_path_from_source_path(build_path, build_files->files[i].path);

            if (build_path_len > 7) { // build path is ./build so don't need to make the directory.
                if (!make_directory_path(build_path)) {
                    int32_t err = GetLastError();
                    printf("Error: Unable to create directory %s. Error Code: %d", build_path, err);
                    free(build_path);
                    return 1;
                }
            }

            
            // do the compile.
            
            const char* obj_file_template = "%s/%s.o ";
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

            if (build_files->files[i].flags == 1) {

                const char* command_template_obj = "%s %s -c %s/%s -o %s/%s.o -MMD";
                build_command_len = 1 + snprintf(NULL, 0, command_template_obj, current_config->compiler, current_config->c_flags, build_files->files[i].path, build_files->files[i].name, build_path, build_files->files[i].name);
                build_command = str_new(build_command_len);
                snprintf(build_command, build_command_len, command_template_obj, current_config->compiler, current_config->c_flags, build_files->files[i].path, build_files->files[i].name, build_path, build_files->files[i].name);
                printf(" %s\n", build_command);
                
                if (system(build_command)) {
                    errors += 1;
                }

                free(build_command);
            }
            free(build_path);
        }

        if (errors > 0) {
            printf("%d errors in build", errors);
        }
        else {
            // do the final build part.
            printf("\nLinking executable [%s]:\n", current_config->output_file_name);
            const char* command_template_exe = "%s %s %s -o ./build/%s %s";
            build_command_len = 1 + snprintf(NULL, 0, command_template_exe, current_config->compiler, current_config->c_flags, obj_files, current_config->output_file_name, current_config->l_flags);
            build_command = str_new(build_command_len);
            snprintf(build_command, build_command_len, command_template_exe, current_config->compiler, current_config->c_flags, obj_files, current_config->output_file_name, current_config->l_flags);
            printf(" %s\n", build_command);

            if (system(build_command)) {
                printf("Error building executable.\n");
            }

            free(build_command);
        }
        free(obj_files);
    }
    else {
        printf("Nothing to build.\n");
    }
    
    config_destroy(current_config);

    file_list_destroy(source_files);
    file_list_destroy(object_files);
    file_list_destroy(build_files);
    return 0;
}