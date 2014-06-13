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
/*                        Copyright (c) 2000                             */
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
/*               Date:  August 2000                                      */
/*************************************************************************/
/*                                                                       */
/*  Waveforms                                                            */
/*                                                                       */
/*************************************************************************/
#include "cst_alloc.h"
#include "cst_error.h"
#include "cst_string.h"
#include "cst_val.h"
#include "cst_wave.h"

CST_VAL_REGISTER_TYPE(wave,cst_wave)

cst_wave *new_wave()
{
    cst_wave *w = cst_alloc(struct cst_wave_struct,1);
    w->num_samples = 0;
    w->samples = NULL;
    return w;
}

void delete_wave(cst_wave *w)
{
    if (w)
    {
        cst_free(w->samples);
        cst_free(w);
    }
    return;
}

void cst_wave_resize(cst_wave *w,int samples, int num_channels)
{
    int16_t *ns;

    if (!w)
    {
        cst_errmsg("cst_wave_resize: null wave given to resize\n");
        cst_error();
    }
    ns = cst_alloc(int16_t,samples*num_channels);
    if (num_channels == w->num_channels)
        memmove(ns,w->samples,
                sizeof(int16_t) * num_channels *
                (samples < w->num_samples ? samples : w->num_samples));
    cst_free(w->samples);
    w->samples = ns;
    w->num_samples = samples;
    w->num_channels = num_channels;

}
