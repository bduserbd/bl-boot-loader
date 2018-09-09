#ifndef BL_STRING_H
#define BL_STRING_H

#include "bl-types.h"

/* Support only ASCII */
char *bl_strcpy(char *, const char *);
char *bl_strncpy(char *, const char *, bl_size_t);
int bl_strcmp(const char *, const char *);
int bl_wcscmp(const wchar_t *, const wchar_t *);
int bl_strncmp(const char *, const char *, bl_size_t);
int bl_wcsncmp(const wchar_t *, const wchar_t *, bl_size_t);
int bl_strcasecmp(const char *, const char *);
int bl_strncasecmp(const char *, const char *, bl_size_t);
bl_size_t bl_strlen(const char *);
bl_size_t bl_strnlen(const char *, bl_size_t);
char *bl_strdup(const char *);
char *bl_strndup(const char *, bl_size_t);
void *bl_memset(void *, int, bl_size_t);
void *bl_memcpy(void *, const void *, bl_size_t);
int  bl_memcmp(const u8 *, const u8 *, bl_size_t);
char *bl_strtok(char *, const char *);
char *bl_strchr(const char *, int);
int bl_toupper(int);
int bl_tolower(int);
char *bl_string_toupper(char *);
char *bl_string_tolower(char *);
int bl_isprint(int);
void bl_convert_utf16_to_utf8(char *, const u16 *, bl_size_t);

#endif

