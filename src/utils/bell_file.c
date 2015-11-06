/*************************************************************************/
/*                This code has been created for Bellbird.               */
/*                See COPYING for more copyright details.                */
/*************************************************************************/
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "cst_alloc.h"
#include "cst_file.h"
#include "cst_string.h"

cst_file bell_fopen(const char * filename, const char * mode)
/* Bare wrapper for standard fopen */
{
    return fopen(filename,mode);
}

int bell_fgetc(cst_file fh)
/* Bare wrapper for standard fgetc */
{
    return fgetc(fh);
}

size_t bell_fwrite(const void *buf, size_t size, size_t count, cst_file fh)
/* Bare wrapper for standard fwrite */
{
    return fwrite(buf, size, count, fh);
}

size_t bell_fread(void *buf, size_t size, size_t count, cst_file fh)
/* Bare wrapper for standard fread */
{
    return fread(buf, size, count, fh);
}

off_t bell_ftello(cst_file fh)
/* Bare wrapper for ftello to allow large file support tell functions */
{
    return ftello(fh);
}

int bell_fseeko(cst_file fh, off_t offset, int whence)
/* Bare wrapper for fseeko to allow large file support seek functions */
{
    return fseeko(fh,offset,whence);
}

int bell_fclose(cst_file fh)
/* Bare wrapper for standard fclose */
{
    return fclose(fh);
}

int bell_fprintf(cst_file fh, const char *fmt, ...)
/* Bare wrapper for fprintf based on recommendations of C FAQ Question 15.5 */
{
    int retval;
    va_list parg;

    va_start(parg, fmt);
    retval = vfprintf(fh, fmt, parg);
    va_end(parg);

    return retval;
}

int bell_snprintf(char *buf, size_t n, const char *fmt, ...)
/* Bare wrapper for snprintf based on recommendations of C FAQ Question 15.5 */
/* Note we rely on standards compliant snprintf. Anyone using non-compliant */
/* '-1' returning function on overflow will have to provide their own here. */
{
    int retval;
    va_list parg;

    va_start(parg, fmt);
    retval = vsnprintf(buf, n, fmt, parg);
    va_end(parg);

    return retval;
}