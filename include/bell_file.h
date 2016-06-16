#ifndef __BELL_FILE_H__
#define __BELL_FILE_H__

#include <stdio.h>

FILE *bell_fopen(const char * filename, const char * mode);
int bell_fgetc(FILE *fh);
size_t bell_fwrite(const void *buf, size_t size, size_t count, FILE *fh);
size_t bell_fread(void *buf, size_t size, size_t count, FILE *fh);
off_t bell_ftello(FILE *fh);
int bell_fseeko(FILE *fh, off_t offset, int whence);
int bell_fclose(FILE *fh);
int bell_fprintf(FILE *fh, const char *fmt, ...);
int bell_snprintf(char *buf, size_t n, const char *fmt, ...);

#endif
