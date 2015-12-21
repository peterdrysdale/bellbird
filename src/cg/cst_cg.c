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
/*                         Copyright (c) 2007                            */
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
/*             Authors:  Alan W Black (awb@cs.cmu.edu)                   */
/*                Date:  November 2007                                   */
/*************************************************************************/
/*                                                                       */
/*  Implementation of Clustergen, Statistical Parameter Synthesizer in   */
/*  Flite                                                                */
/*                                                                       */
/*  A statistical corpus based synthesizer.                              */
/*  See Black, A. (2006), CLUSTERGEN: A Statistical Parametric           */
/*  Synthesizer using Trajectory Modeling", Interspeech 2006 - ICSLP,    */
/*  Pittsburgh, PA.                                                      */
/*  http://www.cs.cmu.edu/~awb/papers/is2006/IS061394.PDF                */
/*                                                                       */
/*  Uses MLSA for resynthesis and MLPG for smoothing                     */
/*  mlsa and mlpg come from Festvox's VC code (which came in turn        */
/*  came from NITECH's HTS                                               */
/*                                                                       */
/*************************************************************************/

#include "cst_cg.h"
#include "cst_item.h"
#include "cst_spamf0.h"
#include "cst_utt_utils.h"
#include "cst_utterance.h"
#include "bell_ff_sym.h"
#include "bell_relation_sym.h"

/* Access model parameters, unpacking them as required */
#define CG_MODEL_VECTOR(M,N,X,Y)                                        \
    (M->model_min[Y]+((float)(M->N[X][Y])/65535.0*M->model_range[Y]))

CST_VAL_REGISTER_TYPE(cg_db,cst_cg_db)

static cst_utterance *cg_make_hmmstates(cst_utterance *utt);
static cst_utterance *cg_make_params(cst_utterance *utt);
static cst_utterance *cg_predict_params(cst_utterance *utt);
static cst_utterance *cg_resynth(cst_utterance *utt);

void delete_cg_db(cst_cg_db *db)
{
    int i,j;

    /* Woo Hoo!  We're gonna free this garbage with a big mallet */
    /* In spite of what the const qualifiers say ... */
    cst_free((void *)db->name);

    for (i=0; db->types && db->types[i]; i++)
        cst_free((void *)db->types[i]);
    cst_free((void *)db->types);

    for (i=0; db->f0_trees && db->f0_trees[i]; i++)
        delete_cart((cst_cart *)(void *)db->f0_trees[i]);
    cst_free((void *)db->f0_trees);

    for (j=0; j<db->num_param_models; j++)
    {
        for (i=0; db->param_trees[j] && db->param_trees[j][i]; i++)
            delete_cart((cst_cart *)(void *)db->param_trees[j][i]);
        cst_free((void *)db->param_trees[j]);
    }
    cst_free((void *)db->param_trees);

    if (db->spamf0)
    {
        delete_cart((cst_cart *)(void *)db->spamf0_accent_tree);
        delete_cart((cst_cart *)(void *)db->spamf0_phrase_tree);
        for (i=0; i< db->num_frames_spamf0_accent; i++)
            cst_free((void *)db->spamf0_accent_vectors[i]);
        cst_free((void *)db->spamf0_accent_vectors);
    }

    for (j=0; j<db->num_param_models; j++)
    {
        for (i=0; i<db->num_frames[j]; i++)
            cst_free((void *)db->model_vectors[j][i]);
        cst_free((void *)db->model_vectors[j]);
    }
    cst_free(db->num_channels);
    cst_free(db->num_frames);
    cst_free((void *)db->model_vectors);

    cst_free((void *)db->model_min);
    cst_free((void *)db->model_range);
    for (j = 0; j<db->num_dur_models; j++)
    {
        for (i=0; db->dur_stats[j] && db->dur_stats[j][i]; i++)
        {
            cst_free((void *)db->dur_stats[j][i]->phone);
            cst_free((void *)db->dur_stats[j][i]);
        }
        cst_free((void *)db->dur_stats[j]);
        delete_cart((void *)db->dur_cart[j]);
    }
    cst_free((void *)db->dur_stats);
    cst_free((void *)db->dur_cart);

    for (i=0; db->phone_states && db->phone_states[i]; i++)
    {
        for (j=0; db->phone_states[i][j]; j++)
            cst_free((void *)db->phone_states[i][j]);
        cst_free((void *)db->phone_states[i]);
    }
    cst_free((void *)db->phone_states);

    cst_free((void *)db->dynwin);

    for (i=0; i<db->ME_num; i++)
        cst_free((void *)db->me_h[i]);
    cst_free((void *)db->me_h);

    cst_free((void *)db);
}

