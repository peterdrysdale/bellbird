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
#include "cst_features.h"
#include "cst_cg.h"
#include "cst_cg_map.h"
#include "bell_file.h"

cst_voice *cst_cg_load_voice(const char *filename,
                             const cst_lang *lang_table)
{
    cst_voice *vox;
    cst_lexicon *lex = NULL;
    int i, end_of_features;
    const char *language;
    const char *xname;
    cst_cg_db *cg_db;
    char* fname;
    char* fval;
    cst_file vd;
    int num_param_models = 3;
    int num_dur_models = 1;
    int tempint;

    if ((vd = bell_fopen(filename,"rb")) == NULL)
    {
        cst_errmsg("Unable to open clustergen voice: \"%s\" \n",filename);
	cst_error();
    }

    if (cst_cg_read_header(vd) != 0)
    {
        bell_fclose(vd);
        cst_errmsg("Clustergen voice: \"%s\", appears to have the wrong format \n",filename);
	cst_error();
    }

    vox = new_voice();

    /* Read voice features from the external file */
    /* Read until the feature is "end_of_features" */
    fname="";
    end_of_features = 0;
    while (end_of_features == 0)
    {
	cst_read_voice_feature(vd,&fname, &fval);
        if (cst_streq(fname,"end_of_features"))
            end_of_features = 1;
        else
        {
// Only set features bellbird actually uses at this time
            if (cst_streq(fname,"language"))
            {
                xname = feat_own_string(vox->features,fname);
                feat_set_string(vox->features,xname, fval);
            }
            else if (cst_streq(fname,"num_param_models"))
            {
                bell_validate_atoi(fval,&tempint);
                num_param_models=tempint;
            }
            else if (cst_streq(fname,"num_dur_models"))
            {
                bell_validate_atoi(fval,&tempint);
                num_dur_models=tempint;
            }
        }
        cst_free(fname);
        cst_free(fval);
    }

    /* Load up cg_db from external file */
    cg_db = cst_cg_load_db(vd,num_param_models,num_dur_models);

    if (cg_db == NULL)
    {
	bell_fclose(vd);
        return NULL;
    }

    /* Use the language feature to initialize the correct voice */
    language = get_param_string(vox->features, "language", "");

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
	/* Delete allocated memory in cg_db */
	cst_cg_free_db(cg_db);
	bell_fclose(vd);
	return NULL;	
    }
    
    /* Things that weren't filled in already. */
    vox->name = cg_db->name;
    feat_set_string(vox->features,"name",cg_db->name);
    
    feat_set(vox->features,"lexicon",lexicon_val(lex));
    feat_set(vox->features,"postlex_func",uttfunc_val(lex->postlex));

    /* Waveform synthesis */
    feat_set(vox->features,"wave_synth_func",uttfunc_val(&cg_synth));
    feat_set(vox->features,"cg_db",cg_db_val(cg_db));
    feat_set_int(vox->features,"sample_rate",cg_db->sample_rate);

    bell_fclose(vd);

    return vox;
}
