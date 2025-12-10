#ifndef STRING_H
#define STRING_H

#include <windows.h>
#include <stdint.h>

char* str_new(int32_t length);

uint32_t str_len(const char* str);
int32_t str_cmp(const char* str1, const char* str2);
int32_t str_cmp_ignore_case(const char* str1, const char* str2);
void str_cpy(const char* src, char* dest);

uint32_t str_index_of(const char* str, const char ch);
uint32_t str_last_index_of(const char* str, const char ch);

uint32_t str_substr(char* new_str, const char* str, uint32_t start, uint32_t stop);
uint32_t str_concat(char* new_str, const char* str1, const char* str2);

#endif