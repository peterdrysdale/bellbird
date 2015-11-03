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
/*                      Copyright (c) 2010-2011                          */
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
/*             Author:  Alok Parlikar (aup@cs.cmu.edu)                   */
/*               Date:  April 2010                                       */
/*************************************************************************/
/*                                                                       */
/*  A clustergen generic voice, that can load from a file                */
/*                                                                       */
/*************************************************************************/
#include "cst_error.h"
#include "cst_string.h"
#include "cst_cg_map.h"
#include "bell_file.h"
#include "bell_ff_sym.h"
#include "bell_relation_sym.h"

static char *cg_voice_header_string = "CMU_FLITE_CG_VOXDATA-v2.0";
// This is the supported voice type for this module
// This string must not be set to greater than 199 chars

static int cst_read_int(cst_file fd)
{
    int val;
    int n;

    n = bell_fread(&val,sizeof(int),1,fd);
    if (n != 1)
    {
        cst_errmsg("cst_read_int: Unable to read in int from file.\n");
        cst_error();
    }
    return val;
}

static float cst_read_float(cst_file fd)
{
    float val;
    int n;

    n = bell_fread(&val,sizeof(float),1,fd);
    if (n != 1)
    {
        cst_errmsg("cst_read_float: Unable to read in float from file.\n");
        cst_error();
    }
    return val;
}

static void *cst_read_padded(cst_file fd, int *numbytes)
{
    void* ret;
    int n; 

    *numbytes = cst_read_int(fd);
    ret = (void *)cst_alloc(char,*numbytes);
    n = bell_fread(ret,sizeof(char),*numbytes,fd);
    if (n != (*numbytes))
    {
        cst_errmsg("cst_read_padded: Unable to read in bytes from file.\n");
        cst_error();
    }
    return ret;
}

static char *cst_read_string(cst_file fd)
{
    int numbytes;

    return (char *)cst_read_padded(fd,&numbytes);
}

static void* cst_read_array(cst_file fd)
{
    int temp;
    void* ret;
    ret = cst_read_padded(fd,&temp);
    return ret;
}

static void** cst_read_2d_array(cst_file fd)
{
    int numrows;
    int i;
    void** arrayrows = NULL;

    numrows = cst_read_int(fd);

    if (numrows > 0)
    {
        arrayrows = cst_alloc(void *,numrows);

        for (i=0;i<numrows;i++)
            arrayrows[i] = cst_read_array(fd);
    }

    return arrayrows; 
}

static char **cst_read_db_types(cst_file fd)
{
    char** types;
    int numtypes;
    int i;

    numtypes = cst_read_int(fd);
    types = cst_alloc(char*,numtypes+1);
  
    for (i=0;i<numtypes;i++)
    {
        types[i] = cst_read_string(fd);
    }
    types[i] = 0;
  
    return types;
}

static cst_cart_node* cst_read_tree_nodes(cst_file fd)
{   
    cst_cart_node* nodes;
    int temp;
    int i, num_nodes;
    short vtype;
    char *str;

    num_nodes = cst_read_int(fd);
    nodes = cst_alloc(cst_cart_node,num_nodes+1);

    for (i=0; i<num_nodes; i++)
    {
        bell_fread(&nodes[i].feat,sizeof(char),1,fd);
        bell_fread(&nodes[i].op,sizeof(char),1,fd);
        bell_fread(&nodes[i].no_node,sizeof(short),1,fd);
        bell_fread(&vtype,sizeof(short),1,fd);
        if (vtype == CST_VAL_TYPE_STRING)
        {
            str = cst_read_padded(fd,&temp);
            nodes[i].val = string_val(str);
            cst_free(str);
        }
        else if (vtype == CST_VAL_TYPE_INT)
            nodes[i].val = int_val(cst_read_int(fd));
        else if (vtype == CST_VAL_TYPE_FLOAT)
            nodes[i].val = float_val(cst_read_float(fd));
        else
            nodes[i].val = int_val(cst_read_int(fd));
    }
    nodes[i].val = NULL;

    return nodes;
}

