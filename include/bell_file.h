#ifndef __BELL_FILE_H__
#define __BELL_FILE_H__

cst_file bell_fopen(const char * filename, const char * mode);
int bell_fgetc(cst_file fh);
size_t bell_fwrite(const void *buf, size_t size, size_t count, cst_file fh);
size_t bell_fread(void *buf, size_t size, size_t count, cst_file fh);
off_t bell_ftello(cst_file fh);
int bell_fseeko(cst_file fh, off_t offset, int whence);
int bell_fclose(cst_file fh);
int bell_fprintf(cst_file fh, const char *fmt, ...);
int bell_snprintf(char *buf, size_t n, const char *fmt, ...);

#endif
