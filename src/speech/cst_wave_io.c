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
/*                         Copyright (c) 2000                            */
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
#include "cst_file.h"
#include "cst_string.h"
#include "cst_wave.h"
#include "bell_file.h"

/* File format definition */
#define RIFF_FORMAT_PCM    0x0001

static int cst_wave_load_riff_header(cst_wave_header *header,cst_file fd)
{
    char info[4];
    short d_short;
    int d_int;
    int rv = BELL_IO_SUCCESS;

    if (bell_fread(info,1,4,fd) != 4)
    {
        cst_errmsg("cst_load_wave_riff: Unable to read all of id (RIFF) in WAV file\n");
	return BELL_IO_ERROR;
    }
    else if (strncmp(info,"RIFF",4) != 0)
    {
        cst_errmsg("cst_load_wave_riff: Unable to find sequence (RIFF) in WAV file\n");
	return BELL_IO_ERROR;
    }

    if(bell_fread(&d_int,4,1,fd) != 1)
    {
        rv = BELL_IO_ERROR;
    }
#ifdef WORDS_BIGENDIAN
    d_int = SWAPINT(d_int);
#endif
    header->num_bytes = d_int;
    
    if ((bell_fread(info,1,4,fd) != 4) ||
	(strncmp(info,"WAVE",4) != 0))
    {
        cst_errmsg("cst_load_wave_riff: Unable to find sequence (WAVE) in WAV file\n");
	return BELL_IO_ERROR;
    }
    if ((bell_fread(info,1,4,fd) != 4) ||
	(strncmp(info,"fmt ",4) != 0))
    {
        cst_errmsg("cst_load_wave_riff: Unable to find sequence (fmt ) in WAV file\n");
	return BELL_IO_ERROR;
    }
    if(bell_fread(&d_int,4,1,fd) != 1)
    {
        rv = BELL_IO_ERROR;
    }
#ifdef WORDS_BIGENDIAN
    d_int = SWAPINT(d_int);
#endif
    header->hsize = d_int;
    if(bell_fread(&d_short,2,1,fd) != 1)
    {
        rv = BELL_IO_ERROR;
    }
#ifdef WORDS_BIGENDIAN
    d_short = SWAPSHORT(d_short);
#endif

    if (d_short != RIFF_FORMAT_PCM)
    {
	cst_errmsg("cst_load_wave_riff: unsupported sample format\n");
	return BELL_IO_ERROR;
    }
    if(bell_fread(&d_short,2,1,fd) != 1)
    {
        rv = BELL_IO_ERROR;
    }
#ifdef WORDS_BIGENDIAN
    d_short = SWAPSHORT(d_short)
#endif

    header->num_channels = d_short;

    if(bell_fread(&d_int,4,1,fd) != 1)
    {
        rv = BELL_IO_ERROR;
    }
#ifdef WORDS_BIGENDIAN
    d_int = SWAPINT(d_int);
#endif
    header->sample_rate = d_int;
    if(bell_fread(&d_int,4,1,fd) != 1)      /* avg bytes per second */
    {
        rv = BELL_IO_ERROR;
    }
    if(bell_fread(&d_short,2,1,fd) != 1)    /* block align */
    {
        rv = BELL_IO_ERROR;
    }
    if(bell_fread(&d_short,2,1,fd) != 1)    /* bits per sample */
    {
        rv = BELL_IO_ERROR;
    }

    if(rv == BELL_IO_ERROR)
    {
        cst_errmsg("cst_load_wave_riff: Unable to read header.\n");
        return BELL_IO_ERROR;
    }

    return BELL_IO_SUCCESS;
}

