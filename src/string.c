#include "string.h"

char* str_new(int32_t length) {
    if (length < 1) return NULL;
    char* s = malloc(length+1);
    s[0] = '\0';
    s[length] = '\0';
    return s;
}

uint32_t str_len(const char* str) {
    uint32_t l = 0;
    while(str[l]) l++;

    return l;
}

int32_t str_cmp(const char* str1, const char* str2) {
    uint32_t i = 0;
    while(str1[i] == str2[i]){
        if (str1[i] == '\0') return 1;
        i++;
    }
    return 0;
}

void str_cpy(const char* src, char* dest) {
    uint32_t i = 0;
    while(src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

uint32_t str_index_of(const char* str, const char ch) {
    uint32_t len = str_len(str);
    uint32_t i=0;

    while(i<len && str[i] != ch) {
        i += 1;
    }

    return i;
}

uint32_t str_last_index_of(const char* str, const char ch) {
    uint32_t len = str_len(str);
    int32_t i=len;

    while(i>=0 && str[i] != ch) {
        i -= 1;
    }

    if (i < 0) return len;

    return i;
}

uint32_t str_substr(char* new_str, const char* str, uint32_t start, uint32_t stop) {
    if (start > stop) return 0;

    uint32_t len = str_len(str);
    int32_t new_len = stop - start;

    if (new_len < 1 || new_len > len) return 0;

    if (new_str == NULL) return new_len;

    for(uint32_t i=0 ; i<new_len ; i++) {
        new_str[i] = str[start+i];
    }
    new_str[new_len] = '\0';
    return new_len;
}

uint32_t str_concat(char* new_str, const char* str1, const char* str2) {

    uint32_t len1 = str_len(str1);
    uint32_t len2 = str_len(str2);
    uint32_t new_len = len1 + len2;

    if (new_len < 1) return 0;
    if (new_str == NULL) return new_len;

    uint32_t i = 0;
    for(i=0 ; i<len1 ; i++) {
        new_str[i] = str1[i];
    }
    for(i=0 ; i<len2 ; i++) {
        new_str[i+len1] = str2[i];
    }
    new_str[new_len] = '\0';
    return new_len;

}