/* */
cst_utterance *cg_synth(cst_utterance *utt)
{
    cst_cg_db *cg_db;
    cg_db = val_cg_db(UTT_FEAT_VAL(utt,"cg_db"));

    cg_make_hmmstates(utt);
    cg_make_params(utt);
    cg_predict_params(utt);
    if (cg_db->spamf0)
    {
	cst_spamf0(utt);
    }
    cg_resynth(utt);

    return utt;
}

static float cg_state_duration(cst_item *s, cst_cg_db *cg_db)
{
    float zdur, dur;
    const char *n;
    int i, x, dm;

    for (dm=0,zdur=0.0; dm < cg_db->num_dur_models; dm++)
        zdur += val_float(cart_interpret(s,cg_db->dur_cart[dm]));
    zdur /= dm;  /* get average zdur prediction from all dur models */
    n = item_feat_string(s,"name");

    /* Note we only use the dur stats from the first model, that is */
    /* correct, but wouldn't be if the dur tree was trained on different */
    /* data */
    for (x=i=0; cg_db->dur_stats[0][i]; i++)
    {
        if (cst_streq(cg_db->dur_stats[0][i]->phone,n))
        {
            x=i;
            break;
        }
    }
    if (!cg_db->dur_stats[0][i])  /* unknown type name */
        x = 0;

    dur = (zdur*cg_db->dur_stats[0][x]->stddev)+cg_db->dur_stats[0][x]->mean;

    /*    dur = 1.2 * (float)exp((float)dur); */

    return dur;
}

static cst_utterance *cg_make_hmmstates(cst_utterance *utt)
{
    /* Build HMM state structure below the segment structure */
    cst_cg_db *cg_db;
    cst_relation *hmmstate, *segstate;
    cst_item *seg, *s, *ss;
    const char *segname;
    int sp,p;

    cg_db = val_cg_db(UTT_FEAT_VAL(utt,"cg_db"));
    hmmstate = utt_relation_create(utt,HMMSTATE);
    segstate = utt_relation_create(utt,SEGSTATE);

    for (seg = UTT_REL_HEAD(utt,SEGMENT); seg; seg=item_next(seg))
    {
        ss = relation_append(segstate,seg);
        segname = item_feat_string(seg,"name");
        for (p=0; cg_db->phone_states[p]; p++)
            if (cst_streq(segname,cg_db->phone_states[p][0]))
                break;
        if (cg_db->phone_states[p] == NULL)
            p = 0;  /* unknown phoneme */
        for (sp=1; cg_db->phone_states[p][sp]; sp++)
        {
            s = relation_append(hmmstate,NULL);
            item_add_daughter(ss,s);
            item_set_string(s,"name",cg_db->phone_states[p][sp]);
            item_set_int(s,"statepos",sp);
        }
    }

    return utt;
}

