/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp� <rosmo@sektori.com>
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

#define INCL_DOS
#include <os2.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <memory.h>

#include <unistd.h>
#include <glob.h>
#include <sys/time.h>

#include <math.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stdint.h>
;

#include "decoder.h"

Decoder::Decoder() {
        // empty structure
        memset(this,0,sizeof(Decoder));
        DosCreateEventSem(NULL,&play,0,FALSE);
        DosCreateEventSem(NULL,&ok,0,FALSE);
        format.size=sizeof(FORMAT_INFO);
        format.format=WAVE_FORMAT_PCM;

        register_all_we_need();
//		av_register_all();
        }

Decoder::~Decoder() {
   DosCloseEventSem(play);
   DosCloseEventSem(ok);
   decoder_close();
   }

void Decoder::decoder_thread() {

   ULONG resetcount;
//   while(1)
   rew = ffwd = 0;

   do
   {
      debug("decoder_thread circle started!\n");

      DosWaitEventSem(play, (ULONG)-1);
      DosResetEventSem(play,&resetcount);
      debug("decoder_thread semaphore reset!\n");

      status = DECODER_STARTING;

      last_length = -1;

//      DosResetEventSem(playsem,&resetcount);
      DosPostEventSem(ok);

	  if (file2open) {
	     //if (opened_file()==NULL || strcmp(file2open,opened_file()) )
	     {
	     	if (pFormatCtx) {
            	debug("decoder close (%s)\n",pFormatCtx->filename);
      			decoder_close();
      			}

            if(decoder_open(file2open))
            {
	           debug("open %s failed \n",file2open);
               WinPostMsg(hwnd,WM_PLAYERROR,0,0);
               status = DECODER_STOPPED;
	         //DosPostEventSem(playsem);
               continue;
            }
            debug("decoder opened %s\n",file2open);

            if (decoder_init())
            {
               debug("init %s failed \n",file2open);
               WinPostMsg(hwnd,WM_PLAYERROR,0,0);
               status = DECODER_STOPPED;
	         //DosPostEventSem(playsem);
               continue;
        	}
            debug("decoder inited %s\n",file2open);
         }
         //else
         if (filepos || eof)
             jumpto=0;
         file2open=NULL;
      }


      stop = 0;

      status = DECODER_PLAYING;

      if(jumpto >= 0)
      {
         decoder_jumpto(jumpto,1);
         jumpto = -1;
         }

      while (0==proceed() && !eof && !stop) { // && !stop && get_state()!=FLAC__STREAM_DECODER_END_OF_STREAM ) {
         if(jumpto >= 0)
         {
            decoder_jumpto(jumpto,1);
            DosResetEventSem(play,&resetcount);
            jumpto = -1;
         } else
         if(ffwd && lastplayed>samples2play)
         {
            debug("thread ffwd: %d, %lu/%lu\n",ffwd,decoder_filepos(),decoder_filelength());
            if (filepos+samples2skip>total_samples)
            	break;
            decoder_jumpto_sample(filepos+samples2skip);
         } else
         if(rew && lastplayed>samples2play)
         {
            debug("thread rew: %d, %lu/%lu\n",rew,decoder_filepos(),decoder_filelength());
            if (samples2skip+samples2play>filepos)
                break;
            decoder_jumpto_sample(filepos-samples2skip-samples2play);
         }
      }

	  if (bufused) {
         int written = output_play_samples(a, &format, buffer, bufused, 0);
         #if (DEBUG>2)
         fwrite(buffer,1,bufused,fpraw);
         #endif
         debug("decoder_thread write: %d bytes (written %d) / %d blocks (%d buf / %d audiobuf)\n",bufused,written,0,0,audio_buffersize);
         if (written < bufused) {
            WinPostMsg(hwnd,WM_PLAYERROR,0,0);
            debug("ERROR: write error\n");
            }
    	 }

	  debug("decoder_thread circle end!\n");
      status = DECODER_STOPPED;

//      DosPostEventSem(playsem);
      WinPostMsg(hwnd,WM_PLAYSTOP,0,0);

      DosPostEventSem(ok);
      debug("decoder_thread circle end 2!\n");
   }
   while (1);
   decoder_close();
   debug("decoder_thread finish!\n");
}


ULONG Decoder::decoder_length() {
   if(status == DECODER_PLAYING)
      last_length = decoder_filelength();

   if (last_length<0) last_length=0;
   return last_length;
}

ULONG Decoder::decoder_command(ULONG msg, DECODER_PARAMS *params) {
   ULONG resetcount;

   debug("[%s] ",status==DECODER_STOPPED?"STOPPED":
   				 status==DECODER_PLAYING?"PLAYING":
   				 status==DECODER_STOPPED?"STOPPED":
                 status==DECODER_PAUSED ?"PAUSED ":
                 						 " ERROR ");
   switch(msg)
   {

      case DECODER_JUMPTO:
         debug("decoder_command jumpto:%d!\n",params->jumpto);
         jumpto = params->jumpto;
         DosPostEventSem(play);
         break;

      case DECODER_PLAY:
   		 debug("decoder_command play (%s -> %s)!\n",pFormatCtx?pFormatCtx->filename:"none",params->filename);
         if(status == DECODER_STOPPED)
         {
   			jumpto = -1;
   			rew = ffwd = 0;
            file2open=params->filename;
            DosResetEventSem(ok,&resetcount);
            DosPostEventSem(play);
         	debug("decoder_command semaphore posted!\n");
            if (decodertid<0 || DosWaitEventSem(ok, 10000) == 640)
            {
               status = DECODER_STOPPED;
               if (decodertid>0) DosKillThread(decodertid);
               decodertid = _beginthread(::decoder_thread,0,64*1024,(void *) this);
               return 102;
            }
         }
         else
            return 101;
         break;

      case DECODER_STOP:
         debug("decoder_command stop!\n");
         if(status != DECODER_STOPPED)
         {
            DosResetEventSem(ok,&resetcount);
            stop = TRUE;
            jumpto = -1;
            rew = ffwd = 0;
            if(DosWaitEventSem(ok, 10000) == 640)
            {
               status = DECODER_STOPPED;
               if (decodertid>0) DosKillThread(decodertid);
               decodertid = _beginthread(::decoder_thread,0,64*1024,(void *) this);
               return 102;
            }
         }
         else
            return 101;
         break;

      case DECODER_FFWD:
         debug("decoder_command ffwd:%d!\n",params->ffwd);
         ffwd = params->ffwd;
         //if(status == DECODER_STOPPED)
         //   DosPostEventSem(play);
         break;

      case DECODER_REW:
         debug("decoder_command rew:%d!\n",params->rew);
         rew = params->rew;
         //if(status == DECODER_STOPPED)
         //   DosPostEventSem(play);
         break;

      case DECODER_EQ:
         debug("decoder_command eq!\n");
         return 1;

      case DECODER_SETUP:
         debug("decoder_command setup!\n");
         output_play_samples = params->output_play_samples;
         a = params->a;
         audio_buffersize = params->audio_buffersize;
         error_display = params->error_display;
         info_display = params->info_display;
         hwnd = params->hwnd;
//         playsem = params->playsem;
//         DosPostEventSem(playsem);
         break;
      default:
         debug("decoder_command unknown!\n");
   }
   return PLUGIN_OK;
}


