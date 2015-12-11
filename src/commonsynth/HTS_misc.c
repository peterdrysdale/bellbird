/*************************************************************************/
/*                This code has been modified for Bellbird.              */
/*                See COPYING for more copyright details.                */
/*                The unmodified source code copyright notice            */
/*                is included below.                                     */
/*************************************************************************/
/* ----------------------------------------------------------------- */
/*           The HMM-Based Speech Synthesis Engine "hts_engine API"  */
/*           developed by HTS Working Group                          */
/*           http://hts-engine.sourceforge.net/                      */
/* ----------------------------------------------------------------- */
/*                                                                   */
/*  Copyright (c) 2001-2013  Nagoya Institute of Technology          */
/*                           Department of Computer Science          */
/*                                                                   */
/*                2001-2008  Tokyo Institute of Technology           */
/*                           Interdisciplinary Graduate School of    */
/*                           Science and Engineering                 */
/*                                                                   */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/* - Redistributions of source code must retain the above copyright  */
/*   notice, this list of conditions and the following disclaimer.   */
/* - Redistributions in binary form must reproduce the above         */
/*   copyright notice, this list of conditions and the following     */
/*   disclaimer in the documentation and/or other materials provided */
/*   with the distribution.                                          */
/* - Neither the name of the HTS working group nor the names of its  */
/*   contributors may be used to endorse or promote products derived */
/*   from this software without specific prior written permission.   */
/*                                                                   */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND            */
/* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,       */
/* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF          */
/* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE          */
/* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS */
/* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,          */
/* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED   */
/* TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,     */
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON */
/* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,   */
/* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY    */
/* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE           */
/* POSSIBILITY OF SUCH DAMAGE.                                       */
/* ----------------------------------------------------------------- */

#ifndef HTS_MISC_C
#define HTS_MISC_C

#ifdef __cplusplus
#define HTS_MISC_C_START extern "C" {
#define HTS_MISC_C_END   }
#else
#define HTS_MISC_C_START
#define HTS_MISC_C_END
#endif                          /* __CPLUSPLUS */

HTS_MISC_C_START;

#include <stdarg.h>             /* for va_list */
#include <string.h>             /* for memcpy */

#include "cst_alloc.h"
#include "cst_error.h"
#include "cst_file.h"
#include "HTS_misc.h"

/* check variance in finv() */
#define INFTY   ((double) 1.0e+38)
#define INFTY2  ((double) 1.0e+19)
#define INVINF  ((double) 1.0e-38)
#define INVINF2 ((double) 1.0e-19)

#define HTS_FILE  0
#define HTS_DATA  1

typedef struct _HTS_Data {
   unsigned char *data;
   size_t size;
   size_t index;
} HTS_Data;

/* HTS_fgetc: wrapper for fgetc */
static int HTS_fgetc(HTS_File * fp)
{
   if (fp == NULL) {
      return EOF;
   } else if (fp->type == HTS_FILE) {
      return fgetc((FILE *) fp->pointer);
   } else if (fp->type == HTS_DATA) {
      HTS_Data *d = (HTS_Data *) fp->pointer;
      if (d->size <= d->index)
         return EOF;
      return (int) d->data[d->index++];
   }
   cst_errmsg("HTS_fgetc: Unknown file type.\n");
   return EOF;
}

/* HTS_fopen_from_cst_file: wrapper for opening from cst_file */
HTS_File *HTS_fopen_from_cst_file(cst_file cst_fp)
{
   HTS_File *fp = cst_alloc(HTS_File,1);
   fp->type = HTS_FILE;
   fp->pointer = (void *) cst_fp;

   return fp;
}

/* HTS_fopen_from_fn: wrapper for fopen */
HTS_File *HTS_fopen_from_fn(const char *name, const char *opt)
{
   HTS_File *fp = cst_alloc(HTS_File,1);

   fp->type = HTS_FILE;
   fp->pointer = (void *) fopen(name, opt);

   if (fp->pointer == NULL) {
      cst_errmsg("HTS_fopen: Cannot open %s.\n", name);
      cst_free(fp);
      return NULL;
   }

   return fp;
}

