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

CST_VAL_REGISTER_TYPE(lexicon,cst_lexicon)

#define WP_SIZE 64

static int lex_match_entry(const char *a, const char *b);
static int lex_lookup_bsearch(const cst_lexicon *l,const char *word);
static int find_full_match(const cst_lexicon *l,
			   int i,const char *word);

void delete_lexicon(cst_lexicon *lexicon)
{
//  Delete addenda if it was loaded at runtime
    if (lexicon->lex_addenda)
        delete_val(lexicon->lex_addenda);
}

static cst_val *lex_make_entry(const cst_lexicon *lex, const cst_string *entry)
{
//  Parse and validate a line from pronouncing dictionary addenda
//  Format is the same as Bellbird's uncompressed pronouncing dictionary
    cst_tokenstream *e;
    cst_val *phones = NULL;
    cst_val *ventry;
    const cst_string *p;
    cst_string *word;
    cst_string *pos;
    int entrylen;
    int i;

    entrylen=cst_strlen(entry);
    if (entrylen<5)
        return NULL; // Ignore entries which are clearly too short

//  Extract part of speech information
    pos = cst_alloc(cst_string,2);
    pos[0] = entry[0];
    pos[1] = '\0';

//  Extract word
    word = cst_strdup(entry+1);
    for(i=1; i<entrylen; i++)
    {
        if (entry[i] == ':')
        {
            word[i-1] = '\0';
            break;
        }
    }
    if ( i<2 || i>entrylen-2 )
    {
        cst_free(pos);
        cst_free(word);
        cst_errmsg("Ignoring malformed addenda line:%s\n",entry);
        return NULL;
    }

//  Extract phone information
    e = ts_open_string(entry+i+1," ","","","");

    while (!ts_eof(e))
    {
        p = ts_get(e);
        /* Check for valid phone */
        for (i=0; lex->phone_table[i]; i++)
        {
            if (cst_streq(p,lex->phone_table[i]))
                break;
        }
        if (cst_streq("",p)) /* trailing ws at eoln causes this */
            break;
        else if (lex->phone_table[i])
            phones = cons_val(string_val(p),phones); // Add valid phone
        else
        {
            cst_errmsg("lex_make_entry: lex: %s word %s phone %s not in lexicon phoneset\n",
                        lex->name,
                        word,
                        p);
        }
    }

    ventry = cons_val(string_val(word),cons_val(string_val(pos),
                                                val_reverse(phones)));
    cst_free(word);cst_free(pos); ts_close(e);

    return ventry;
}

cst_val *cst_lex_load_addenda(const cst_lexicon *lex, const char *lexfile)
{
//  Load a pronouncing dictionary addenda at runtime
    cst_tokenstream *lf;
    const cst_string *line;
    cst_val *e = NULL;
    cst_val *na = NULL;

    lf = ts_open(lexfile,"\n","","","");
    if (lf == NULL)
    {
	cst_errmsg("lex_load_addenda: cannot open addenda file\n");
        return NULL;;
    }

    while (!ts_eof(lf))
    {
        line = ts_get(lf);
        if (line[0] == '#')
            continue;  /* a comment */
        e = lex_make_entry(lex,line);
        if (e)
            na = cons_val(e,na);
    }

    ts_close(lf);
    return na;
}

const cst_val *get_entry_lex_addenda(const char *word,const cst_val *lex_addenda)
{
//  Return lex_addenda entry corresponding to word if it exists
    return val_assoc_string(word,lex_addenda);

}

cst_val *get_phones_lex_addenda(const cst_val *entry)
{
//  Extract phones from a lex_addenda entry
    return (cst_val *)val_cdr(val_cdr(entry));
}

int in_lex(const cst_lexicon *l, const char *word, const char *pos)
{
//  Return TRUE if in addenda or lexicon
    int r = FALSE;
    char *wp;
    size_t wordlen;

//  Check if in addenda first and early exit if it is
    if ((l->lex_addenda) && (get_entry_lex_addenda(word,l->lex_addenda)))
    {
        return TRUE;
    }

//  Otherwise look in builtin lexicon
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