static char* replacefeat(const char * feat, const char* old, const char* new)
{
// Recursively replace internal_ff strings in feat.
// Using shorter ones improves performance of internal_ff
// which was the third most time expensive function
    char* retstring = NULL;  // current string to return (will be recursively updated)
    char* tempstring = NULL; // working copy of string
    size_t retstringlen;     // length of string to return
    size_t oldlen;           // length of old directive
    size_t newlen;           // length of new directive
    size_t offset;           // index of start of directive to be replaced
    char* foundstr;
    const char* p = feat;
    int first = TRUE;        // Is this the first loop of recursion
    size_t i;
    size_t j;

    oldlen = cst_strlen(old);
    newlen = cst_strlen(new);
    while ( (foundstr = strstr(p,old)) != NULL)
    {
        offset = foundstr - p;
        retstringlen = cst_strlen(p) + newlen - oldlen;
        tempstring = cst_alloc(char, retstringlen+1);

        // copy with replacement to tempstring
        // we copy directly - testing suggests faster on these short strings
        for (i=0; i<offset; i++)
        {
            tempstring[i] = p[i];
        }
        for (i=0,j=offset; i<newlen; i++,j++)
        {
            tempstring[j] = new[i];
        }
        for (i=offset+oldlen,j=offset+newlen; j< retstringlen ;i++,j++)
        {
            tempstring[j] = p[i];
        }
        tempstring[retstringlen] = '\0';

        if (!first)
        {
            cst_free(retstring);
        }
        else
        {
            first = FALSE;
        }
        p = tempstring;
        retstring = tempstring;
    }
    return retstring;
}

const char * const ff_translation[] =
{
    "ph_vc",                  PH_VC,
    "ph_vlng",                PH_VLNG,
    "ph_vheight",             PH_VHEIGHT,
    "ph_vfront",              PH_VFRONT,
    "ph_vrnd",                PH_VRND,
    "ph_ctype",               PH_CTYPE,
    "ph_cplace",              PH_CPLACE,
    "ph_cvox",                PH_CVOX,
    "lisp_cg_duration",       LISP_CG_DURATION,
    "lisp_cg_state_pos",      LISP_CG_STATE_POS,
    "lisp_cg_state_place",    LISP_CG_STATE_PLACE,
    "lisp_cg_state_index",    LISP_CG_STATE_INDEX,
    "lisp_cg_state_rindex",   LISP_CG_STATE_RINDEX,
    "lisp_cg_phone_place",    LISP_CG_PHONE_PLACE,
    "lisp_cg_phone_index",    LISP_CG_PHONE_INDEX,
    "lisp_cg_phone_rindex",   LISP_CG_PHONE_RINDEX,
    "lisp_cg_position_in_phrasep",     LISP_CG_POSITION_IN_PHRASEP,
    "lisp_cg_find_phrase_number",      LISP_CG_FIND_PHRASE_NUMBER,
    "lisp_is_pau",            LISP_IS_PAU,
    "word_numsyls",           WORD_NUMSYLS,
    "ssyl_in",                SSYL_IN,
    "ssyl_out",               SSYL_OUT,
    "syl_in",                 SYL_IN,
    "syl_out",                SYL_OUT,
    "syl_break",              SYL_BREAK,
    "lisp_cg_break",          LISP_CG_BREAK,
    "syl_onsetsize",          SYL_ONSETSIZE,
    "syl_codasize",           SYL_CODASIZE,
    "accented",               ACCENTED,
    "asyl_in",                ASYL_IN,
    "asyl_out",               ASYL_OUT,
    "lisp_coda_fric",         LISP_CODA_FRIC,
    "lisp_onset_fric",        LISP_ONSET_FRIC,
    "lisp_coda_stop",         LISP_CODA_STOP,
    "lisp_onset_stop",        LISP_ONSET_STOP,
    "lisp_coda_nasal",        LISP_CODA_NASAL,
    "lisp_onset_nasal",       LISP_ONSET_NASAL,
    "lisp_coda_glide",        LISP_CODA_GLIDE,
    "lisp_onset_glide",       LISP_ONSET_GLIDE,
    "seg_onsetcoda",          SEG_ONSETCODA,
    "pos_in_syl",             POS_IN_SYL,
    "position_type",          POSITION_TYPE,
    "sub_phrases",            SUB_PHRASES,
    "last_accent",            LAST_ACCENT,
    "next_accent",            NEXT_ACCENT,
    "syl_final",              SYL_FINAL,
    "lisp_cg_syl_ratio",      LISP_CG_SYL_RATIO,
    "lisp_cg_phrase_ratio",   LISP_CG_PHRASE_RATIO,
    "lisp_cg_syls_in_phrase", LISP_CG_SYLS_IN_PHRASE,
    "pos_in_phrase",          POS_IN_PHRASE,
    "pos_in_word",            POS_IN_WORD,
    "syllable_duration",      SYLLABLE_DURATION,
    "syl_vowel",              SYL_VOWEL,
    "syl_numphones",          SYL_NUMPHONES,
    "gpos",                   GPOS,
    "lisp_cg_content_words_in_phrase",   LISP_CG_CONTENT_WORDS_IN_PHRASE,
    NULL,                     NULL
};

