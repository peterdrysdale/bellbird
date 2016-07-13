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
/*                       Copyright (c) 2010-2011                         */
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
/*  Load a clustergen voice from a file                                  */
/*                                                                       */
/*************************************************************************/

#include "flite.h"
#include "cst_error.h"
#include "cst_string.h"
#include "cst_features.h"
#include "cst_cg.h"
#include "cst_cart.h"
#include "cst_val.h"
#include "cst_synth.h"
#include "bell_file.h"
#include "bell_ff_sym.h"
#include "bell_relation_sym.h"

static char *cg_voice_header_string = "CMU_FLITE_CG_VOXDATA-v2.0";
// This is the supported voice type for this module
// This string must not be set to greater than 199 chars

static bell_boolean bell_read_int(FILE *fd, int *pval)
{
    int n;

    n = bell_fread(pval,sizeof(int),1,fd);
    if (n != 1)
    {
        return FALSE;
    }
    return TRUE;
}

static bell_boolean bell_read_float(FILE *fd, float* pval)
{
    int n;

    n = bell_fread(pval,sizeof(float),1,fd);
    if (n != 1)
    {
        return FALSE;
    }
    return TRUE;
}

static int bell_read_bytes(FILE *fd, void** ppval)
{
    int bytestoread;
    int n;


    if ((bell_read_int(fd, &bytestoread) == TRUE)
        && (bytestoread > 0)
        && (bytestoread < 1000000)) // this part of a voice file should not be greater than ~1MB
    {
        *ppval = (void *)cst_alloc(char,bytestoread);
        n = bell_fread(*ppval, sizeof(char), bytestoread, fd);
        if (n != bytestoread)
        {
            cst_free(*ppval);
            return -1;
        }
        else
        {
            return n;
        }
    }
    return -1;
}

static bell_boolean bell_read_string(FILE *fd, char **pstring)
{
    int n;
    n = bell_read_bytes(fd, (void *)pstring);
    if (n >=0 && n < 3000) // String should not be too large
    {
        return TRUE;
    }
    else if (n > 3000)
    {
        cst_free(*pstring);
    }
    return FALSE;
}

static bell_boolean bell_read_2d_array(FILE *fd, void ***parrayrows)
{
    int numbytes;
    int numrows;
    int i;
    int n;
    void **arrayrows = NULL;
    bell_boolean err = FALSE;

    if (bell_read_int(fd,&numrows) != TRUE)
    {
        err = TRUE;
    }

    if  ((err != TRUE) && (numrows > 0) && (numrows < 100000))
    {
        arrayrows = cst_alloc(void *,numrows);
        if (TRUE == bell_read_int(fd,&numbytes)
            && (numbytes >= 0 )
            &&(numbytes*numrows < 10000000))
        {
            arrayrows[0] = (void *) cst_alloc(char,(numrows*numbytes));
            n = bell_fread(arrayrows[0],sizeof(char),numbytes,fd);
            if (n != numbytes)
            {
                err = TRUE;
            }
            else
            {
                for (i=1; i < numrows; i++) {
                    arrayrows[i] = arrayrows[i-1] + numbytes;
                    if ((bell_read_int(fd,&n) != TRUE) && (n != numbytes))
                    {
                        err = TRUE;
                    }
                    n = bell_fread(arrayrows[i],sizeof(char),numbytes,fd);
                    if (n != numbytes)
                    {
                        err = TRUE;
                    }
                }
            }
            if (TRUE == err)
            {
                cst_free(arrayrows[0]);
            }
        }
        if (TRUE == err)
        {
            cst_free(arrayrows);
            arrayrows = NULL;
            err = TRUE;
        }
    }

    *parrayrows = arrayrows;
    if (TRUE == err) return FALSE;
    return TRUE;
}

static const char **bell_read_db_types(FILE *fd)
{
    char** types;
    int n;
    int i, j;
    bell_boolean err = FALSE;

    if ((TRUE != bell_read_int(fd, &n)) || (n < 0) || (n > 10000))
    {
        return NULL;
    }

    types = cst_alloc(char*,n+1);

    for (i = 0; i < n; i++)
    {
        if (TRUE != bell_read_string(fd, &types[i]))
        {
            err = TRUE;
            break;
        }
    }

    if (TRUE == err)
    {
        for (j = 0; j < i; j++)
        {
            cst_free(types[j]);
        }
        cst_free(types);
        return NULL;
    }

    types[i] = 0;
    return (const char **)types;
}

