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
/*  Lexicon related functions                                            */
/*                                                                       */
/*************************************************************************/
#include "cst_error.h"
#include "cst_features.h"
#include "cst_lexicon.h"
#include "cst_tokenstream.h"
#include "bell_file.h"

CST_VAL_REGISTER_TYPE_NODEL(lexicon,cst_lexicon)

#define WP_SIZE 64

static int lex_match_entry(const char *a, const char *b);
static int lex_lookup_bsearch(const cst_lexicon *l,const char *word);
static int find_full_match(const cst_lexicon *l,
			   int i,const char *word);

static cst_val *cst_lex_make_entry(const cst_lexicon *lex, const cst_string *entry)
{   /* if replace then replace entry in addenda of lex with entry */
    /* else append entry to addenda of lex                        */
    cst_tokenstream *e;
    cst_val *phones = NULL;
    cst_val *ventry;
    const cst_string *w, *p;
    cst_string *word;
    cst_string *pos;
    int i;

    e = ts_open_string(entry,
                       cst_ts_default_whitespacesymbols,
                       "","","");

    w = ts_get(e);
    if (w[0] == '"') /* it was a quoted entry */
    {                   /* so reparse it */
        ts_close(e);
        e = ts_open_string(entry,
                           cst_ts_default_whitespacesymbols,
                           "","","");
        w = ts_get_quoted_token(e,'"','\\');
    }

    word = cst_strdup(w);
    p = ts_get(e);
    if (!cst_streq(":",p)) /* there is a real pos */
    {
        pos = cst_strdup(p);
        p = ts_get(e);
        if (!cst_streq(":",p)) /* there is a real pos */
        {
            bell_fprintf(stdout,"add_addenda: lex %s: expected \":\" in %s\n",
                        lex->name,
                        word);
            cst_free(word); cst_free(pos); ts_close(e);
            return NULL;
        }
    }
    else
        pos = cst_strdup("nil");

    while (!ts_eof(e))
    {
        p = ts_get(e);
        /* Check its a legal phone */
        for (i=0; lex->phone_table[i]; i++)
       {
            if (cst_streq(p,lex->phone_table[i]))
                break;
        }
        if (cst_streq("#",p)) /* comment to end of line */
            break;
        else if (cst_streq("",p)) /* trailing ws at eoln causes this */
            break;
        else if (lex->phone_table[i])
            /* Only add it if its a valid phone */
            phones = cons_val(string_val(p),phones);
        else
        {
            bell_fprintf(stdout,"add_addenda: lex: %s word %s phone %s not in lexicon phoneset\n",
                        lex->name,
                        word,
                        p);
        }
    }

    ventry = cons_val(string_val(word),cons_val(string_val(pos),
                                                val_reverse(phones)));
    cst_free(word); cst_free(pos); ts_close(e);
#if 0
    printf("entry: ");
    val_print(stdout,ventry);
    printf("\n");
#endif

    return ventry;
}

cst_val *cst_lex_load_addenda(const cst_lexicon *lex, const char *lexfile)
{   /* Load an addend from given file, check its phones wrt lex */
    cst_tokenstream *lf;
    const cst_string *line;
    cst_val *e = NULL;
    cst_val *na = NULL;
    int i;

    lf = ts_open(lexfile,"\n","","","");
    if (lf == NULL)
    {
	cst_errmsg("lex_add_addenda: cannot open lexicon file\n");
        return NULL;;
    }

    while (!ts_eof(lf))
    {
        line = ts_get(lf);
        if (line[0] == '#')
            continue;  /* a comment */
        for (i=0; line[i]; i++)
        {
            if (line[i] != ' ')
                break;
        }
        if (line[i])
        {
            e = cst_lex_make_entry(lex,line);
            if (e)
                na = cons_val(e,na);
        }
        else
            continue;  /* a blank line */
    }

    ts_close(lf);
    return val_reverse(na);
}

int in_lex(const cst_lexicon *l, const char *word, const char *pos)
{
    /* return TRUE is its in the lexicon */
    int r = FALSE;
    char *wp;
    size_t wordlen;

    wordlen = cst_strlen(word);
    wp = cst_alloc(char,wordlen+2);
    bell_snprintf(wp,wordlen+2,"%c%s",(pos ? pos[0] : '0'),word);

    if (lex_lookup_bsearch(l,wp) >= 0)
	r = TRUE;

    cst_free(wp);
    return r;
}