static char* replaceff(const char * feat)
{
// Replace last component of feat with compact featfunc symbol
// This improves cg voice performance
    size_t featlen;
    const char * newlast = NULL;
    char * retstring = NULL;
    const char * p;
    char * q;
    const char * featendpart;
    int i = 0;

    featlen = cst_strlen(feat);
    for (featendpart=feat+featlen-1; *featendpart != '.' && featendpart > feat; featendpart--)
        ;
    if (*featendpart == '.') featendpart++;

    while (ff_translation[i] != NULL)
    {
        if (cst_streq(featendpart,ff_translation[i]))
        {
            newlast = ff_translation[i+1];
            break;
        }
        i += 2;
    }

    if (newlast != NULL)
    {
        retstring = cst_alloc(char,featlen-cst_strlen(featendpart)+cst_strlen(newlast)+1);
        for (p = feat, q=retstring; p < featendpart; )
        {
            *q++ = *p++;
        }
        for (p = newlast; *p ;)
        {
            *q++ = *p++;
        }
    }
    else
    {
        retstring = cst_strdup(feat);
    }
    return retstring;
}

static char** cst_read_tree_feats(cst_file fd)
{
    char** feats;
    int numfeats;
    int i;
    char* tempfeat;
    char* tempfeat2;

    numfeats = cst_read_int(fd);
    feats = cst_alloc(char *,numfeats+1);

    for (i=0;i<numfeats;i++)
    {
        tempfeat = cst_read_string(fd);
        tempfeat2 = replacefeat(tempfeat,".parent",".P");
        if (tempfeat2)
        {
            cst_free(tempfeat);
            tempfeat = tempfeat2;
        }
        tempfeat2 = replacefeat(tempfeat,":SylStructure",":"SYLSTRUCTURE);
        if (tempfeat2)
        {
            cst_free(tempfeat);
            tempfeat = tempfeat2;
        }
        tempfeat2 = replacefeat(tempfeat,":segstate",":"SEGSTATE);
        if (tempfeat2)
        {
            cst_free(tempfeat);
            tempfeat = tempfeat2;
        }
        tempfeat2 = replacefeat(tempfeat,":mcep_link",":"MCEP_LINK);
        if (tempfeat2)
        {
            cst_free(tempfeat);
            tempfeat = tempfeat2;
        }
        tempfeat2 = replacefeat(tempfeat,":HMMstate",":"HMMSTATE);
        if (tempfeat2)
        {
            cst_free(tempfeat);
            tempfeat = tempfeat2;
        }
        tempfeat2 = replacefeat(tempfeat,":Syllable",":"SYLLABLE);
        if (tempfeat2)
        {
            cst_free(tempfeat);
            tempfeat = tempfeat2;
        }
        tempfeat2 = replacefeat(tempfeat,":Phrase",":"PHRASE);
        if (tempfeat2)
        {
            cst_free(tempfeat);
            tempfeat = tempfeat2;
        }
        tempfeat2 = replacefeat(tempfeat,":Word",":"WORD);
        if (tempfeat2)
        {
            cst_free(tempfeat);
            tempfeat = tempfeat2;
        }
        feats[i] = replaceff(tempfeat);
        cst_free(tempfeat);
    }
    feats[i] = 0;
  
    return feats;
}

static cst_cart* cst_read_tree(cst_file fd)
{
    cst_cart* tree;

    tree = cst_alloc(cst_cart,1);
    tree->rule_table = cst_read_tree_nodes(fd);  
    tree->feat_table = (const char * const *)cst_read_tree_feats(fd);

    return tree;
}

static cst_cart** cst_read_tree_array(cst_file fd)
{
    cst_cart** trees = NULL;
    int numtrees;
    int i;

    numtrees = cst_read_int(fd);
    
    if (numtrees > 0)
    {
        trees = cst_alloc(cst_cart *,numtrees+1);

        for (i=0;i<numtrees;i++)
            trees[i] = cst_read_tree(fd);
        trees[i] = 0;
    }

    return trees; 
}

static dur_stat** cst_read_dur_stats(cst_file fd)
{
    int numstats;
    int i,temp;
    dur_stat** ds;

    numstats = cst_read_int(fd);
    ds = cst_alloc(dur_stat *,(1+numstats));

    /* load structuer values */
    for (i=0;i<numstats;i++)
    {
        ds[i] = cst_alloc(dur_stat,1);
        ds[i]->mean = cst_read_float(fd);
        ds[i]->stddev = cst_read_float(fd);
        ds[i]->phone = cst_read_padded(fd,&temp);
    }
    ds[i] = NULL;

    return ds;
}

static char*** cst_read_phone_states(cst_file fd)
{
    int i,j,count1,count2,temp;
    char*** ps;

    count1 = cst_read_int(fd);
    ps = cst_alloc(char **,count1+1);
    for (i=0;i<count1;i++)
    {
        count2 = cst_read_int(fd);
        ps[i] = cst_alloc(char *,count2+1);
        for (j=0;j<count2;j++)
	{
            ps[i][j]=cst_read_padded(fd,&temp);
	}
        ps[i][j] = 0;
    }
    ps[i] = 0;

    return ps;
}