static cst_cart_node* bell_read_tree_nodes(FILE *fd)
{
    cst_cart_node* nodes;
    int tempint;
    float tempfloat;
    int i, j, n;
    short vtype;
    char *str;
    bell_boolean err = FALSE;

    if ((TRUE != bell_read_int(fd,&n)) || (n < 0) || (n > 100000))
    {
        return NULL;
    }
    nodes = cst_alloc(cst_cart_node, n+1);

    for (i = 0; i < n; i++)
    {
        if (1 != bell_fread(&nodes[i].feat, sizeof(char), 1, fd))
        {
            err = TRUE;
            break;
        }
        if (1 != bell_fread(&nodes[i].op, sizeof(char), 1, fd))
        {
            err = TRUE;
            break;
        }
        if (1 != bell_fread(&nodes[i].no_node, sizeof(short), 1, fd))
        {
            err = TRUE;
            break;
        }
        if (1 != bell_fread(&vtype, sizeof(short), 1, fd))
        {
            err = TRUE;
            break;
        }
        if (vtype == CST_VAL_TYPE_STRING)
        {
            if (TRUE != bell_read_string(fd, &str))
            {
                err = TRUE;
                break;
            }
            else
            {
                nodes[i].val = string_val(str);
                cst_free(str);
            }
        }
        else if (vtype == CST_VAL_TYPE_INT)
        {
            if (TRUE != bell_read_int(fd, &tempint))
            {
                err = TRUE;
                break;
            }
            else
            {
                nodes[i].val = int_val(tempint);
            }
        }
        else if (vtype == CST_VAL_TYPE_FLOAT)
        {
            if (TRUE != bell_read_float(fd, &tempfloat))
            {
                err = TRUE;
                break;
            }
            else
            {
                nodes[i].val = float_val(tempfloat);
            }
        }
        else
        {
            if (TRUE != bell_read_int(fd, &tempint))
            {
                err = TRUE;
                break;
            }
            else
            {
                nodes[i].val = int_val(tempint);
            }
        }
    }
    if (TRUE == err)
    {
        for (j = 0; j < i; j++)
        {
            delete_val((cst_val *)nodes[j].val);
        }
        cst_free(nodes);
        return NULL;
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

static const char * const ff_translation[] =
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

static const char* const * bell_read_tree_feats(FILE *fd)
{
    char** feats;
    int n;
    int i, j;
    char* tempfeat;
    char* tempfeat2;
    bell_boolean err = FALSE;

    if ((TRUE != bell_read_int(fd, &n)) || (n < 0) || (n > 1000))
    {
        return NULL;
    }
    feats = cst_alloc(char *,n+1);

    for (i = 0; i < n; i++)
    {
        if (TRUE != bell_read_string(fd, &tempfeat))
        {
            err = TRUE;
            break;
        }
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
    if (TRUE == err)
    {
        for (j = 0; j < i; j++)
        {
            cst_free(feats[j]);
        }
        cst_free(feats);
        return NULL;
    }
    feats[i] = 0;

    return (const char * const *)feats;
}

static cst_cart* bell_read_tree(FILE *fd)
{
    cst_cart* tree;

    tree = cst_alloc(cst_cart,1);
    tree->rule_table = bell_read_tree_nodes(fd);
    tree->feat_table = bell_read_tree_feats(fd);

    if ((NULL == tree->rule_table) || (NULL == tree->feat_table))
    {
        delete_cart(tree);
        return NULL;
    }
    return tree;
}

static bell_boolean bell_read_tree_array(FILE *fd, cst_cart ***ptrees)
{
    cst_cart** trees = NULL;
    int n;
    int i, j;
    bell_boolean err = FALSE;

    if ((TRUE != bell_read_int(fd, &n)) || (n < 0) || (n > 200))
    {
        return FALSE;
    }
    if (n > 0)
    {
        trees = cst_alloc(cst_cart *, n+1);
        for (i = 0; i < n; i++)
        {
            trees[i] = bell_read_tree(fd);
            if (NULL == trees[i])
            { // error reading tree
                err = TRUE;
                break;
            }
        }
        trees[i] = 0;

        if (TRUE == err)
        { // Clean up after error reading one of the trees
            for (j = 0; j < i; j++)
            {
                delete_cart(trees[j]);
            }
            cst_free(trees);
            *ptrees = NULL;
            return FALSE;
        }
    }
    *ptrees = trees;
    return TRUE;
}

static dur_stat** bell_read_dur_stats(FILE *fd)
{
    int n;
    int i, j;
    float tempfloat;
    dur_stat** ds;
    bell_boolean err = FALSE;

    if ((TRUE != bell_read_int(fd, &n)) || (n < 0) || (n > 1000))
    {
        return NULL;
    }
    ds = cst_alloc(dur_stat *,(1 + n));

    /* load structure values */
    for (i = 0; i < n; i++)
    {
        ds[i] = cst_alloc(dur_stat,1);
        if (TRUE != bell_read_float(fd, &tempfloat))
        {
            err = TRUE;
            break;
        }
        ds[i]->mean = tempfloat;
        if (TRUE != bell_read_float(fd, &tempfloat))
        {
            err = TRUE;
            break;
        }
        ds[i]->stddev = tempfloat;
        if (TRUE != bell_read_string(fd, &(ds[i]->phone)))
        {
            err = TRUE;
            break;
        }
    }
    ds[i] = NULL;

    if (TRUE == err)
    { // Clean up if an error has occurred with any dur_stat element
        for (j = 0; j < i; j++)
        {
            cst_free(ds[j]->phone);
            cst_free(ds[j]);
        }
        cst_free(ds);
        return NULL;
    }

    return ds;
}

static char*** bell_read_phone_states(FILE *fd)
{
    int n, n2;
    int i, j;
    int k, l;
    char*** ps;
    bell_boolean err = FALSE;

    if ((TRUE != bell_read_int(fd, &n)) || (n < 0) || (n > 1000))
    {
        return NULL;
    }
    ps = cst_alloc(char **, n + 1);

    for (i = 0; i < n; i++)
    {
        if ((TRUE != bell_read_int(fd, &n2)) || (n2 < 0) || (n2 > 100))
        {
            err = TRUE;
            break;
        }
        ps[i] = cst_alloc(char *, n2 + 1);
        for (j = 0; j < n2; j++)
	{
            if (TRUE != bell_read_string(fd, &(ps[i][j])))
            {
                err = TRUE;
                break;
            }
	}
        ps[i][j] = 0;
        if (TRUE == err)
        {
            for (k = 0; k < j; k++)
            {
                cst_free(ps[i][k]);
            }
            cst_free(ps[i]);
            break;
        }
    }
    if (TRUE == err)
    {
        for(k = 0; k < i; k++)
        {
            for(l = 0; ps[k][l] != 0; l++)
            {
                cst_free(ps[k][l]);
            }
            cst_free(ps[k]);
        }
        cst_free(ps);
        return NULL;
    }
    ps[i] = 0;

    return ps;
}

static bell_boolean bell_cg_read_header(FILE *fd)
{
    char header[200];
    size_t n;
    int endianness;

    n = bell_fread(header,sizeof(char),cst_strlen(cg_voice_header_string)+1,fd);

    if (n < cst_strlen(cg_voice_header_string)+1)
        return FALSE;

    if (!cst_streq(header,cg_voice_header_string))
        return FALSE;

    bell_fread(&endianness,sizeof(int),1,fd); /* for byte order check */
    if (endianness != 1)
        return FALSE;                           /* dumped with other byte order */

    return TRUE;
}

static cst_cg_db *cst_cg_load_db(FILE *fd,int num_param_models,int num_dur_models)
{
    cst_cg_db* db = new_cg_db();
    int i, j, k;
    int tempint;
    float tempfloat;
    void *tempvoid;
    void **temparray;
    char *tempstring;
    cst_cart **tempcarts;
    bell_boolean err = FALSE;

    if (TRUE == bell_read_string(fd, &tempstring))
    {
        db->name = tempstring;
    }
    else
    {
        err = TRUE;
    }
    if (FALSE == err)
    {
        db->types = bell_read_db_types(fd);
        if (NULL == db->types) err = TRUE;
    }
    if ((FALSE == err) && (TRUE == bell_read_int(fd, &tempint)))
    {
        db->num_types = tempint;
    }
    else
    {
        err = TRUE;
    }
    if ((FALSE == err) && (TRUE == bell_read_int(fd, &tempint)))
    {
        db->sample_rate = tempint;
    }
    else
    {
        err = TRUE;
    }
    if ((FALSE == err) && (TRUE == bell_read_float(fd, &tempfloat)))
    {
        db->f0_mean = tempfloat;
    }
    else
    {
        err = TRUE;
    }
    if ((FALSE == err) && (TRUE == bell_read_float(fd, &tempfloat)))
    {
        db->f0_stddev = tempfloat;
    }
    else
    {
        err = TRUE;
    }
    if ((FALSE == err) && (TRUE == bell_read_tree_array(fd, &tempcarts)))
    {
        db->f0_trees = (const cst_cart**) tempcarts;
    }
    else
    {
        err = TRUE;
    }
    db->num_param_models = num_param_models;
    if (FALSE == err)
    {
        db->param_trees = cst_alloc(const cst_cart **,db->num_param_models);
        for (i = 0; i < db->num_param_models; i++)
        {
            if (TRUE == bell_read_tree_array(fd, &tempcarts))
            {
                db->param_trees[i] = (const cst_cart **) tempcarts;
            }
            else
            {
                err = TRUE;
                break;
            }
        }
        if (TRUE == err)
        { // Clean up partially read structures on error
            for (j = 0; j < i; j++)
            {
                for (k = 0; db->param_trees[j][k]; k++)
                {
                    delete_cart((cst_cart *)(void *)db->param_trees[j][k]);
                }
                cst_free((void *)db->param_trees[j]);
            }
            cst_free(db->param_trees);
            db->param_trees = NULL;
        }
    }
    if ((FALSE == err) && (TRUE == bell_read_int(fd, &tempint)))
    {
        db->spamf0 = tempint;
    }
    else
    {
        err = TRUE;
    }
    if (db->spamf0)
    {
        if (FALSE == err) db->spamf0_accent_tree = bell_read_tree(fd);
        if (NULL == db->spamf0_accent_tree) err = TRUE;
        if (FALSE == err) db->spamf0_phrase_tree = bell_read_tree(fd);
        if (NULL == db->spamf0_phrase_tree) err = TRUE;
    }
    if (FALSE == err)
    {
        db->num_channels = cst_alloc(int,db->num_param_models);
        db->num_frames = cst_alloc(int,db->num_param_models);
        db->model_vectors = cst_alloc(const unsigned short **,db->num_param_models);
        for (i=0; i<db->num_param_models; i++)
        {
            if (TRUE == bell_read_int(fd,&tempint))
            {
                db->num_channels[i] = tempint;
            }
            else
            {
                err = TRUE;
                break;
            }
            if (TRUE == bell_read_int(fd,&tempint))
            {
                db->num_frames[i] = tempint;
            }
            else
            {
                err = TRUE;
                break;
            }
            if (TRUE == bell_read_2d_array(fd, &temparray))
            {
                db->model_vectors[i] = (const unsigned short **) temparray;
            }
            else
            {
                err = TRUE;
                break;
            }
        }
        if (TRUE == err)
        { // Clean up partially read structures after error
            cst_free(db->num_channels);
            db->num_channels = NULL;
            cst_free(db->num_frames);
            db->num_frames = NULL;
            for (j = 0; j < i; j++)
            {
                cst_free(db->model_vectors[j]);
            }
            cst_free(db->model_vectors);
            db->model_vectors = NULL;
        }
        else
        {
        // In voices that were built before, they might have NULLs as the
        // the vectors rather than a real model, so adjust the num_param_models
        // accordingly -- this wont cause a leak as there is no alloc'd memory
        // in the later unset vectors
            for (i=0; i<db->num_param_models; i++)
            {
                if (db->model_vectors[i] == NULL)
                    break;
            }
            db->num_param_models = i;
        }
    }
    if (db->spamf0)
    {
        if ((FALSE == err) && (TRUE == bell_read_int(fd, &tempint)))
        {
            db->num_channels_spamf0_accent = tempint;
        }
        else
        {
            err = TRUE;
        }
        if ((FALSE == err) && (TRUE == bell_read_int(fd, &tempint)))
        {
            db->num_frames_spamf0_accent = tempint;
        }
        else
        {
            err = TRUE;
        }
        if ((FALSE == err) && (TRUE == bell_read_2d_array(fd, &temparray)))
        {
            db->spamf0_accent_vectors = (const float * const *) temparray;
        }
        else
        {
            err = TRUE;
        }
    }
    if ((FALSE == err) && (bell_read_bytes(fd, &tempvoid) >= 0))
    {
        db->model_min = tempvoid;
    }
    else
    {
        err = TRUE;
    }
    if ((FALSE == err) && (bell_read_bytes(fd, &tempvoid) >= 0))
    {
        db->model_range = tempvoid;
    }
    else
    {
        err = TRUE;
    }
    if ((FALSE == err) && (TRUE == bell_read_float(fd, &tempfloat)))
    {
        db->frame_advance = tempfloat;
    }
    else
    {
        err = TRUE;
    }
    db->num_dur_models = num_dur_models;
    if (FALSE == err)
    {
        db->dur_stats = cst_alloc(const dur_stat **,db->num_dur_models);
        db->dur_cart = cst_alloc(const cst_cart *,db->num_dur_models);

        for (i=0; i<db->num_dur_models; i++)
        {
            db->dur_stats[i] = (const dur_stat **)bell_read_dur_stats(fd);
            if (NULL == db->dur_stats[i])
            {
                err = TRUE;
                break;
            }
            db->dur_cart[i] = (const cst_cart *)bell_read_tree(fd);
            if (NULL == db->dur_cart[i])
            {
                // First clean up dur_stats from this iteration
                for (k = 0; db->dur_stats[i][k]; k++)
                {
                    cst_free((void *)db->dur_stats[i][k]->phone);
                    cst_free((void *)db->dur_stats[i][k]);
                }
                cst_free((void *)db->dur_stats[i]);
                err = TRUE;
                break;
            }
        }
        if (TRUE == err)
        { //Clean up partially loaded structures on error
            for (j = 0; j < i; j++)
            {
                // Clean up dur_stats and dur_cart from previous iterations before error
                for (k = 0; db->dur_stats[j][k]; k++)
                {
                    cst_free((void *)db->dur_stats[j][k]->phone);
                    cst_free((void *)db->dur_stats[j][k]);
                }
                cst_free((void *)db->dur_stats[j]);
                delete_cart((void *)db->dur_cart[j]);
            }
            cst_free(db->dur_stats);
            db->dur_stats = NULL;
            cst_free(db->dur_cart);
            db->dur_cart = NULL;
        }
    }
    if (FALSE == err)
    {
        db->phone_states =
            (const char * const * const *)bell_read_phone_states(fd);
        if (NULL == db->phone_states) err = TRUE;
    }
    if ((FALSE == err) && (TRUE == bell_read_int(fd, &tempint)))
    {
        if (0 == tempint)
        {
            cst_errmsg("Warning: only do_mlpg voice are supported");
            err = TRUE;
        }
    }
    else
    {
        err = TRUE;
    }
    if ((FALSE == err) && (bell_read_bytes(fd, &tempvoid) >= 0))
    {
        db->dynwin = tempvoid;
    }
    else
    {
        err = TRUE;
    }
    if ((FALSE == err) && (TRUE == bell_read_int(fd, &tempint)))
    {
        db->dynwinsize = tempint;
    }
    else
    {
        err = TRUE;
    }
    if ((FALSE == err) && (TRUE == bell_read_float(fd, &tempfloat)))
    {
        db->mlsa_alpha = tempfloat;
    }
    else
    {
        err = TRUE;
    }
    if ((FALSE == err) && (TRUE == bell_read_float(fd, &tempfloat)))
    {
        db->mlsa_beta = tempfloat;
    }
    else
    {
        err = TRUE;
    }
    if ((FALSE == err) && (TRUE == bell_read_int(fd, &tempint)))
    {
        ; // dummy read since multimodel parameter is misleading
    }
    else
    {
        err = TRUE;
    }
    if ((FALSE == err) && (TRUE == bell_read_int(fd, &tempint)))
    {
        db->mixed_excitation = tempint;
    }
    else
    {
        err = TRUE;
    }
    if ((FALSE == err) && (TRUE == bell_read_int(fd, &tempint)))
    {
        db->ME_num = tempint;
    }
    else
    {
        err = TRUE;
    }
    if ((FALSE == err) && (TRUE == bell_read_int(fd, &tempint)))
    {
        db->ME_order = tempint;
    }
    else
    {
        err = TRUE;
    }
    if ((FALSE == err) && (TRUE == bell_read_2d_array(fd, &temparray)))
    {
        db->me_h = (const double * const *) temparray;
    }
    else
    {
        err = TRUE;
    }
    if ((FALSE == err) && (TRUE == bell_read_int(fd, &tempint)))
    {
        db->spamf0 = tempint; /* yes, twice, its above too */
    }
    else
    {
        err = TRUE;
    }
    if ((FALSE == err) && (TRUE == bell_read_float(fd, &tempfloat)))
    {
        db->gain = tempfloat;
    }
    else
    {
        err = TRUE;
    }

    if (TRUE == err)
    {
        delete_cg_db(db);
        return NULL;
    }
    return db;
}

static bell_boolean bell_read_feature(FILE *fd,char** fname, char** fval)
{
    if (TRUE != bell_read_string(fd, fname))
    {
        return FALSE;
    }
    if (TRUE != bell_read_string(fd, fval))
    {
        cst_free(*fname);
        return FALSE;
    }
    return TRUE;
}

bell_voice *cst_cg_load_voice(const char *filename,
                             const cst_lang *lang_table)
{
    bell_voice *vox;
    cst_lexicon *lex = NULL;
    int i, end_of_features;
    const char *language;
    cst_cg_db *cg_db;
    char* fname = "";
    char* fval = NULL;
    FILE *vd;
    int num_param_models = 3;
    int num_dur_models = 1;
    int tempint;

    if ((vd = bell_fopen(filename,"rb")) == NULL)
    {
	return NULL;
    }

    if (bell_cg_read_header(vd) != TRUE)
    {
        bell_fclose(vd);
        cst_errmsg("Clustergen voice: \"%s\", appears to have the wrong format \n",filename);
	return NULL;
    }

    vox = new_voice();

    /* Read voice features from the external file */
    /* Read until the feature is "end_of_features" */
    end_of_features = 0;
    while (end_of_features == 0)
    {
	if (TRUE != bell_read_feature(vd, &fname, &fval))
        {
            bell_fclose(vd);
            delete_voice(vox);
            return NULL;
        }
        if (cst_streq(fname, "end_of_features"))
            end_of_features = 1;
        else
        {
// Only set features bellbird actually uses at this time
            if (cst_streq(fname, "language"))
            {
                vox->language = cst_strdup(fval);
            }
            else if (cst_streq(fname, "num_param_models"))
            {
                bell_validate_atoi(fval, &tempint);
                num_param_models=tempint;
            }
            else if (cst_streq(fname, "num_dur_models"))
            {
                bell_validate_atoi(fval, &tempint);
                num_dur_models=tempint;
            }
        }
        cst_free(fname);
        cst_free(fval);
    }

    /* Load up cg_db from external file */
    cg_db = cst_cg_load_db(vd, num_param_models, num_dur_models);
    bell_fclose(vd);

    if (cg_db == NULL)
    {
        delete_voice(vox);
        return NULL;
    }

    /* Use the language feature to initialize the correct voice */
    language = vox->language;

    /* Search Lang table for lang_init() and lex_init(); */
    for (i=0; lang_table[i].lang; i++)
    {
        if (cst_streq(language,lang_table[i].lang))
        {
            (lang_table[i].lang_init)(vox);
            lex = (lang_table[i].lex_init)();
            break;
        }
    }
    if (lex == NULL)
    {   /* Language is not supported */
	delete_cg_db(cg_db);
        delete_voice(vox);
	return NULL;	
    }
    
    /* Things that weren't filled in already. */
    vox->name = cg_db->name;
    feat_set_string(vox->features, "name", cg_db->name);

    vox->lexicon = lex;
    feat_set(vox->features,"postlex_func", uttfunc_val(lex->postlex));

    /* Waveform synthesis */
    feat_set(vox->features, "wave_synth_func", uttfunc_val(&cg_synth));
    vox->cg_db = cg_db;

    return vox;
}
