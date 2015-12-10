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
/*               Date:  July 1999                                        */
/*************************************************************************/
/*                                                                       */
/*  Tokenizer for strings and files                                      */
/*                                                                       */
/*************************************************************************/
#include "cst_tokenstream.h"
#include "cst_alloc.h"
#include "bell_file.h"

const cst_string * const cst_ts_default_whitespacesymbols = " \t\n\r";

#define TS_BUFFER_SIZE 256
#define TS_CHARCLASS_NONE        0
#define TS_CHARCLASS_WHITESPACE  2
#define TS_CHARCLASS_SINGLECHAR  4
#define TS_CHARCLASS_PREPUNCT    8
#define TS_CHARCLASS_POSTPUNCT  16
#define TS_CHARCLASS(C,CLASS,TS) ((TS)->charclass[(unsigned char)C] & CLASS)

static cst_string ts_getc(cst_tokenstream *ts);

static void set_charclass_table(cst_tokenstream *ts)
{
    int i;
    memset(ts->charclass,0,256);  /* zero everything */
    
    for (i=0; ts->p_whitespacesymbols[i]; i++)
	ts->charclass[(unsigned char)ts->p_whitespacesymbols[i]] |= TS_CHARCLASS_WHITESPACE;
    for (i=0; ts->p_singlecharsymbols[i]; i++)
	ts->charclass[(unsigned char)ts->p_singlecharsymbols[i]] |= TS_CHARCLASS_SINGLECHAR;
    for (i=0; ts->p_prepunctuationsymbols[i]; i++)
	ts->charclass[(unsigned char)ts->p_prepunctuationsymbols[i]] |= TS_CHARCLASS_PREPUNCT;
    for (i=0; ts->p_postpunctuationsymbols[i]; i++)
	ts->charclass[(unsigned char)ts->p_postpunctuationsymbols[i]]|=TS_CHARCLASS_POSTPUNCT;
    return;
}

void set_singlecharsymbols(cst_tokenstream *ts,
                     const cst_string *singlecharsymbols)
{
// function for resetting singlecharsymbols for ssml parsing
// the interface for resetting whitespace, prepunct and postpunct
// symbols has been removed since it was unused and led to logic errors
// within the tokenstream module
    ts->p_singlecharsymbols = singlecharsymbols;

    set_charclass_table(ts);
    return;
}

static void extend_buffer(cst_string **buffer,int *buffer_max, int proposed_max)
{
//  Extend buffer length to be of max. of 20% greater or proposed_max
    int new_max;
    cst_string *new_buffer;

    if ((*buffer_max)+(*buffer_max)/5 > proposed_max)
    {
        new_max = (*buffer_max)+(*buffer_max)/5;
    }
    else
    {
        new_max = proposed_max;
    }
    new_buffer = cst_alloc(cst_string,new_max);
    memmove(new_buffer,*buffer,*buffer_max);
    cst_free(*buffer);
    *buffer = new_buffer;
    *buffer_max = new_max;
}			  

static cst_tokenstream *new_tokenstream(const cst_string *whitespacesymbols,
					const cst_string *singlecharsymbols,
					const cst_string *prepunctsymbols,
					const cst_string *postpunctsymbols)
{   /* Constructor function */
    cst_tokenstream *ts = cst_alloc(cst_tokenstream,1);
    ts->fd = NULL;
    ts->file_pos = 0;
    ts->line_number = 0;
    ts->string_buffer = NULL;
    ts->token_pos = 0;
    ts->whitespace = cst_alloc(cst_string,TS_BUFFER_SIZE);
    ts->ws_max = TS_BUFFER_SIZE;
    if (prepunctsymbols && prepunctsymbols[0])
    {
        ts->prepunctuation = cst_alloc(cst_string,TS_BUFFER_SIZE);
        ts->prep_max = TS_BUFFER_SIZE;
    }
    else
    {
        ts->prepunctuation = NULL;
        ts->prep_max = 0;
    }
    ts->token = cst_alloc(cst_string,TS_BUFFER_SIZE);
    ts->token_max = TS_BUFFER_SIZE;
    if (postpunctsymbols && postpunctsymbols[0])
    {
        ts->postpunctuation = cst_alloc(cst_string,TS_BUFFER_SIZE);
        ts->postp_max = TS_BUFFER_SIZE;
    }
    else
    {
        ts->postpunctuation = NULL;
        ts->postp_max = 0;
    }

    ts->p_whitespacesymbols = whitespacesymbols;
    ts->p_singlecharsymbols = singlecharsymbols;
    ts->p_prepunctuationsymbols = prepunctsymbols;
    ts->p_postpunctuationsymbols = postpunctsymbols;
    set_charclass_table(ts);

    ts->current_char = 0;

    return ts;
}

static void delete_tokenstream(cst_tokenstream *ts)
{
    cst_free(ts->whitespace);
    cst_free(ts->token);
    if (ts->prepunctuation) cst_free(ts->prepunctuation);
    if (ts->postpunctuation) cst_free(ts->postpunctuation);
    cst_free(ts);
}