static cst_utterance *cg_make_params(cst_utterance *utt)
{
    /* puts in the frame items */
    /* historically called "mcep" but can actually be any random vectors */
    cst_cg_db *cg_db;
    cst_relation *mcep, *mcep_link;
    cst_item *s, *mcep_parent, *mcep_frame;
    int num_frames;
    float start, end;
    float dur_stretch, tok_stretch, rdur;

    cg_db = val_cg_db(UTT_FEAT_VAL(utt,"cg_db"));
    mcep = utt_relation_create(utt,MCEP);
    mcep_link = utt_relation_create(utt,MCEP_LINK);
    end = 0.0;
    num_frames = 0;
    dur_stretch = get_param_float(utt->features,"duration_stretch", 1.0);

    for (s = UTT_REL_HEAD(utt,HMMSTATE); s; s=item_next(s))
    {
        start = end;
        tok_stretch = ffeature_float(s,"R:"SEGSTATE".P.R:"SYLSTRUCTURE".P.P.R:"TOKEN".P.local_duration_stretch");
        if (tok_stretch == 0)
            tok_stretch = 1.0;
        rdur = tok_stretch*dur_stretch*cg_state_duration(s,cg_db);
        /* Guarantee duration to be alt least one frame */
        if (rdur < cg_db->frame_advance)
            end = start + cg_db->frame_advance;
        else
            end = start + rdur;
        item_set_float(s,"end",end);
        mcep_parent = relation_append(mcep_link, s);
        for ( ; (num_frames * cg_db->frame_advance) <= end; num_frames++ )
        {
            mcep_frame = relation_append(mcep,NULL);
            item_add_daughter(mcep_parent,mcep_frame);
            item_set_int(mcep_frame,"frame_number",num_frames);
            item_set(mcep_frame,"name",item_feat(mcep_parent,"name"));
        }
    }

    /* Copy duration up onto Segment relation */
    for (s = UTT_REL_HEAD(utt,SEGMENT); s; s=item_next(s))
        item_set(s,"end",ffeature(s,"R:"SEGSTATE".dn.end"));

    UTT_SET_FEAT_INT(utt,"param_track_num_frames",num_frames);

    return utt;
}

static int voiced_frame(cst_item *m)
{
    const char *ph_vc;
    const char *ph_name;

    ph_vc = ffeature_string(m,"R:"MCEP_LINK".P.R:"SEGSTATE".P."PH_VC);
    ph_name = ffeature_string(m,"R:"MCEP_LINK".P.R:"SEGSTATE".P.name");

    if (cst_streq(ph_name,"pau"))
        return 0; /* unvoiced */
    else if (cst_streq("+",ph_vc))
        return 1; /* voiced */
    else if (item_feat_float(m,"voicing") > 0.5)
        /* Even though the range is 0-10, I *do* mean 0.5 */
        return 1; /* voiced */
    else
        return 0; /* unvoiced */
}

static float catmull_rom_spline(float p,float p0,float p1,float p2,float p3)
{
    float q;

    q = ( 0.5 *
          ( ( 2.0 * p1 ) +
            ( p * (-p0 + p2) ) +
            ( (p*p) * (((2.0 * p0) - (5.0 * p1)) +
                       ((4.0 * p2) - p3))) +
            ( (p*p*p) * (-p0 +
                         ((3.0 * p1) - (3.0 * p2)) +
                         p3))));
    return q;
}

static void cg_F0_interpolate_spline(cst_utterance *utt,
                                     cst_track *param_track)
{
    float start_f0, mid_f0, end_f0;
    int start_index, end_index, mid_index;
    int nsi, nei, nmi;  /* next syllable indices */
    float nmid_f0, pmid_f0;
    cst_item *syl;
    int i;
    float m;

    start_f0 = mid_f0 = end_f0 = -1.0;

    for (syl=UTT_REL_HEAD(utt,SYLLABLE); syl; syl=item_next(syl))
    {
        start_index = ffeature_int(syl,"R:"SYLSTRUCTURE".d1.R:"SEGSTATE".d1.R:"MCEP_LINK".d1.frame_number");
        end_index = ffeature_int(syl,"R:"SYLSTRUCTURE".dn.R:"SEGSTATE".dn.R:"MCEP_LINK".dn.frame_number");
        mid_index = (int)((start_index + end_index)/2.0);

        start_f0 = param_track->frames[start_index][0];
        if (end_f0 > 0.0)
            start_f0 = end_f0;  /* not first time through */
        if (mid_f0 < 0.0)
            pmid_f0 = start_f0;  /* first time through */
        else
            pmid_f0 = mid_f0;
        mid_f0 =  param_track->frames[mid_index][0];
        if (item_next(syl)) /* not last syllable */
            end_f0 = (param_track->frames[end_index-1][0]+
                      param_track->frames[end_index][0])/2.0;
        else
            end_f0 = param_track->frames[end_index-1][0];
        nmid_f0=end_f0; /* in case there is no next syl */

        if (item_next(syl))
        {
            nsi = ffeature_int(syl,"n.R:"SYLSTRUCTURE".d1.R:"SEGSTATE".d1.R:"MCEP_LINK".d1.frame_number");
            nei = ffeature_int(syl,"n.R:"SYLSTRUCTURE".dn.R:"SEGSTATE".dn.R:"MCEP_LINK".dn.frame_number");
            nmi = (int)((nsi + nei)/2.0);
            nmid_f0 = param_track->frames[nmi][0];
        }
        /* start to mid syl */
        m = 1.0 / (mid_index - start_index);
        for (i=0; ((start_index+i)<mid_index); i++)
            param_track->frames[start_index+i][0] = 
                 catmull_rom_spline(i*m,pmid_f0,start_f0,mid_f0,end_f0);

        /* mid syl to end */
        m = 1.0 / (end_index - mid_index);
        for (i=0; ((mid_index+i)<end_index); i++)
            param_track->frames[mid_index+i][0] =
                catmull_rom_spline(i*m,start_f0,mid_f0,end_f0,nmid_f0);
    }

    return;
}