int cst_wave_append_riff(cst_wave *w,const char *filename)
{
    /* Appends to wave in file if it already exists */
    cst_file fd;
    cst_wave_header hdr;
    char info[4];
    int d_int;
    int rv, num_bytes, n;

    if (w == NULL)
    {
         cst_errmsg("cst_wave_append_riff: No cst_wave to output./n");
         return BELL_IO_ERROR;
    }

    if (cst_streq("-",filename))
    {
      fd=stdout;
#ifdef WORDS_BIGENDIAN
      short *xdata = cst_alloc(short,cst_wave_num_channels(w)*cst_wave_num_samples(w));
      memmove(xdata,cst_wave_samples(w),sizeof(short)*cst_wave_num_channels(w)*
                  cst_wave_num_samples(w));
      swap_bytes_short(xdata,cst_wave_num_channels(w)*cst_wave_num_samples(w));
      n = bell_fwrite(xdata,sizeof(short),cst_wave_num_channels(w)*
                  cst_wave_num_samples(w),fd);
      cst_free(xdata);
#else
      n = bell_fwrite(cst_wave_samples(w),sizeof(short),
		  cst_wave_num_channels(w)*cst_wave_num_samples(w),fd);
#endif
      if(n != cst_wave_num_channels(w)*cst_wave_num_samples(w) )
      {
          cst_errmsg("cst_wave_append: can't write all output samples.\n");
	  return BELL_IO_ERROR;
      }
      rv = BELL_IO_SUCCESS;
    } else  /* else of stdout switch - normal file output */
    {
      if ((fd = bell_fopen(filename,"r+b")) == NULL)
      {
          cst_errmsg("cst_wave_append: can't open file \"%s\"\n",
	  	     filename);
	  return BELL_IO_ERROR;
      }
    
      rv = cst_wave_load_riff_header(&hdr,fd);
      if (rv != BELL_IO_SUCCESS)
      {
	  bell_fclose(fd);
	  return BELL_IO_ERROR;
      }
    /* We will assume this is bellbird/flite file so it has *ONE* data part */
      if(bell_fread(info,1,4,fd) != 4)
      {
          cst_errmsg("cst_wave_append: Unable to read part of header.\n");
          bell_fclose(fd);
          return BELL_IO_ERROR;
      }
      if(bell_fread(&d_int,4,1,fd) != 1)
      {
          cst_errmsg("cst_wave_append: Unable to read part of header.\n");
          bell_fclose(fd);
          return BELL_IO_ERROR;
      }
#ifdef WORDS_BIGENDIAN
      d_int = SWAPINT(d_int);
#endif
      hdr.num_samples = d_int/sizeof(short);

      if(bell_fseeko(fd, bell_ftello(fd)+(hdr.hsize-16)+
	             (hdr.num_samples*hdr.num_channels*sizeof(short)),
	             SEEK_SET) != 0)
      {
          cst_errmsg("cst_wave_append: Unable to seek for append.\n");
          bell_fclose(fd);
          return BELL_IO_ERROR;
      
      }

#ifdef WORDS_BIGENDIAN
      short *xdata = cst_alloc(short,cst_wave_num_channels(w)*cst_wave_num_samples(w));
      memmove(xdata,cst_wave_samples(w),sizeof(short)*cst_wave_num_channels(w)*
                  cst_wave_num_samples(w));
      swap_bytes_short(xdata,cst_wave_num_channels(w)*cst_wave_num_samples(w));
      n = bell_fwrite(xdata,sizeof(short),cst_wave_num_channels(w)*
                  cst_wave_num_samples(w),fd);
      cst_free(xdata);
#else
      n = bell_fwrite(cst_wave_samples(w),sizeof(short),
		  cst_wave_num_channels(w)*cst_wave_num_samples(w),fd);
#endif
      if(n != cst_wave_num_channels(w)*cst_wave_num_samples(w) )
      {
          cst_errmsg("cst_wave_append: can't write all output samples.\n");
          bell_fclose(fd);
	  return BELL_IO_ERROR;
      }

      if(bell_fseeko(fd,4,SEEK_SET) != 0)
      {
          cst_errmsg("cst_wave_append: Unable to seek for append.\n");
          bell_fclose(fd);
          return BELL_IO_ERROR;
      }
      num_bytes = hdr.num_bytes + (n*sizeof(short));
#ifdef WORDS_BIGENDIAN
      num_bytes = SWAPINT(num_bytes);
#endif
      if(bell_fwrite(&num_bytes,4,1,fd) != 1) /* num bytes in whole file */
      {
          cst_errmsg("cst_wave_append: Unable to write num bytes in whole file.\n");
          bell_fclose(fd);
          return BELL_IO_ERROR;
      }

      if(bell_fseeko(fd,4+4+4+4+4+2+2+4+4+2+2+4,SEEK_SET) != 0)
      {
          cst_errmsg("cst_wave_append: Unable to seek for append.\n");
          bell_fclose(fd);
          return BELL_IO_ERROR;
      }
      num_bytes = 
	(sizeof(short) * cst_wave_num_channels(w) * cst_wave_num_samples(w)) +
	(sizeof(short) * hdr.num_channels * hdr.num_samples);
#ifdef WORDS_BIGENDIAN
      num_bytes = SWAPINT(num_bytes);
#endif
      if(bell_fwrite(&num_bytes,4,1,fd) != 1) /* num bytes in data */
      {
          cst_errmsg("cst_wave_append: Unable to write num bytes in data.\n");
          bell_fclose(fd);
          return BELL_IO_ERROR;
      }
      bell_fclose(fd);
    }  /* endif of stdout switch */

    return rv;
}

