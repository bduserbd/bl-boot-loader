#include "include/string.h"
#include "core/include/memory/heap.h"

char *bl_strcpy(char *dest, const char *src)
{
	char *res;

	if (!dest || !src)
		return NULL;

	res = dest;
	while ((*dest++ = *src++)) ;

	return res;
}
BL_EXPORT_FUNC(bl_strcpy);

char *bl_strncpy(char *dest, const char *src, bl_size_t n)
{
	char *res;

	if (!dest || !src)
		return NULL;

	res = dest;
	while ((*dest++ = *src++) && --n) ;

	return res;
}
BL_EXPORT_FUNC(bl_strncpy);

int bl_strcmp(const char *s1, const char *s2)
{
	if (!s1 && !s2)
		return 0;
	else if (!s1 && s2)
		return -1;
	else if (s1 && !s2)
		return 1;

	while (*s1 == *s2 && *s1 && *s2) { s1++; s2++; } 

	return *s1 - *s2;
}
BL_EXPORT_FUNC(bl_strcmp);

int bl_wcscmp(const wchar_t *s1, const wchar_t *s2)
{
	if (!s1 && !s2)
		return 0;
	else if (!s1 && s2)
		return -1;
	else if (s1 && !s2)
		return 1;

	while (*s1 == *s2 && *s1 && *s2) { s1++; s2++; } 

	return *s1 - *s2;
}
BL_EXPORT_FUNC(bl_wcscmp);

int bl_strncmp(const char *s1, const char *s2, bl_size_t n)
{
	if ((!s1 && !s2) || n == 0)
		return 0;
	else if (!s1 && s2)
		return -1;
	else if (s1 && !s2)
		return 1;

	while (*s1 == *s2 && *s1 && *s2 && --n) { s1++; s2++; } 

	return *s1 - *s2;
}
BL_EXPORT_FUNC(bl_strncmp);

int bl_wcsncmp(const wchar_t *s1, const wchar_t *s2, bl_size_t n)
{
	if ((!s1 && !s2) || n == 0)
		return 0;
	else if (!s1 && s2)
		return -1;
	else if (s1 && !s2)
		return 1;

	while (*s1 == *s2 && *s1 && *s2 && --n) { s1++; s2++; } 

	return *s1 - *s2;
}
BL_EXPORT_FUNC(bl_wcsncmp);

int bl_strcasecmp(const char *s1, const char *s2)
{
	if (!s1 && !s2)
		return 0;
	else if (!s1 && s2)
		return -1;
	else if (s1 && !s2)
		return 1;

	while (bl_tolower(*s1) == bl_tolower(*s2) && *s1 && *s2) { s1++; s2++; } 

	return *s1 - *s2;
}
BL_EXPORT_FUNC(bl_strcasecmp);

int bl_strncasecmp(const char *s1, const char *s2, bl_size_t n)
{
	if ((!s1 && !s2) || n == 0)
		return 0;
	else if (!s1 && s2)
		return -1;
	else if (s1 && !s2)
		return 1;

	while (bl_tolower(*s1) == bl_tolower(*s2) && *s1 && *s2 && --n) { s1++; s2++; } 

	return *s1 - *s2;
}
BL_EXPORT_FUNC(bl_strncasecmp);

void *bl_memset(void *s, int c, bl_size_t n)
{
	char *d;

	if (!s)
		return NULL;

	d = (char *)s;

	while (--n)
		*d++ = c;		

	return s;
}
BL_EXPORT_FUNC(bl_memset);

void *bl_memcpy(void *dest, const void *src, bl_size_t n)
{
	char *d;
	const char *s;

	if (!dest || !src)
		return NULL;

	d = (char *)dest;
	s = (const char *)src;

	if (s > d)
		while (n--)
			*d++ = *s++;
	else {
		d += n;
		s += n;

		while (n--)
			*--d = *--s;
	}

	return dest;
}
BL_EXPORT_FUNC(bl_memcpy);

int bl_memcmp(const u8 *m1, const u8 *m2, bl_size_t n)
{
	bl_size_t i;

	for (i = 0; i < n; i++, m1++, m2++)
		if (*m1 < *m2)
			return -1;
		else if (*m1 > *m2)
			return 1;

	return 0;
}
BL_EXPORT_FUNC(bl_memcmp);

