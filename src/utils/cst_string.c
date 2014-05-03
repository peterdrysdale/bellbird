/*************************************************************************/
/*                This code has been modified for Bellbird.              */
/*                See COPYING for more copyright details.                */
/*                The unmodified source code copyright notice            */
/*                is included below.                                     */
/*************************************************************************/
/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                        Copyright (c) 1999                             */
/*                        All Rights Reserved.                           */
/*                                                                       */
/*  Permission is hereby granted, free of charge, to use and distribute  */
/*  this software and its documentation without restriction, including   */
/*  without limitation the rights to use, copy, modify, merge, publish,  */
/*  distribute, sublicense, and/or sell copies of this work, and to      */
/*  permit persons to whom this work is furnished to do so, subject to   */
/*  the following conditions:                                            */
/*   1. The code must retain the above copyright notice, this list of    */
/*      conditions and the following disclaimer.                         */
/*   2. Any modifications must be clearly marked as such.                */
/*   3. Original authors' names are not deleted.                         */
/*   4. The authors' names are not used to endorse or promote products   */
/*      derived from this software without specific prior written        */
/*      permission.                                                      */
/*                                                                       */
/*  CARNEGIE MELLON UNIVERSITY AND THE CONTRIBUTORS TO THIS WORK         */
/*  DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING      */
/*  ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT   */
/*  SHALL CARNEGIE MELLON UNIVERSITY NOR THE CONTRIBUTORS BE LIABLE      */
/*  FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES    */
/*  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN   */
/*  AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,          */
/*  ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF       */
/*  THIS SOFTWARE.                                                       */
/*                                                                       */
/*************************************************************************/
/*             Author:  Alan W Black (awb@cs.cmu.edu)                    */
/*               Date:  December 1999                                    */
/*************************************************************************/
/*                                                                       */
/*    String manipulation functions                                      */
/*                                                                       */
/*************************************************************************/
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>
#include "cst_alloc.h"
#include "cst_string.h"

double cst_atof(const char *str)
{
// Bare atof where no validation is performed. See bell_validate_atof for atof with checks
    return atof(str);
}

cst_string *cst_strdup(const cst_string *str)
{
    cst_string *nstr = NULL;

    if (str)
    {
	nstr = cst_alloc(cst_string,cst_strlen((const char *)str)+1);
	memmove(nstr,str,cst_strlen((const char *)str)+1);
    }
    return nstr;
}

char *cst_substr(const char *str,int start, int length)
{
    char *nstr = NULL;

    if (str)
    {
	nstr = cst_alloc(char,length+1);
	strncpy(nstr,str+start,length);
	nstr[length] = '\0';
    }
    return nstr;
}

cst_string *cst_downcase(const cst_string *str)
{
    cst_string *dc;
    int i;

    dc = cst_strdup(str);
    for (i=0; str[i] != '\0'; i++)
    {
	if (isupper((int)str[i]))
	    dc[i] = tolower((int)str[i]);
    }
    return dc;
}

cst_string *cst_upcase(const cst_string *str)
{
    cst_string *uc;
    int i;

    uc = cst_strdup(str);
    for (i=0; str[i] != '\0'; i++)
    {
	if (islower((int)str[i]))
	    uc[i] = toupper((int)str[i]);
    }
    return uc;
}

int cst_member_string(const char *str, const char * const *slist)
{
    const char * const *p;

    for (p = slist; *p; ++p)
	if (cst_streq(*p, str))
	    break;

    return *p != NULL;
}

int bell_validate_atof(const char * str, float * floatout)
{
// A safer atof checking for some types of errors
    char *end;

    errno = 0;
    *floatout=strtof(str, &end);

    if (end==str || ERANGE==errno)
    {
        return FALSE;
    }

    return TRUE;
}

int bell_validate_atoi(const char * str, int * intout)
{
// A safer atoi checking for some types of errors
    char *end;

    errno = 0;
    const long sl = strtol(str,&end,10);

    if (end==str || ERANGE==errno)
    {
        if (intout != NULL) *intout = 0;
        return FALSE;
    }
    else if (sl>INT_MAX || sl<INT_MIN)
    {
        if (intout != NULL) *intout = 0;
        return FALSE;
    }

    if(intout != NULL)
    {
        *intout=(int) sl;
    }
    return TRUE;
}