int cst_wave_save_riff(cst_wave *w,const char *filename)
{
    cst_file fd;
    int rv;

    if (cst_streq("-",filename))
    {
      fd=stdout;
      rv = cst_wave_save_riff_fd(w, fd);
    }
    else
    {
      if ((fd = bell_fopen(filename,"wb")) == NULL)
      {
	  cst_errmsg("cst_wave_save: can't open file \"%s\"\n",
		     filename);
	  return BELL_IO_ERROR;
      }
      rv = cst_wave_save_riff_fd(w, fd);
      bell_fclose(fd);
    }
    return rv;
}

int cst_wave_save_riff_fd(cst_wave *w, cst_file fd)
{
    short d_short;
    int d_int, n;
    int num_bytes;
    int rv = BELL_IO_SUCCESS;

    if (w == NULL)
    {
         cst_errmsg("cst_wave_save_riff_fd: No cst_wave to output.\n");
         return BELL_IO_ERROR;
    }

    if(bell_fwrite("RIFF",4,1,fd) != 1)
    {
         rv = BELL_IO_ERROR;
    }
    num_bytes = (cst_wave_num_samples(w)
		 * cst_wave_num_channels(w)
		 * sizeof(short)) + 8 + 16 + 12;
#ifdef WORDS_BIGENDIAN
    num_bytes = SWAPINT(num_bytes);
#endif
    if(bell_fwrite(&num_bytes,4,1,fd) != 1) /* num bytes in whole file */
    {
         rv = BELL_IO_ERROR;
    }
    if(bell_fwrite("WAVE",1,4,fd) != 4)
    {
         rv = BELL_IO_ERROR;
    }
    if(bell_fwrite("fmt ",1,4,fd) != 4)
    {
         rv = BELL_IO_ERROR;
    }
    num_bytes = 16;                   /* size of header */
#ifdef WORDS_BIGENDIAN
    num_bytes = SWAPINT(num_bytes);
#endif
    if(bell_fwrite(&num_bytes,4,1,fd) != 1)
    {
         rv = BELL_IO_ERROR;
    }
    d_short = RIFF_FORMAT_PCM;        /* sample type */
#ifdef WORDS_BIGENDIAN
    d_short = SWAPSHORT(d_short);
#endif
    if(bell_fwrite(&d_short,2,1,fd) != 1)
    {
         rv = BELL_IO_ERROR;
    }
    d_short = cst_wave_num_channels(w); /* number of channels */
#ifdef WORDS_BIGENDIAN
    d_short = SWAPSHORT(d_short);
#endif
    if(bell_fwrite(&d_short,2,1,fd) != 1)
    {
         rv = BELL_IO_ERROR;
    }         
    d_int = cst_wave_sample_rate(w);  /* sample rate */
#ifdef WORDS_BIGENDIAN
    d_int = SWAPINT(d_int);
#endif
    if(bell_fwrite(&d_int,4,1,fd) != 1)
    {
         rv = BELL_IO_ERROR;
    }
    d_int = (cst_wave_sample_rate(w)
	     * cst_wave_num_channels(w)
	     * sizeof(short));        /* average bytes per second */
#ifdef WORDS_BIGENDIAN
    d_int = SWAPINT(d_int);
#endif
    if(bell_fwrite(&d_int,4,1,fd) != 1)
    {
         rv = BELL_IO_ERROR;
    }
    d_short = (cst_wave_num_channels(w)
	       * sizeof(short));      /* block align */
#ifdef WORDS_BIGENDIAN
    d_short = SWAPSHORT(d_short);
#endif
    if(bell_fwrite(&d_short,2,1,fd) != 1)
    {
         rv = BELL_IO_ERROR;
    }         
    d_short = 2 * 8;                  /* bits per sample */
#ifdef WORDS_BIGENDIAN
    d_short = SWAPSHORT(d_short);
#endif
    if(bell_fwrite(&d_short,2,1,fd) != 1)
    {
         rv = BELL_IO_ERROR;
    }
    if(bell_fwrite("data",1,4,fd) != 4)
    {
         rv = BELL_IO_ERROR;
    }
    d_int = (cst_wave_num_channels(w)
	     * cst_wave_num_samples(w)
	     * sizeof(short));	      /* bytes in data */
#ifdef WORDS_BIGENDIAN
    d_int = SWAPINT(d_int);
#endif
    if(bell_fwrite(&d_int,4,1,fd) !=  1)
    {
         rv = BELL_IO_ERROR;
    }

    if( rv == BELL_IO_ERROR )
    {
         cst_errmsg("cst_wave_save_riff_fd: Unable to write header.\n");
         return BELL_IO_ERROR;
    }

#ifdef WORDS_BIGENDIAN
    short *xdata = cst_alloc(short,cst_wave_num_channels(w)*cst_wave_num_samples(w));
    memmove(xdata,cst_wave_samples(w),sizeof(short)*cst_wave_num_channels(w)*
		   cst_wave_num_samples(w));
    swap_bytes_short(xdata,cst_wave_num_channels(w)*cst_wave_num_samples(w));
    n = bell_fwrite(xdata,sizeof(short),
		   cst_wave_num_channels(w)*cst_wave_num_samples(w),fd);
    cst_free(xdata);
#else
    n = bell_fwrite(cst_wave_samples(w),sizeof(short),
		   cst_wave_num_channels(w)*cst_wave_num_samples(w),fd);
#endif
    if( n != cst_wave_num_channels(w)*cst_wave_num_samples(w) )
    {
        cst_errmsg("cst_wave_save_riff_fd: Unable to write all output samples.\n");
	return BELL_IO_ERROR;
    }
    else
    {
        return BELL_IO_SUCCESS;
    }	
}