bl_size_t bl_strlen(const char *s)
{
	bl_size_t len;

	if (!s)
		return 0;

	for (len = 0; s[len]; len++) ;

	return len;
}
BL_EXPORT_FUNC(bl_strlen);

bl_size_t bl_strnlen(const char *s, bl_size_t maxlen)
{
	bl_size_t len;

	if (!s)
		return 0;

	for (len = 0; s[len]; len++)
		if (len + 1 == maxlen)
			return maxlen;

	return len;
}
BL_EXPORT_FUNC(bl_strnlen);

char *bl_strdup(const char *s)
{
	char *res;
	bl_size_t len;

	if (!s)
		return NULL;

	len = bl_strlen(s) + 1;
	res = bl_heap_alloc(len);
	if (!res)
		return NULL;

	res[len] = 0;
	return bl_memcpy(res, s, len);
}
BL_EXPORT_FUNC(bl_strdup);

char *bl_strndup(const char *s, bl_size_t n)
{
	char *res;
	bl_size_t len;

	if (!s)
		return NULL;

	len = bl_strlen(s) + 1;
	if (len > n)
		len = n + 1;

	res = bl_heap_alloc(len);
	if (!res)
		return NULL;

	res[len] = 0;
	return bl_memcpy(res, s, len);
}
BL_EXPORT_FUNC(bl_strndup);

/*
 * Simple implementation of `strtok'. From man page:
 * "The end of each token is found by scanning forward until either the next
 *  delimiter byte is found or until the terminating null bytes ('\0') is encountered.
 *  If a delimiter byte is found, it is overwritten with a null byte to terminate
 *  the current token, and strtok() saves a pointer to the next token."
 *
 * NOTE: Only one string instance should be used !
 */
static char *bl_strtok_str = NULL;

static int bl_isdelim(char c, const char *delim)
{
	int i;

	for (i = 0; delim[i]; i++)
		if (c == delim[i])
			return 1;

	return 0;
}

char *bl_strtok(char *s, const char *delim)
{
	char *end, *res;

	if (s)
		bl_strtok_str = s;

	if (!delim)
		return bl_strtok_str;

	while (bl_isdelim(*bl_strtok_str, delim))
		bl_strtok_str++;

	if (*bl_strtok_str == '\0')
		return NULL;

	end = bl_strtok_str;
	while (*end != '\0' && !bl_isdelim(*end, delim))
		end++;

	*end = '\0';

	res = bl_strtok_str;
	bl_strtok_str = end + 1;

	return res;
}
BL_EXPORT_FUNC(bl_strtok);

char *bl_strchr(const char *s, int c)
{
	int i;

	if (!s)
		return NULL;

	if (c == '\0')
		return (char *)s + bl_strlen(s);

	for (i = 0; s[i]; i++)
		if (s[i] == c)
			return (char *)s + i;

	return NULL;
}
BL_EXPORT_FUNC(bl_strchr);

int bl_toupper(int c)
{
	return c >= 'a' && c <= 'z' ? c - 0x20 : c;
}
BL_EXPORT_FUNC(bl_toupper);

int bl_tolower(int c)
{
	return c >= 'A' && c <= 'Z' ? c + 0x20 : c;
}
BL_EXPORT_FUNC(bl_tolower);

char *bl_string_toupper(char *str)
{
	char *res;

	for (res = str; *str; str++)
		*str = bl_toupper(*str);

	return res;
}
BL_EXPORT_FUNC(bl_string_toupper);

char *bl_string_tolower(char *str)
{
	char *res;

	for (res = str; *str; str++)
		*str = bl_tolower(*str);

	return res;
}
BL_EXPORT_FUNC(bl_string_tolower);

int bl_isprint(int c)
{
	return c >= ' ' && '~' >= c;
}
BL_EXPORT_FUNC(bl_isprint);

void bl_convert_utf16_to_utf8(char *dest, const u16 *src, bl_size_t n)
{
	bl_size_t i;

	for (i = 0; i < n; i++)
		if (src[i] <= 0x7f)
			*dest++ = (char)src[i];
}
BL_EXPORT_FUNC(bl_convert_utf16_to_utf8);

