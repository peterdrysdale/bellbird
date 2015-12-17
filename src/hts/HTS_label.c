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
#include <ctype.h>              /* for isgraph() */

#include "cst_alloc.h"
#include "cst_error.h"
#include "cst_string.h"
/* hts_engine libraries */
#include "HTS_hidden.h"

/* HTS_Label_initialize: initialize label */
void HTS_Label_initialize(HTS_Label * label)
{
   label->head = NULL;
   label->size = 0;
}

/* HTS_Label_check_time: check label */
static void HTS_Label_check_time(HTS_Label * label)
{
   HTS_LabelString *lstring = label->head;
   HTS_LabelString *next = NULL;

   if (lstring)
      lstring->start = 0.0;
   while (lstring) {
      next = lstring->next;
      if (!next)
         break;
      if (lstring->end < 0.0 && next->start >= 0.0)
         lstring->end = next->start;
      else if (lstring->end >= 0.0 && next->start < 0.0)
         next->start = lstring->end;
      if (lstring->start < 0.0)
         lstring->start = -1.0;
      if (lstring->end < 0.0)
         lstring->end = -1.0;
      lstring = next;
   }
}

/* HTS_Label_load_from_strings: load label from strings */
void HTS_Label_load_from_strings(HTS_Label * label, size_t sampling_rate, size_t fperiod, char **lines, size_t num_lines)
{
   char buff[HTS_MAXBUFLEN];
   HTS_LabelString *lstring = NULL;
   size_t i;
   size_t data_index;
   double start, end;
   const double rate = (double) sampling_rate / ((double) fperiod * 1e+7);

   if (label->head || label->size != 0) {
      cst_errmsg("HTS_Label_load_from_strings: label list is not initialized.\n");
      cst_error();
      return;
   }
   /* copy label */
   for (i = 0; i < num_lines; i++) {
      if (!isgraph((int) lines[i][0]))
         break;
      label->size++;

      if (lstring) {
         lstring->next = cst_alloc(HTS_LabelString,1);
         lstring = lstring->next;
      } else {                  /* first time */
         lstring = cst_alloc(HTS_LabelString,1);
         label->head = lstring;
      }
      data_index = 0;
      if (bell_validate_atoi(lines[i], NULL)) {   /* has frame information */
         bell_get_token_from_string(lines[i], &data_index, buff, HTS_MAXBUFLEN);
         start = cst_atof(buff);
         bell_get_token_from_string(lines[i], &data_index, buff, HTS_MAXBUFLEN);
         end = cst_atof(buff);
         bell_get_token_from_string(lines[i], &data_index, buff, HTS_MAXBUFLEN);
         lstring->name = cst_strdup(buff);
         lstring->start = rate * start;
         lstring->end = rate * end;
      } else {
         lstring->start = -1.0;
         lstring->end = -1.0;
         lstring->name = cst_strdup(lines[i]);
      }
      lstring->next = NULL;
   }
   HTS_Label_check_time(label);
}

/* HTS_Label_get_size: get number of label string */
size_t HTS_Label_get_size(HTS_Label * label)
{
   return label->size;
}

/* HTS_Label_get_string: get label string */
const char *HTS_Label_get_string(HTS_Label * label, size_t index)
{
   size_t i;
   HTS_LabelString *lstring = label->head;

   for (i = 0; i < index && lstring; i++)
      lstring = lstring->next;
   if (!lstring)
      return NULL;
   return lstring->name;
}

/* HTS_Label_get_end_frame: get end frame */
double HTS_Label_get_end_frame(HTS_Label * label, size_t index)
{
   size_t i;
   HTS_LabelString *lstring = label->head;

   for (i = 0; i < index && lstring; i++)
      lstring = lstring->next;
   if (!lstring)
      return -1.0;
   return lstring->end;
}

/* HTS_Label_clear: free label */
void HTS_Label_clear(HTS_Label * label)
{
   HTS_LabelString *lstring, *next_lstring;

   for (lstring = label->head; lstring; lstring = next_lstring) {
      next_lstring = lstring->next;
      cst_free(lstring->name);
      cst_free(lstring);
   }
   HTS_Label_initialize(label);
}
