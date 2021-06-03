#ifndef STRING_H
#define STRING_H

int memcmp(const void *buf1, const void *buf2,size_t n);
void *memcpy(void *buf1, const void *buf2, size_t n);
void *memset(void *buf, int ch, size_t n);
size_t strlen(const char *s);
char* strcpy(char *dest, const char *src);
char* strrchr(const char *, int);
char *strcat(char *s1, const char *s2);
int stricmp(const char *str1, const char *str2);

#endif