/* HTS_fopen_from_fp: wrapper for fopen */
HTS_File *HTS_fopen_from_fp(HTS_File * fp, size_t size)
{
   if (fp == NULL || size == 0)
      return NULL;
   else if (fp->type == HTS_FILE) {
      HTS_Data *d;
      HTS_File *f;
      d = cst_alloc(HTS_Data,1);
      d->data = cst_alloc(unsigned char,size);
      d->size = size;
      d->index = 0;
      if (fread(d->data, sizeof(unsigned char), size, (FILE *) fp->pointer) != size) {
         cst_free(d->data);
         cst_free(d);
         return NULL;
      }
      f = cst_alloc(HTS_File,1);
      f->type = HTS_DATA;
      f->pointer = (void *) d;
      return f;
   } else if (fp->type == HTS_DATA) {
      HTS_File *f;
      HTS_Data *tmp1, *tmp2;
      tmp1 = (HTS_Data *) fp->pointer;
      if (tmp1->index + size > tmp1->size)
         return NULL;
      tmp2 = cst_alloc(HTS_Data,1);
      tmp2->data = cst_alloc(unsigned char,size);
      tmp2->size = size;
      tmp2->index = 0;
      memcpy(tmp2->data, &tmp1->data[tmp1->index], size);
      tmp1->index += size;
      f = cst_alloc(HTS_File,1);
      f->type = HTS_DATA;
      f->pointer = (void *) tmp2;
      return f;
   }

   cst_errmsg("HTS_fopen_from_fp: Unknown file type.\n");
   return NULL;
}

/* HTS_fopen_from_data: wrapper for fopen */
HTS_File *HTS_fopen_from_data(void *data, size_t size)
{
   HTS_Data *d;
   HTS_File *f;

   if (data == NULL || size == 0)
      return NULL;

   d = cst_alloc(HTS_Data,1);
   d->data = cst_alloc(unsigned char,size);
   d->size = size;
   d->index = 0;

   memcpy(d->data, data, size);

   f = cst_alloc(HTS_File,1);
   f->type = HTS_DATA;
   f->pointer = (void *) d;

   return f;
}

/* HTS_fclose: wrapper for fclose */
void HTS_fclose(HTS_File * fp)
{
   if (fp == NULL) {
      return;
   } else if (fp->type == HTS_FILE) {
      if (fp->pointer != NULL)
         fclose((FILE *) fp->pointer);
      cst_free(fp);
      return;
   } else if (fp->type == HTS_DATA) {
      if (fp->pointer != NULL) {
         HTS_Data *d = (HTS_Data *) fp->pointer;
         if (d->data != NULL)
            cst_free(d->data);
         cst_free(d);
      }
      cst_free(fp);
      return;
   }
   cst_errmsg("HTS_fclose: Unknown file type.\n");
}

/* HTS_feof: wrapper for feof */
int HTS_feof(HTS_File * fp)
{
   if (fp == NULL) {
      return 1;
   } else if (fp->type == HTS_FILE) {
      return feof((FILE *) fp->pointer);
   } else if (fp->type == HTS_DATA) {
      HTS_Data *d = (HTS_Data *) fp->pointer;
      return d->size <= d->index ? 1 : 0;
   }
   cst_errmsg("HTS_feof: Unknown file type.\n");
   return 1;
}

/* HTS_fseek: wrapper for fseek */
int HTS_fseek(HTS_File * fp, long offset, int origin)
{
   if (fp == NULL) {
      return 1;
   } else if (fp->type == HTS_FILE) {
      return fseek((FILE *) fp->pointer, offset, origin);
   } else if (fp->type == HTS_DATA) {
      HTS_Data *d = (HTS_Data *) fp->pointer;
      if (origin == SEEK_SET) {
         d->index = (size_t) offset;
      } else if (origin == SEEK_CUR) {
         d->index += offset;
      } else if (origin == SEEK_END) {
         d->index = d->size + offset;
      } else {
         return 1;
      }
      return 0;
   }
   cst_errmsg("HTS_fseek: Unknown file type.\n");
   return 1;
}