cst_tokenstream *ts_open(const char *filename,
			 const cst_string *whitespacesymbols,
			 const cst_string *singlecharsymbols,
			 const cst_string *prepunctsymbols,
			 const cst_string *postpunctsymbols)
{
    cst_tokenstream *ts = new_tokenstream(whitespacesymbols,
					  singlecharsymbols,
					  prepunctsymbols,
					  postpunctsymbols);

    if (cst_streq("-",filename))
	ts->fd = stdin;
    else
	ts->fd = bell_fopen(filename,"rb");
    ts_getc(ts);

    if (ts->fd == NULL)
    {
	delete_tokenstream(ts);
	return NULL;
    }
    else
	return ts;
}

cst_tokenstream *ts_open_string(const cst_string *string,
				const cst_string *whitespacesymbols,
				const cst_string *singlecharsymbols,
				const cst_string *prepunctsymbols,
				const cst_string *postpunctsymbols)
{
    cst_tokenstream *ts = new_tokenstream(whitespacesymbols,
					  singlecharsymbols,
					  prepunctsymbols,
					  postpunctsymbols);

    ts->string_buffer = cst_strdup(string);
    ts_getc(ts);

    return ts;
}

void ts_close(cst_tokenstream *ts)
{
    if (ts->fd != NULL)
    {
	if (ts->fd != stdin)
	    bell_fclose(ts->fd);
	ts->fd = NULL; /* just in case close gets called twice */
    }
    if (ts->string_buffer != NULL)
    {
        cst_free(ts->string_buffer);
	ts->string_buffer = NULL;
    }
    delete_tokenstream(ts);
}

static void get_token_sub_part(cst_tokenstream *ts,
			       int charclass,
			       cst_string **buffer,
			       int *buffer_max)
{
    int p;

    for (p=0; ((ts->current_char != EOF) &&
               (TS_CHARCLASS(ts->current_char,charclass,ts)) &&
	       (!TS_CHARCLASS(ts->current_char,
			      TS_CHARCLASS_SINGLECHAR,ts))); p++)
    {
	if (p+1 >= *buffer_max) extend_buffer(buffer,buffer_max,p+1);
	(*buffer)[p] = ts->current_char;
	ts_getc(ts);
    }
    (*buffer)[p] = '\0';
}

/* Can't afford dynamically generate this char class so have separater func */
static void get_token_sub_part_2(cst_tokenstream *ts,
				 cst_string **buffer,
				 int *buffer_max)
// Get token up until EOF or whitespace or singlechar character.
// This will include postpunctuation.
{
    int p;

    for (p=0; ((ts->current_char != EOF) &&
               (!TS_CHARCLASS(ts->current_char,TS_CHARCLASS_WHITESPACE,ts)) &&
	       (!TS_CHARCLASS(ts->current_char,
			      TS_CHARCLASS_SINGLECHAR,ts)));
         p++)
    {
	if (p+1 >= *buffer_max) extend_buffer(buffer,buffer_max,p+1);
	(*buffer)[p] = ts->current_char;
	ts_getc(ts);
    }
    (*buffer)[p] = '\0';
}

int ts_eof(cst_tokenstream *ts)
{
    if (ts->current_char == EOF)
	return TRUE;
    else
	return FALSE;
}

int ts_set_stream_pos(cst_tokenstream *ts, int pos)
{
    /* Note this doesn't preserve line_pos */
    int new_pos, l;

    if (ts->fd){
        bell_fseeko(ts->fd,(off_t)pos,SEEK_SET);
        new_pos = (int)bell_ftello(ts->fd);
    }
    else if (ts->string_buffer)
    {
        l = cst_strlen(ts->string_buffer);
        if (pos > l)
            new_pos = l;
        else if (pos < 0)
            new_pos = 0;
        else
            new_pos = pos;
    }
    else
        new_pos = pos;  /* not sure it can get here */
    ts->file_pos = new_pos;
    ts->current_char = ' ';  /* To be safe (but this is wrong) */

    return ts->file_pos;
}

int ts_get_stream_pos(cst_tokenstream *ts)
{
    return ts->file_pos;
}

static cst_string ts_getc(cst_tokenstream *ts)
{
    if (ts->fd)
    {
	ts->current_char = bell_fgetc(ts->fd);
    }
    else if (ts->string_buffer)
    {
	if (ts->string_buffer[ts->file_pos] == '\0')
	    ts->current_char = EOF;
	else
	    ts->current_char = ts->string_buffer[ts->file_pos];
    }
    
    if (ts->current_char != EOF)
	ts->file_pos++;
    if (ts->current_char == '\n')
	ts->line_number++;
    return ts->current_char;
}