static void cg_smooth_F0(cst_utterance *utt,cst_cg_db *cg_db,
                         cst_track *param_track)
{
    /* Smooth F0 and mark unvoiced frames as 0.0 */
    cst_item *mcep;
    int i;
    float mean, stddev;

    cg_F0_interpolate_spline(utt,param_track);

    mean = get_param_float(utt->features,"int_f0_target_mean", cg_db->f0_mean);
    mean *= get_param_float(utt->features,"f0_shift", 1.0);
    stddev = 
        get_param_float(utt->features,"int_f0_target_stddev", cg_db->f0_stddev);
    
    for (i=0,mcep=UTT_REL_HEAD(utt,MCEP); mcep; i++,mcep=item_next(mcep))
    {
        if (voiced_frame(mcep))
        {
            /* scale the F0 -- which normally wont change it at all */
            param_track->frames[i][0] = 
                (((param_track->frames[i][0]-cg_db->f0_mean)/cg_db->f0_stddev) 
                 *stddev)+mean;
            /* Some safety checks */
            if (param_track->frames[i][0] < 50)
                param_track->frames[i][0] = 50;
            if (param_track->frames[i][0] > 700)
                param_track->frames[i][0] = 700;
        }
        else /* Unvoice it */
            param_track->frames[i][0] = 0.0;
    }

    return;
}

