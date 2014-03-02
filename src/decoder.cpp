/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 *    3. The name of the author may not be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* Shorten player plug-in for PM123 */

#define INCL_DOS
#define INCL_PM

#include <os2.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <unistd.h>
#include <glob.h>
#include <sys/time.h>

#include <math.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <io.h>
#include <locale.h>

#include "decoder.h"

void decoder_thread(void *arg) {
   ((Decoder *) arg)->decoder_thread();
}

int _System decoder_init(void **W) {
   debug("init called!\n");
   Decoder *w=new Decoder;
   w->decodertid = _beginthread(decoder_thread,0,64*1024,(void *) w);

   if(w->decodertid < 1) {
      delete w;
      return -1;
   	  }

   *W=w;
   return w->decodertid;
}

BOOL _System decoder_uninit(void *W) {
   debug("uninit called!\n");
   Decoder *w = (Decoder *) W;
   int decodertid = w->decodertid;
   BOOL rc=!DosKillThread(decodertid);
   delete w;
   return rc;
}

ULONG _System decoder_command(void *W, ULONG msg, DECODER_PARAMS *params) {
   debug("command %lu called!\n",msg);
   return ((Decoder *) W)->decoder_command(msg,params);
   }

ULONG _System decoder_length(void *W)
{
   unsigned long r=((Decoder *)W)->decoder_length();
   debug("decoder_length called = %lu!\n",r);
   return r;
}

ULONG _System decoder_status(void *W)
{
//   debug("decoder_status called (status=%d)!\n",((Decoder *)W)->decoder_status());
   return ((Decoder *)W)->decoder_status();
}

ULONG _System decoder_trackinfo(char *drive, int track, DECODER_INFO *info)
{
   return 200;
}

ULONG _System decoder_cdinfo(char *drive, DECODER_CDINFO *info)
{
   return 100;
}


ULONG _System decoder_support(char *ext[], int *size)
{
   debug("decoder_support %d called!\n",*size);
   if(size)
   {
/*      if(ext != NULL)
      {
         if (*size >= 1) strcpy(ext[0],"*.m4a");
      }
*/
      *size = 0; //1;
   }
   return DECODER_FILENAME;
}

void _System plugin_query(PLUGIN_QUERYPARAM *param)
{
   debug("plugin_query called!\n");
   param->type = PLUGIN_DECODER;
   param->author = "ntim";
   param->desc = PRODNAME " " PRODVERSION "-" PRODBUILD;
   param->configurable = FALSE;
}


ULONG _System decoder_fileinfo(char *filename, DECODER_INFO *info)
{
   debug("decoder_fileinfo %s called!\n",filename);

   int ssize=info->size;
   memset(info,0,ssize);
   info->size = ssize;

   int vers_1_32= info->size >= INFO_SIZE_2; // detect pm123 version

   if (vers_1_32)
      info->haveinfo = 0;
   debug("pm123 version %s 1.32 detected!\n",vers_1_32?">=":"<");

    Decoder w;
    //char sbuf[32];

    int r=w.decoder_open(filename);
    r=r?r:w.decoder_init();

    if (r==0) {
    	{
      	struct info_item {
        	char* name;
        	void* field;
        	int isstr;
        	int flag;
        	int size;
        //char* name;
         } cp_infos[]={
            {info->title    ,  w.pFormatCtx->title    , 1, DECODER_HAVE_TITLE     ,128},
            {info->artist   ,  w.pFormatCtx->author   , 1, DECODER_HAVE_ARTIST    ,128},
            {info->album    ,  w.pFormatCtx->album    , 1, DECODER_HAVE_ALBUM     ,128},
            {info->year     , &w.pFormatCtx->year     , 0, DECODER_HAVE_YEAR      ,128},
            {info->comment  ,  w.pFormatCtx->comment  , 1, DECODER_HAVE_COMMENT   ,128},
            {info->genre    ,  w.pFormatCtx->genre    , 1, DECODER_HAVE_GENRE     ,128},
            {info->track    , &w.pFormatCtx->track    , 0, DECODER_HAVE_TRACK     ,128},
            {info->copyright,  w.pFormatCtx->copyright, 1, DECODER_HAVE_COPYRIGHT ,128}
            };

        unsigned int infofields=vers_1_32?sizeof(cp_infos)/sizeof(cp_infos[0]):6;

    	for (unsigned int i=0; i<infofields; i++) {
            if (cp_infos[i].isstr) {
    		   if (((char*)cp_infos[i].field)[0]!='\0') {
    		   	strncpy(cp_infos[i].name,((char*)cp_infos[i].field),cp_infos[i].size);
    		   	cp_infos[i].name[cp_infos[i].size-1]='\0';
                if (vers_1_32)
                   	info->haveinfo |= cp_infos[i].flag;
               	}
            } else {
               if (*(int*)(cp_infos[i].field)) {
                snprintf(cp_infos[i].name,cp_infos[i].size,"%d",*(int*)cp_infos[i].field);
                cp_infos[i].name[cp_infos[i].size-1]='\0';
                if (vers_1_32)
                    info->haveinfo |= cp_infos[i].flag;
                }
            }
        }
    	}

/* don't need - no replaygain!

        if (vers_1_32) {
           struct info_float_item {
               float* name;
               char* field;
               int flag;
            	} ft_infos[]={
               {&info->track_gain,  w.REPLAYGAIN_TRACK_GAIN, DECODER_HAVE_TRACK_GAIN},
               {&info->track_peak,  w.REPLAYGAIN_TRACK_PEAK, DECODER_HAVE_TRACK_PEAK},
               {&info->album_gain,  w.REPLAYGAIN_ALBUM_GAIN, DECODER_HAVE_ALBUM_GAIN},
               {&info->album_peak,  w.REPLAYGAIN_ALBUM_PEAK, DECODER_HAVE_ALBUM_PEAK}
               };
            char* save_locale=strnewdup(setlocale(LC_NUMERIC,NULL));
            if (save_locale)
            	setlocale(LC_NUMERIC,"C");
            else
            	debug("save locale failed!\n");
        	for (unsigned int i=0; i<sizeof(ft_infos)/sizeof(ft_infos[0]); i++) {
            	if (ft_infos[i].field) {
                	*ft_infos[i].name=atof(ft_infos[i].field);
                	debug("replay/gain specified: %g (%s)\n",*ft_infos[i].name,ft_infos[i].field);
                    info->haveinfo |= ft_infos[i].flag;
                	}
            	}
    		if (save_locale) {
                setlocale(LC_NUMERIC,save_locale);
                delete save_locale;
            	}
            }
*/

        info->songlength=w.decoder_filelength();
	    debug("reported filelength = %u! (%lu) \n",info->songlength,w.decoder_filelength());
        info->format.size=sizeof(FORMAT_INFO);
        info->format.bits=w.format.bits;
        info->format.channels=w.format.channels;
        info->format.samplerate=w.format.samplerate;
        info->format.format=w.format.format;

/*
        snprintf( info->tech_info, sizeof(info->tech_info), PRODNAME " %d bits, %.1f kHz, %s",
            info->format.bits,(float) info->format.samplerate / 1000.0f,
            info->format.channels == 1 ? "Mono" : info->format.channels == 2 ? "Stereo" : _ltoa(info->format.channels,sbuf,10) );
*/
		avcodec_string(info->tech_info, sizeof(info->tech_info), w.pCodecCtx, 0 );
    }
    else {
    	debug("open %s failed: %d\n",filename,r);
        }

    return r;
}