int cst_wave_load_riff(cst_wave *w,const char *filename)
{
    cst_file fd;
    int rv;

    if ((fd = bell_fopen(filename,"rb")) == NULL)
    {
	cst_errmsg("cst_wave_load: can't open file \"%s\"\n",
		   filename);
	return BELL_IO_ERROR;
    }

    rv = cst_wave_load_riff_fd(w,fd);
    bell_fclose(fd);
    
    return rv;
}



int cst_wave_load_riff_fd(cst_wave *w,cst_file fd)
{
    cst_wave_header hdr;
    int rv;
    char info[4];
    int d_int, d;
    int data_length;
    int samples;

    rv = cst_wave_load_riff_header(&hdr,fd);
    if (rv != BELL_IO_SUCCESS)
	return BELL_IO_ERROR;
	
    if(bell_fseeko(fd,bell_ftello(fd)+(hdr.hsize-16),SEEK_SET) != 0) /* skip rest of header */
    {
        cst_errmsg("cst_wave_load_riff_fd: Unable to process header.\n");
        return BELL_IO_ERROR;
    }

    /* Note there's a bunch of potential random headers */
    while (1)
    {
	if (bell_fread(info,1,4,fd) != 4)
	    return BELL_IO_ERROR;
	if (strncmp(info,"data",4) == 0)
	{
	    if(bell_fread(&d_int,4,1,fd) != 1)
            {
                cst_errmsg("cst_wave_load_riff_fd: Unable to read header.\n");
                return BELL_IO_ERROR; 
            }
#ifdef WORDS_BIGENDIAN
	    d_int = SWAPINT(d_int);
#endif
	    samples = d_int/sizeof(short);
	    break;
	}
	else if (strncmp(info,"fact",4) == 0)
	{   
	    if(bell_fread(&d_int,4,1,fd) != 1)
            {
                cst_errmsg("cst_wave_load_riff_fd: Unable to read header.\n");
                return BELL_IO_ERROR;
            }
#ifdef WORDS_BIGENDIAN
	    d_int = SWAPINT(d_int);
#endif
	    if(bell_fseeko(fd,bell_ftello(fd)+d_int,SEEK_SET) != 0)
            {
                cst_errmsg("cst_wave_load_riff_fd: Unable to seek in header.\n");
                return BELL_IO_ERROR;
            }
	}
	else if (strncmp(info,"clm ",4) == 0)
	{   /* another random chunk type -- resample puts this one in */
	    if(bell_fread(&d_int,4,1,fd) != 1)
            {
                cst_errmsg("cst_wave_load_riff_fd: Unable to read header.\n");
                return BELL_IO_ERROR;
            }
#ifdef WORDS_BIGENDIAN
	    d_int = SWAPINT(d_int);
#endif
	    if(bell_fseeko(fd,bell_ftello(fd)+d_int,SEEK_SET) != 0)
            {
                cst_errmsg("cst_wave_load_riff_fd: Unable to seek in header.\n");
                return BELL_IO_ERROR;
            }
	}
	else 
	{
	    cst_errmsg("cst_wave_load_riff: unsupported chunk type \"%*s\"\n",
		       4,info);
	    return BELL_IO_ERROR;
	}
    }

    /* Now read the data itself */
    cst_wave_set_sample_rate(w,hdr.sample_rate);     /* sample rate */
    data_length = samples;
    cst_wave_resize(w,samples/hdr.num_channels,hdr.num_channels);

    if ((d = bell_fread(w->samples,sizeof(short),data_length,fd)) != data_length)
    {
	cst_errmsg("cst_wave_load_riff: %d missing samples, resized accordingly\n",
		   data_length-d);
	w->num_samples = d;
    }

#ifdef WORDS_BIGENDIAN
    swap_bytes_short(w->samples,w->num_samples);
#endif

    return BELL_IO_SUCCESS;
}