static void extract_postpunctuation(cst_tokenstream *ts)
{
//  Remove postpunctuation from ts->token and add
//  to ts->postpunctuation
    int p,t;

    ts->postpunctuation[0] = '\0';
    if (ts->p_postpunctuationsymbols[0])
    {
        t = cst_strlen(ts->token);
        p=t;
        while (p > 0)
        {
            if ( (ts->token[p] == '\0') ||
                 (TS_CHARCLASS(ts->token[p],TS_CHARCLASS_POSTPUNCT,ts)) )
            {
                p--; // check for '\0' or TS_CHARCLASS_POSTPUNCT char
            }
            else if ( ((ts->token[p]&0xf8) == 0x98) &&
                      ((p-2) > 0) &&
                      (ts->token[p-1] == '\x80') &&
                      (ts->token[p-2] == '\xe2') )
            {
                p -= 3;  //check for utf-8 quotation marks block
                         //UTF codes points U+2018 -> U+201F
            }
            else
            {
                break;
            }
        }

        if (t != p)
        {
	    if (t-p >= ts->postp_max)
	        extend_buffer(&ts->postpunctuation,&ts->postp_max,(t-p));
            /* Copy postpunctuation from token */
	    memmove(ts->postpunctuation,&ts->token[p+1],(t-p));
            /* truncate token at postpunctuation */
            ts->token[p+1] = '\0';
        }
    }
}

const cst_string *ts_get_quoted_token(cst_tokenstream *ts,
					 char quote,
					 char escape)
{
    /* for reading the next quoted token that starts with quote and
       ends with quote, quote may appear only if preceded by escape */
    int p;

    /* skipping whitespace */
    get_token_sub_part(ts,TS_CHARCLASS_WHITESPACE,
		       &ts->whitespace,
		       &ts->ws_max);
    ts->token_pos = ts->file_pos - 1;

    if (ts->current_char == quote)
    {   /* go until quote */
	ts_getc(ts);
        for (p=0; ((ts->current_char != EOF) &&
                   (ts->current_char != quote));
             p++)
        {
            if (p >= ts->token_max) 
                extend_buffer(&ts->token,&ts->token_max,p);
            ts->token[p] = ts->current_char;
            ts_getc(ts);
            if (ts->current_char == escape)
            {
                ts_get(ts);
                if (p >= ts->token_max) 
                    extend_buffer(&ts->token,&ts->token_max,p);
                ts->token[p] = ts->current_char;
                ts_get(ts);
            }
        }
        ts->token[p] = '\0';
	ts_getc(ts);
    }
    else /* its not quoted, like to be careful dont you */
    {    /* treat is as standard token                  */
	/* Get prepunctuation */
        if (ts->current_char != EOF &&
            TS_CHARCLASS(ts->current_char,TS_CHARCLASS_PREPUNCT,ts))
            get_token_sub_part(ts,
                               TS_CHARCLASS_PREPUNCT,
                               &ts->prepunctuation,
                               &ts->prep_max);
        else if (ts->prepunctuation)
            ts->prepunctuation[0] = '\0';
	/* Get the symbol itself */
        if (ts->current_char != EOF &&
            TS_CHARCLASS(ts->current_char,TS_CHARCLASS_SINGLECHAR,ts))
	{
	    if (2 >= ts->token_max) extend_buffer(&ts->token,&ts->token_max,2);
	    ts->token[0] = ts->current_char;
	    ts->token[1] = '\0';
	    ts_getc(ts);
	}
	else
	    get_token_sub_part_2(ts,&ts->token,&ts->token_max);
	/* This'll have token *plus* post punctuation in ts->token */
        if (ts->postpunctuation)
        {
            extract_postpunctuation(ts);
        }
    }

    return ts->token;
}

const cst_string *ts_get(cst_tokenstream *ts)
{
    /* Get next token */

    /* Skip whitespace */
    get_token_sub_part(ts,
		       TS_CHARCLASS_WHITESPACE,
		       &ts->whitespace,
		       &ts->ws_max);

    /* quoted strings currently ignored */
    ts->token_pos = ts->file_pos - 1;
	
    /* Get prepunctuation */
    if (ts->current_char != EOF &&
        TS_CHARCLASS(ts->current_char,TS_CHARCLASS_PREPUNCT,ts))
	get_token_sub_part(ts,
			   TS_CHARCLASS_PREPUNCT,
			   &ts->prepunctuation,
			   &ts->prep_max);
    else if (ts->prepunctuation)
	ts->prepunctuation[0] = '\0';
    /* Get the symbol itself */
    if (ts->current_char != EOF &&
        TS_CHARCLASS(ts->current_char,TS_CHARCLASS_SINGLECHAR,ts))
    {
	if (2 >= ts->token_max) extend_buffer(&ts->token,&ts->token_max,2);
	ts->token[0] = ts->current_char;
	ts->token[1] = '\0';
	ts_getc(ts);
    }
    else
	get_token_sub_part_2(ts,&ts->token,&ts->token_max);
    /* This'll have token *plus* post punctuation in ts->token */
    if (ts->postpunctuation)
    {
        extract_postpunctuation(ts);
    }

    return ts->token;
}