cst_val *lex_lookup(const cst_lexicon *l, const char *word, const char *pos)
{
    int index;
    int p;
    const char *q;
    char *wp;
    cst_val *phones = 0;
    size_t wordlen;

    wordlen = cst_strlen(word);
    wp = cst_alloc(char,wordlen+2);
    bell_snprintf(wp,wordlen+2,"%c%s",(pos ? pos[0] : '0'),word);

    index = lex_lookup_bsearch(l,wp);

    if (index >= 0)
    {
        for (p=index-2; l->data[p]; p--)
            for (q=l->phone_hufftable[l->data[p]]; *q; q++)
                phones = cons_val(string_val(l->phone_table[(unsigned char)*q]),
                                 phones);

        phones = val_reverse(phones);
    }
    else if (l->lts_function)
    {
        phones = (l->lts_function)(l,word,"");
    }
    else if (l->lts_rule_set)
    {
        phones = lts_apply(word,
                          "",  /* more features if we had them */
                          l->lts_rule_set);
    }

    cst_free(wp);
    
    return phones;
}

static int lex_uncompress_word(char *ucword,int p,const cst_lexicon *l)
{
    int i,j=0,length;
    unsigned char *cword;

    cword = &l->data[p];
    for (i=0,j=0; cword[i]; i++)
    {
        if (cword[i] == '\1') /* Test for escape character indicating */
                              /* uncompressed byte.                   */
        {
            if (j+2<WP_SIZE)
            {
               ucword[j] = cword[i+1]; /* Copy escaped uncompressed byte */
               i++;
               j += 1;
            }
            else
               break;
        }
        else
        {
            length = cst_strlen(l->entry_hufftable[cword[i]]);
            if (j+length+1<WP_SIZE)
            {
               memmove(ucword+j,l->entry_hufftable[cword[i]],length);
               j += length;
            }
            else
               break;
        }
    }
    ucword[j] = '\0';

    return j;
}
static int lex_data_next_entry(const cst_lexicon *l,int p,int end)
{
    for (p++; p < end; p++)
	if (l->data[p-1] == 255)
	    return p;
    return end;
}
static int lex_data_prev_entry(const cst_lexicon *l,int p,int start)
{
    for (p--; p > start; p--)
	if (l->data[p-1] == 255)
	    return p;
    return start;
}
static int lex_data_closest_entry(const cst_lexicon *l,int p,int start,int end)
{
    int d;

    d=0;
    while ((p-d > start) && 
	   (p+d < end))
    {
	if (l->data[(p+d)-1] == 255)
        {
	    return p+d;
        }
	else if (l->data[(p-d)-1] == 255)
        {
	    return p-d;
        }
	d++;
    }
    return p-d;
}

static int lex_lookup_bsearch(const cst_lexicon *l, const char *word)
{
    int start,mid,end,c;
    /* needs to be longer than longest word in lexicon */
    char word_pos[WP_SIZE];

    start = 0;
    end = l->num_bytes;
    while (start < end) {
	mid = (start + end)/2;

	/* find previous entry start */
	mid = lex_data_closest_entry(l,mid,start,end);
	lex_uncompress_word(word_pos,mid,l);

	c = lex_match_entry(word_pos,word);

	if (c == 0)
        {
	    return find_full_match(l,mid,word);
        }
	else if (c > 0)
	    end = mid;
	else
        {
	    start = lex_data_next_entry(l,mid + 1,end);
        }

#if 0
	if (l->data[start-1] == 255)
	{
	    lex_uncompress_word(word_pos,start,l);
	    printf("start %s %d ",word_pos,start);
	}
	else
	    printf("start NULL %d ",start);
	if (l->data[mid-1] == 255)
	{
	    lex_uncompress_word(word_pos,mid,l);
	    printf("mid %s %d ",word_pos,mid);
	}
	else
	    printf("mid NULL %d ",mid);
	if (l->data[end-1] == 255)
	{
	    lex_uncompress_word(word_pos,end,l);
	    printf("end %s %d ",word_pos,end);
	}
	else
	    printf("end NULL %d ",end);
	printf("\n");
#endif

    }
    return -1;
}

static int find_full_match(const cst_lexicon *l,
			   int i,const char *word)

{
    /* found word, now look for actual match including pos */
    int w, match=i;
    /* needs to be longer than longest word in lexicon */
    char word_pos[WP_SIZE];

    for (w=i; w > 0; )
    {
	lex_uncompress_word(word_pos,w,l);
	if (!cst_streq(word+1,word_pos+1))
	    break;
	else if (cst_streq(word,word_pos))
	    return w;
	match = w;  /* if we can't find an exact match we'll take this one */
        /* go back to last entry */
	w = lex_data_prev_entry(l,w,0);
    }

    for (w=i; w < l->num_bytes;)
    {
	lex_uncompress_word(word_pos,w,l);
	if (!cst_streq(word+1,word_pos+1))
	    break;
	else if (cst_streq(word,word_pos))
	    return w;
        /* go to next entry */
	w = lex_data_next_entry(l,w,l->num_bytes);
    }

    return match;
}

static int lex_match_entry(const char *a, const char *b)
{
    int c;

    c = strcmp(a+1,b+1);

#if 0
    cst_errmsg("Compare a:%s.b:%s.\n",(a+1),(b+1));
#endif

    return c;
}
