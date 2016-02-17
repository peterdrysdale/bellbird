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
/*                         Copyright (c) 2011                            */
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
/*             Authors:  Gopala Anumanchipalli (gopalakr@cs.cmu.edu)     */
/*                Date:  November 2011                                   */
/*************************************************************************/

#include <math.h>
#include "cst_cg.h"
#include "cst_spamf0.h"
#include "cst_track.h"
#include "cst_utterance.h"
#include "cst_utt_utils.h"
#include "bell_relation_sym.h"

static void cst_synthtilt(const float frame_advance,
                          const float start, const float peak, const float tiltamp, 
                          const float tiltdur, const float tilttilt,
                          float **f0_out)
{
    float arise,afall,drise,dfall,i;
    int num_frames;
    arise= tiltamp*(1+tilttilt)/2;
    afall= tiltamp*(1-tilttilt)/2;
    drise= tiltdur*(1+tilttilt)/2;
    dfall= tiltdur*(1-tilttilt)/2;
    num_frames=(int)ceil((double)(start/frame_advance));
    // Synthesizing the rise event
    for (i=frame_advance;((num_frames * frame_advance)<(start+(drise/2))) ; num_frames++,i+=frame_advance)
    {
        f0_out[num_frames][0]+= peak - arise + (2 * arise * (i/drise) * (i/drise));
        f0_out[num_frames][0]=exp(f0_out[num_frames][0]);
    }
    for (;((num_frames * frame_advance)<(start+drise)) ; num_frames++,i+=frame_advance)
    {
        f0_out[num_frames][0]+= peak - 2 * arise * (1- (i/drise)) * (1- (i/drise));
        f0_out[num_frames][0]=exp(f0_out[num_frames][0]);
    }
    // Synthesizing the fall event
    for (i=frame_advance;((num_frames * frame_advance)<(start+drise+(dfall/2))) ; num_frames++,i+=frame_advance)
    {
        f0_out[num_frames][0]+= peak + afall - (2 * afall * (i/dfall) * (i/dfall)) - afall;
        f0_out[num_frames][0]=exp(f0_out[num_frames][0]);
    }
    for (;((num_frames * frame_advance)<(start+drise+dfall)) ; num_frames++,i+=frame_advance)
    {
        f0_out[num_frames][0]+= peak + (2 * afall * (1- (i/dfall)) * (1- (i/dfall))) - afall;
        f0_out[num_frames][0]=exp(f0_out[num_frames][0]);
    }
    return ;
}

cst_utterance *cst_spamf0(cst_utterance *utt)
{
    float **f0_out=val_track(UTT_FEAT_VAL(utt,"param_track"))->frames;;
    cst_item *s;
    cst_cg_db *cg_db;
    const cst_cart *acc_tree, *phrase_tree;
    float end,f0val,syldur,start;
    int i,f;
    cg_db = val_cg_db(UTT_FEAT_VAL(utt,"cg_db"));
    int param_track_num_frames = UTT_FEAT_INT(utt,"param_track_num_frames");

    acc_tree = cg_db->spamf0_accent_tree;
    phrase_tree = cg_db->spamf0_phrase_tree;
    end = 0.0;
    i = 0;
    for (s = UTT_REL_HEAD(utt,SEGMENT); s; s=item_next(s))
    {
        end = ffeature_float(s,"end");
        if (cst_streq("pau",ffeature_string(s,"name")))
        {
            f0val=0;
        }
        else
        {
            f0val=val_float(cart_interpret(s,phrase_tree));
        }

        for (; ((i * cg_db->frame_advance) <= end) && (i < param_track_num_frames); i++)
        {
            f0_out[i][0]=f0val;
        }
    }

    for (s=UTT_REL_HEAD(utt,SYLLABLE); s; s=item_next(s))
    {
        f = val_int(cart_interpret(s,acc_tree));
        start = ffeature_float(s,"R:"SYLSTRUCTURE".d1.R:"SEGMENT".p.end");
        syldur = ffeature_float(s,"R:"SYLSTRUCTURE".dn.R:"SEGMENT".end") - start;
        cst_synthtilt(cg_db->frame_advance,
		      start,
                      cg_db->spamf0_accent_vectors[f][0],
                      cg_db->spamf0_accent_vectors[f][2],
                      syldur,
                      cg_db->spamf0_accent_vectors[f][6],
                      f0_out);
    }
    return utt;
}