/* HTS_ftell: wrapper for ftell */
size_t HTS_ftell(HTS_File * fp)
{
   if (fp == NULL) {
      return 0;
   } else if (fp->type == HTS_FILE) {
      fpos_t pos;
      fgetpos((FILE *) fp->pointer, &pos);
#if defined(_WIN32) || defined(__CYGWIN__) || defined(__APPLE__) || defined(__ANDROID__)
      return (size_t) pos;
#else
      return (size_t) pos.__pos;
#endif                          /* _WIN32 || __CYGWIN__ || __APPLE__ || __ANDROID__ */
   }
   cst_errmsg("HTS_ftell: Unknown file type.\n");
   return 0;
}

/* HTS_fread: wrapper for fread */
static size_t HTS_fread(void *buf, size_t size, size_t n, HTS_File * fp)
{
   if (fp == NULL || size == 0 || n == 0) {
      return 0;
   } else if (fp->type == HTS_FILE) {
      return fread(buf, size, n, (FILE *) fp->pointer);
   } else if (fp->type == HTS_DATA) {
      HTS_Data *d = (HTS_Data *) fp->pointer;
      size_t i, length = size * n;
      unsigned char *c = (unsigned char *) buf;
      for (i = 0; i < length; i++) {
         if (d->index < d->size)
            c[i] = d->data[d->index++];
         else
            break;
      }
      if (i == 0)
         return 0;
      else
         return i / size;
   }
   cst_errmsg("HTS_fread: Unknown file type.\n");
   return 0;
}

#ifdef WORDS_BIGENDIAN
/* HTS_byte_swap: byte swap */
static void HTS_byte_swap(void *p, size_t size, size_t block)
{
   char *q, tmp;
   size_t i, j;

   q = (char *) p;

   for (i = 0; i < block; i++) {
      for (j = 0; j < (size / 2); j++) {
         tmp = *(q + j);
         *(q + j) = *(q + (size - 1 - j));
         *(q + (size - 1 - j)) = tmp;
      }
      q += size;
   }
}
#endif                          /* WORDS_BIGENDIAN */

/* HTS_fread_little_endian: fread with byteswap */
size_t HTS_fread_little_endian(void *buf, size_t size, size_t n, HTS_File * fp)
{
   size_t block = HTS_fread(buf, size, n, fp);

#ifdef WORDS_BIGENDIAN
   HTS_byte_swap(buf, size, block);
#endif                          /* WORDS_BIGENDIAN */

   return block;
}

/* bell_get_pattern_token: get pattern token (single/double quote can be used) */
bell_boolean bell_get_pattern_token(HTS_File * fp, char *buff, size_t bufflen)
{
   int cint;
   size_t i;
   bell_boolean squote = FALSE, dquote = FALSE;

   if (fp == NULL || HTS_feof(fp))
      return FALSE;
   cint = HTS_fgetc(fp);

   if (cint == EOF)
      return FALSE;

   while (((char)cint) == ' ' || ((char)cint) == '\n') {
      cint = HTS_fgetc(fp);
      if (cint == EOF)
          return FALSE;
   }

   if (((char)cint) == '\'') {             /* single quote case */
      cint = HTS_fgetc(fp);
      if (cint == EOF)
          return FALSE;
      squote = TRUE;
   }

   if (((char)cint) == '\"') {             /*double quote case */
      cint = HTS_fgetc(fp);
      if (cint == EOF)
          return FALSE;
      dquote = TRUE;
   }

   if (((char)cint) == ',' && bufflen > 1) {    /*special character ',' */
      buff[0] = ',';
      buff[1] = '\0';
      return TRUE;
   }

   i = 0;
   while (i < bufflen) {
      buff[i++] = (char)cint;
      cint = HTS_fgetc(fp);
      if ((squote || dquote) && cint == EOF)
         return FALSE;
      if (squote && ((char)cint) == '\'')
         break;
      if (dquote && ((char)cint) == '\"')
         break;
      if (!squote && !dquote) {
         if (((char)cint) == ' ')
            break;
         if (((char)cint) == '\n')
            break;
         if (HTS_feof(fp))
            break;
      }
   }
   if (i == bufflen)
   {
      cst_errmsg("bell_get_pattern_token: Overflow of buffer probably due to malformed voice file\n");
      return FALSE;
   }

   buff[i] = '\0';
   return TRUE;
}

