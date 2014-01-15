#ifndef __BELL_FILE_H__
#define __BELL_FILE_H__

char * bell_build_filename(const char * fn_voice, const char * end_segment);

cst_file bell_fopen(const char * filename, const char * mode);
int bell_fgetc(cst_file fh);
long bell_fwrite(const void *buf, long size, long count, cst_file fh);
long bell_fread(void *buf, long size, long count, cst_file fh);
off_t bell_ftello(cst_file fh);
int bell_fseeko(cst_file fh, off_t offset, int whence);
int bell_fclose(cst_file fh);
int bell_fprintf(cst_file fh, const char *fmt, ...);
int bell_sprintf(char *buf, const char *fmt, ...);

#endif