int cst_cg_read_header(cst_file fd)
{
    char header[200];
    size_t n;
    int endianness;

    n = bell_fread(header,sizeof(char),cst_strlen(cg_voice_header_string)+1,fd);

    if (n < cst_strlen(cg_voice_header_string)+1)
        return -1;

    if (!cst_streq(header,cg_voice_header_string))
        return -1;

    bell_fread(&endianness,sizeof(int),1,fd); /* for byte order check */
    if (endianness != 1)
        return -1;                           /* dumped with other byte order */
  
    return 0;
}

cst_cg_db *cst_cg_load_db(cst_file fd,int num_param_models,int num_dur_models)
{
    cst_cg_db* db = cst_alloc(cst_cg_db,1);
    int i;

    db->name = cst_read_string(fd);
    db->types = (const char**)cst_read_db_types(fd);

    db->num_types = cst_read_int(fd);
    db->sample_rate = cst_read_int(fd);
    db->f0_mean = cst_read_float(fd);
    db->f0_stddev = cst_read_float(fd);

    db->f0_trees = (const cst_cart**) cst_read_tree_array(fd);

    db->num_param_models = num_param_models;
    db->param_trees = cst_alloc(const cst_cart **,db->num_param_models);
    for (i=0; i<db->num_param_models; i++)
        db->param_trees[i] = (const cst_cart **) cst_read_tree_array(fd);

    db->spamf0 = cst_read_int(fd);
    if (db->spamf0)
    {
        db->spamf0_accent_tree = cst_read_tree(fd);
        db->spamf0_phrase_tree = cst_read_tree(fd);
    }

    db->num_channels = cst_alloc(int,db->num_param_models);
    db->num_frames = cst_alloc(int,db->num_param_models);
    db->model_vectors = cst_alloc(const unsigned short **,db->num_param_models);
    for (i=0; i<db->num_param_models; i++)
    {
        db->num_channels[i] = cst_read_int(fd);
        db->num_frames[i] = cst_read_int(fd);
        db->model_vectors[i] =
            (const unsigned short **)cst_read_2d_array(fd);
    }
    /* In voices that were built before, they might have NULLs as the */
    /* the vectors rather than a real model, so adjust the num_param_models */
    /* accordingly -- this wont cause a leak as there is no alloc'd memory */
    /* in the later unset vectors */
    for (i=0; i<db->num_param_models; i++)
    {
        if (db->model_vectors[i] == NULL)
            break;
    }
    db->num_param_models = i;

    if (db->spamf0)
    {
        db->num_channels_spamf0_accent = cst_read_int(fd);
        db->num_frames_spamf0_accent = cst_read_int(fd);
        db->spamf0_accent_vectors = 
            (const float * const *)cst_read_2d_array(fd);
    }

    db->model_min = cst_read_array(fd);
    db->model_range = cst_read_array(fd);

    db->frame_advance = cst_read_float(fd);

    db->num_dur_models = num_dur_models;
    db->dur_stats = cst_alloc(const dur_stat **,db->num_dur_models);
    db->dur_cart = cst_alloc(const cst_cart *,db->num_dur_models);

    for (i=0; i<db->num_dur_models; i++)
    {
        db->dur_stats[i] = (const dur_stat **)cst_read_dur_stats(fd);
        db->dur_cart[i] = (const cst_cart *)cst_read_tree(fd);
    }

    db->phone_states = 
        (const char * const * const *)cst_read_phone_states(fd);

    if (cst_read_int(fd) == 0)
    {
        cst_errmsg("Warning: only do_mlpg voice are supported");
    }
    db->dynwin = cst_read_array(fd);
    db->dynwinsize = cst_read_int(fd);

    db->mlsa_alpha = cst_read_float(fd);
    db->mlsa_beta = cst_read_float(fd);

    cst_read_int(fd); // dummy read since multimodel parameter is misleading
    db->mixed_excitation = cst_read_int(fd);

    db->ME_num = cst_read_int(fd);
    db->ME_order = cst_read_int(fd);
    db->me_h = (const double * const *)cst_read_2d_array(fd);
    
    db->spamf0 = cst_read_int(fd); /* yes, twice, its above too */
    db->gain = cst_read_float(fd);

    return db;
  
}

void cst_cg_free_db(cst_cg_db *db)
{
    /* Only gets called when this isn't populated : I think */ 
    cst_free(db);
}

void cst_read_voice_feature(cst_file fd,char** fname, char** fval)
{
    int temp;
    *fname = cst_read_padded(fd,&temp);
    *fval = cst_read_padded(fd,&temp);
}
