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
/*  Copyright (c) 2001-2012  Nagoya Institute of Technology          */
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

#ifndef HTS_MISC_H
#define HTS_MISC_H

#include "cst_file.h"

typedef struct _HTS_File {
   unsigned char type;
   void *pointer;
} HTS_File;

/* HTS_fopen_from_cst_file: wrapper for opening from cst_file */
HTS_File *HTS_fopen_from_cst_file(cst_file cst_fp);

/* HTS_fopen: wrapper for fopen */
HTS_File *HTS_fopen_from_fn(const char *name, const char *opt);

/* HTS_fopen_from_fp: wrapper for fopen */
HTS_File *HTS_fopen_from_fp(HTS_File * fp, size_t size);

/* HTS_fopen_from_data: wrapper for fopen */
HTS_File *HTS_fopen_from_data(void *data, size_t size);

/* HTS_fclose: wrapper for fclose */
void HTS_fclose(HTS_File * fp);

/* HTS_feof: wrapper for feof */
int HTS_feof(HTS_File * fp);

/* HTS_fseek: wrapper for fseek */
int HTS_fseek(HTS_File * fp, long offset, int origin);

/* HTS_ftell: wrapper for ftell */
size_t HTS_ftell(HTS_File * fp);

/* HTS_fread_little_endiana: fread with byteswap */
size_t HTS_fread_little_endian(void *buf, size_t size, size_t n, HTS_File * fp);

/* bell_get_pattern_token: get pattern token (single/double quote can be used) */
bell_boolean bell_get_pattern_token(HTS_File * fp, char *buff, size_t bufflen);

/* bell_get_token_from_fp: get token from file pointer (separators are space,tab,line break) */
bell_boolean bell_get_token_from_fp(HTS_File * fp, char *buff, size_t bufflen);

/* bell_get_token_from_fp_with_separator: get token from file pointer with specified separator */
bell_boolean bell_get_token_from_fp_with_separator(HTS_File * fp, char *buff, size_t bufflen, char separator);

/* bell_get_token_from_string: get token from string (separator are space,tab,line break) */
bell_boolean bell_get_token_from_string(const char *string, size_t * index, char *buff, size_t bufflen);

/* bell_get_token_from_string_with_separator: get token from string with specified separator */
bell_boolean bell_get_token_from_string_with_separator(const char *str, size_t * index, char *buff, size_t bufflen, char separator);

/* HTS_finv: calculate 1.0/variance function */
double HTS_finv(const double x);

/*    Extra stuff we need */
#define HTS_MAXBUFLEN 1024
#endif 