static cst_utterance *cg_predict_params(cst_utterance *utt)
{
    cst_cg_db *cg_db;
    cst_track *param_track;
    cst_track *str_track = NULL;
    cst_item *mcep;
    const cst_cart *mcep_tree, *f0_tree;
    int i,j,f,p,o,pm;
    const char *mname;
    float f0_val;
    float local_gain, voicing;
    int extra_feats = 0;

    cg_db = val_cg_db(UTT_FEAT_VAL(utt,"cg_db"));

    extra_feats = 1;  /* voicing */
    if (cg_db->mixed_excitation)
    {
        extra_feats += 5;
        str_track = new_track(UTT_FEAT_INT(utt,"param_track_num_frames"),5);
    }

    param_track = new_track(UTT_FEAT_INT(utt,"param_track_num_frames"),
                     (cg_db->num_channels[0])- (2 * extra_feats));/* no voicing or str */
    f=0;
    for (i=0,mcep=UTT_REL_HEAD(utt,MCEP); mcep; i++,mcep=item_next(mcep))
    {
        mname = item_feat_string(mcep,"name");
        local_gain = ffeature_float(mcep,"R:"MCEP_LINK".P.R:"SEGSTATE".P.R:"SYLSTRUCTURE".P.P.R:"TOKEN".P.local_gain");
        if (local_gain == 0.0) local_gain = 1.0;
        for (p=0; cg_db->types[p]; p++)
            if (cst_streq(mname,cg_db->types[p]))
                break;
        if (cg_db->types[p] == NULL)
            p=0; /* if there isn't a matching tree, use the first one */

        /* Predict F0 */
        f0_tree = cg_db->f0_trees[p];
        f0_val = val_float(cart_interpret(mcep,f0_tree));
        param_track->frames[i][0] = f0_val;
        /* what about stddev ? */

        if (cg_db->num_param_models > 1)
        {   /* MULTI model */
            /* Predict spectral coeffs */
            voicing = 0.0;
            for (pm=0; pm<cg_db->num_param_models; pm++)
            {
                mcep_tree = cg_db->param_trees[pm][p];
                f = val_int(cart_interpret(mcep,mcep_tree));
                /* If there is one model this will be fine, if there are */
                /* multiple models this will be the nth model */
                item_set_int(mcep,"clustergen_param_frame",f);

                /* Old code used to average in param[0] with F0 too (???) */

                for (j=2; j<param_track->num_channels; j++)
                {
                    if (pm == 0) param_track->frames[i][j] = 0.0;
                    param_track->frames[i][j] +=
                        CG_MODEL_VECTOR(cg_db,model_vectors[pm],f,(j))/
                        (float)cg_db->num_param_models;
                }

//              mixed excitation
                if (cg_db->mixed_excitation)
                {
                    o = j;
                    for (j=0; j<5; j++)
                    {
                        if (pm == 0) str_track->frames[i][j] = 0.0;
                        str_track->frames[i][j] +=
                            CG_MODEL_VECTOR(cg_db,model_vectors[pm],f,
                                            (o+(2*j))) /
                            (float)cg_db->num_param_models;
                    }
                }

                /* last coefficient is average voicing for cluster */
                voicing /= (float)(pm+1);
                voicing +=
                    CG_MODEL_VECTOR(cg_db,model_vectors[pm],f,
                                    cg_db->num_channels[pm]-2) /
                    (float)(pm+1);
            }
            item_set_float(mcep,"voicing",voicing);
            /* Apply local gain to c0 */
            param_track->frames[i][2] *= local_gain;

        }
        else
        {   /* SINGLE model */
            /* Predict Spectral */
            mcep_tree = cg_db->param_trees[0][p];
            f = val_int(cart_interpret(mcep,mcep_tree));
            item_set_int(mcep,"clustergen_param_frame",f);

            param_track->frames[i][0] =
                (param_track->frames[i][0]+
                 CG_MODEL_VECTOR(cg_db,model_vectors[0],f,0))/2.0;

            for (j=2; j<param_track->num_channels; j++)
                param_track->frames[i][j] =
                    CG_MODEL_VECTOR(cg_db,model_vectors[0],f,(j));

            if (cg_db->mixed_excitation)
            {
                o = j;
                for (j=0; j<5; j++)
                {
                    str_track->frames[i][j] =
                        CG_MODEL_VECTOR(cg_db,model_vectors[0],f,(o+(2*j)));
                }
            }
            /* last coefficient is average voicing for cluster */
            item_set_float(mcep,"voicing",
                           CG_MODEL_VECTOR(cg_db,model_vectors[0],f,
                                           cg_db->num_channels[0]-2));
        }
    }

    cg_smooth_F0(utt,cg_db,param_track);

    UTT_SET_FEAT(utt,"param_track",track_val(param_track));
    if (cg_db->mixed_excitation)
        UTT_SET_FEAT(utt,"str_track",track_val(str_track));

    return utt;
}

static cst_utterance *cg_resynth(cst_utterance *utt)
{
    cst_cg_db *cg_db;
    cst_wave *w;
    cst_track *param_track;
    cst_track *str_track = NULL;
    cst_track *smoothed_track;

    cg_db = val_cg_db(UTT_FEAT_VAL(utt,"cg_db"));
    param_track = val_track(UTT_FEAT_VAL(utt,"param_track"));
    if (cg_db->mixed_excitation)
        str_track = val_track(UTT_FEAT_VAL(utt,"str_track"));

    smoothed_track = cg_mlpg(param_track, cg_db);
    w = mlsa_resynthesis(smoothed_track,str_track,cg_db);
    delete_track(smoothed_track);

    if (w == NULL)
    {
        /* Synthesis Failed, probably because it was interrupted */
        UTT_SET_FEAT_INT(utt,"Interrupted",1);
        w = new_wave();
    }

    utt_set_wave(utt,w);

    return utt;
}