/* bell_get_token_from_fp: get token from file pointer (separators are space, tab, and line break) */
bell_boolean bell_get_token_from_fp(HTS_File * fp, char *buff, size_t bufflen)
{
   int cint;
   size_t i;

   if (fp == NULL || HTS_feof(fp))
      return FALSE;
   cint = HTS_fgetc(fp);

   if (cint == EOF)
      return FALSE;

   while (((char)cint) == ' ' || ((char)cint) == '\n' || ((char)cint) == '\t') {
      cint = HTS_fgetc(fp);
      if (cint == EOF)
          return FALSE;
   }

   for (i = 0; ((char)cint) != ' ' && ((char)cint) != '\n' && ((char)cint) != '\t' && (i < bufflen);) {
      buff[i++] = ((char)cint);
      cint = HTS_fgetc(fp);
      if (cint == EOF)
         break;
   }
   if (i == bufflen)
   {
      cst_errmsg("bell_get_token_from_fp: Overflow of buffer probably due to malformed voice file\n");
      return FALSE;
   }

   buff[i] = '\0';
   return TRUE;
}

/* bell_get_token_from_fp_with_separator: get token from file pointer with specified separator */
bell_boolean bell_get_token_from_fp_with_separator(HTS_File * fp, char *buff, size_t bufflen, char separator)
{
   int cint;
   size_t i;

   if (fp == NULL || HTS_feof(fp))
      return FALSE;
   cint = HTS_fgetc(fp);

   if (cint == EOF)
      return FALSE;

   while (((char)cint) == separator) {
      cint = HTS_fgetc(fp);
      if (cint == EOF)
          return FALSE;
   }

   for (i = 0; ((char)cint) != separator && (i < bufflen);) {
      buff[i++] = ((char)cint);
      cint = HTS_fgetc(fp);
      if (cint == EOF)
         break;
   }
   if (i == bufflen)
   {
      cst_errmsg("bell_get_token_from_fp_with_separator: Overflow of buffer probably due to malformed voice file\n");
      return FALSE;
   }

   buff[i] = '\0';
   return TRUE;
}

/* bell_get_token_from_string: get token from string (separators are space, tab, and line break) */
bell_boolean bell_get_token_from_string(const char *string, size_t * index, char *buff, size_t bufflen)
{
   char c;
   size_t i;

   c = string[(*index)];
   if (c == '\0')
      return FALSE;
   c = string[(*index)++];
   if (c == '\0')
      return FALSE;
   while (c == ' ' || c == '\n' || c == '\t') {
      if (c == '\0')
         return FALSE;
      c = string[(*index)++];
   }
   for (i = 0; c != ' ' && c != '\n' && c != '\t' && c != '\0' && (i < bufflen); i++) {
      buff[i] = c;
      c = string[(*index)++];
   }
   if (i == bufflen)
   {
      cst_errmsg("bell_get_token_from_string: Overflow of buffer probably due to malformed labels\n");
      cst_error();
   }

   buff[i] = '\0';
   return TRUE;
}

/* bell_get_token_from_string_with_separator: get token from string with specified separator */
bell_boolean bell_get_token_from_string_with_separator(const char *str, size_t * index, char *buff, size_t bufflen, char separator)
{
   char c;
   size_t i = 0;

   if (str == NULL)
      return FALSE;

   c = str[(*index)];
   if (c == '\0')
      return FALSE;
   while (c == separator) {
      if (c == '\0')
         return FALSE;
      (*index)++;
      c = str[(*index)];
   }
   while (c != separator && c != '\0' && (i < bufflen)) {
      buff[i++] = c;
      (*index)++;
      c = str[(*index)];
   }
   if (i == bufflen)
   {
      cst_errmsg("bell_get_token_from_string: Overflow of buffer probably due to malformed voice file\n");
      cst_error();
   }
   if (c != '\0')
      (*index)++;

   buff[i] = '\0';

   if (i > 0)
      return TRUE;
   else
      return FALSE;
}

/* HTS_finv: calculate 1.0/variance function */
double HTS_finv(const double x)
{
   if (x >= INFTY2)
      return 0.0;
   if (x <= -INFTY2)
      return 0.0;
   if (x <= INVINF2 && x >= 0)
      return INFTY;
   if (x >= -INVINF2 && x < 0)
      return -INFTY;

   return (1.0 / x);
}

HTS_MISC_C_END;

#endif                          /* !HTS_MISC_C */
