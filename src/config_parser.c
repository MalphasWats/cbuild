#include "config_parser.h"

static const char* DEFAULT_CONFIG_FILENAME = "build.cfg";

config_t* load_config() {
    return load_config_file(DEFAULT_CONFIG_FILENAME);
}

void config_destroy(config_t* config) {
    free(config->compiler);
    free(config->output_file_name);
    free(config->c_flags);
    free(config->l_flags);

    free(config);
}

config_t* load_config_file(const char* filename) {
    config_t* config = malloc(sizeof(config_t));

    config->compiler = str_new(str_len(DEFAULT_CONFIG.compiler));
    str_cpy(DEFAULT_CONFIG.compiler, config->compiler);
    config->output_file_name = str_new(str_len(DEFAULT_CONFIG.output_file_name));
    str_cpy(DEFAULT_CONFIG.output_file_name, config->output_file_name);
    config->c_flags = str_new(str_len(DEFAULT_CONFIG.c_flags));
    str_cpy(DEFAULT_CONFIG.c_flags, config->c_flags);
    config->l_flags = str_new(str_len(DEFAULT_CONFIG.l_flags));
    str_cpy(DEFAULT_CONFIG.l_flags, config->l_flags);

    FILE *f;
    f = fopen(filename, "r");
    if (!f) {
        printf("Error: Unable to open config file %s. Using default config\n", filename);
        return config;
    }

    char c;
    char* key = str_new(17); // Current longest key: output_file_name
    char* value = str_new(255); //TODO: flags could probably easily exceed this.
    uint32_t ki = 0;
    uint32_t vi = 0;
    while( (c = fgetc(f)) != EOF) {
        if (ki > 16) {
            printf("Error: configuration key too large.\n");
            while( (c = fgetc(f)) != '\n' && c != EOF);
        }

        if (c == '\n') {
            ki = 0;
            key[ki] = '\0';
            vi = 0;
            value[vi] = '\0';
        }
        else if (c == ' ') {} // skip
        else if (c == '=') {
            //grab the value
            while( (c = fgetc(f)) != '\n' && c != EOF) {
                if (!(vi==0 && c == ' ')) {
                    value[vi] = c;
                    value[++vi] = '\0';
                }
                if (vi > 254){
                    //TODO allow longer values
                    printf("Error: configuration value too long for %s\n", key);
                    while( (c = fgetc(f)) != '\n' && c != EOF);
                    break;
                }
            }

            if (str_cmp(key, "compiler")) {
                free(config->compiler);
                config->compiler = str_new(str_len(value));
                str_cpy(value, config->compiler);
            }
            else if (str_cmp(key, "output_file_name")) {
                free(config->output_file_name);
                config->output_file_name = str_new(str_len(value));
                str_cpy(value, config->output_file_name);
            }
            else if (str_cmp(key, "c_flags")) {
                free(config->c_flags);
                config->c_flags = str_new(str_len(value));
                str_cpy(value, config->c_flags);
            }
            else if (str_cmp(key, "l_flags")) {
                free(config->l_flags);
                config->l_flags = str_new(str_len(value));
                str_cpy(value, config->l_flags);
            }

            ki = 0;
            key[ki] = '\0';
            vi = 0;
            value[vi] = '\0';
        }
        else {
            // Looking for a key
            // TODO - force c to lowercase
            //c = str_char_to_lower(c);
            key[ki] = c;
            key[++ki] = '\0';
        }
    }

    free(key);
    free(value);
    
    fclose(f);

    return config;
}


