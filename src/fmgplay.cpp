#include <os2.h>

#include "decoder.h"
#include <algorithm>

const int skipmseconds=1500;
const int playmseconds=400;


int Decoder::decoder_open(const char *filename) {

        if ((res=av_open_input_file(&pFormatCtx, filename, NULL, 0, NULL))!=0) {
            ddebug("can't open file %s, res=%d\n",filename,res);
            return -1;
            }
        // Retrieve stream information
        if ((res=av_find_stream_info(pFormatCtx))<0) {
            // Couldn't find stream information
            ddebug("can't find stream information in %s, res=%d\n",filename,res);
            return -2;
            }

        // Find the first video stream
        selectedstream=-1;
        for(unsigned int i=0; i<pFormatCtx->nb_streams; i++)
            if (pFormatCtx->streams[i]->codec->codec_type==CODEC_TYPE_AUDIO)
            {
                selectedstream=i;
                break;
            }
        if(selectedstream==-1) {
        	ddebug("Didn't find an audio stream\n");
            return -3; //Didn't find an audio stream
        	}

        ddebug("Audiostream#%d selected...\n",selectedstream);

        // Get a pointer to the codec context for the audio stream
        pCodecCtx=pFormatCtx->streams[selectedstream]->codec;

        // Find the decoder for the audio stream
        pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
        if(pCodec==NULL)  {
            ddebug("Codec not found (%d)\n",pCodecCtx->codec_id);
            return -4; // Codec not found
            }
        ddebug("Audiocodec '%s' selected...\n",pCodec->name);

        // Inform the codec that we can handle truncated bitstreams -- i.e.,
        // bitstreams where frame boundaries can fall in the middle of packets
        if (pCodec->capabilities & CODEC_CAP_TRUNCATED)
            pCodecCtx->flags|=CODEC_FLAG_TRUNCATED;

        // Open codec
        if  ((res=avcodec_open(pCodecCtx, pCodec))<0) {
            ddebug("Could not open codec, res=%d\n",res);
            return -4; // Could not open codec
        	}

		//filenamei=strnewdup(filename);
		return 0;
        }

int Decoder::decoder_close() {
        if (buffer) {
            delete buffer;
            buffer = NULL;
            }
//        if (filenamei) {
//        	delete filenamei;
//        	filenamei = NULL;
//        	}
        #if (DEBUG>2)
        if (fpraw) {
            fclose(fpraw);
            fpraw=NULL;
            }
        #endif
        if (pCodecCtx) {
        	avcodec_close(pCodecCtx);
        	pCodecCtx=NULL;
        	}
        if (pFormatCtx) {
        	av_close_input_file(pFormatCtx);
        	pFormatCtx=NULL;
        	}
        return 0;
        }


int Decoder::decoder_init() {
        dddebug("decoder_init: ...\n");

        format.format=WAVE_FORMAT_PCM;
        sample_rate					  = pCodecCtx->sample_rate;
        sample_rate					  = (sample_rate>0)?sample_rate:44100;
        format.samplerate 			  = sample_rate;
        channels=format.channels      = pCodecCtx->channels;
        bps=format.bits               = pCodecCtx->bits_per_sample>0?pCodecCtx->bits_per_sample:16;
        time_base					  = av_q2d(pFormatCtx->streams[selectedstream]->time_base);
        duration					  = pFormatCtx->duration;
		ms_duration					  = ULONG(double(duration)*1000/AV_TIME_BASE);
        total_samples                 = duration * sample_rate / AV_TIME_BASE;

        dddebug("detected stream: %d channels, %d bits, %d samples per second, total %lu samples, timebase: %g\n",
                channels,bps,sample_rate,total_samples,time_base
                );

        blockalign=channels*((bps-1)/8+1);


        if (buffer) delete buffer;

		bufsize=std::max(AVCODEC_MAX_AUDIO_FRAME_SIZE,
				 std::max(FF_MIN_BUFFER_SIZE,
				  	 int(channels*pCodecCtx->frame_size*sizeof(int16_t)))
				)+audio_buffersize;

        buffer = new char[bufsize];

        bufused=0;
        filepos=0;

        #if (DEBUG>2)
        if (fpraw)
            fclose(fpraw);
        fpraw=fopen(PRODNAME "play.raw","wb");
        #endif

        blockalign=blockalign?blockalign:4;
        sample_rate=sample_rate?sample_rate:44100;

        samples2play=int64_t(double(playmseconds)/1000*sample_rate);
        samples2skip=int64_t(double(skipmseconds)/1000*sample_rate);

        filesize=0;
        saved_vbr=0;
        update_vbr((pCodecCtx->bit_rate)/1000);

        jumpto = -1;
        samplesplayed=0;
        bytesplayed=0;
        lastplayed=0;
		jumptarget=0;

        return 0;
        }

int Decoder::proceed() {

    int res;

    // read one frame
	do {
		if ((res=av_read_frame(pFormatCtx,&pkt))<0) {
			ddebug("av_read_frame res=%d\n",res);
	   	 	eof=1;
			return 1;
			}
	}
	while ((pkt.stream_index!=selectedstream)&&(av_free_packet(&pkt),1));
    if (pkt.dts) filepos = ULONG(pkt.dts*time_base*sample_rate);


    dddebug("proceed: packet read: size=%d, res=%d\n",pkt.size,res);

	// prepare pkt free at exit
	class freeer {
		AVPacket *pkt;
		public:
			freeer(AVPacket *pkt_to_free) {
				pkt=pkt_to_free;
				}
            ~freeer() {
			    av_free_packet(pkt);
				}
	}
	free_at_return(&pkt);

    // decode in loop
    int lbufrest=bufsize-bufused;

/*/  // is it really need?

    static char* rebuf=NULL;
    static int   rebufsize=0;
    if (rebufsize<pkt.size+FF_INPUT_BUFFER_PADDING_SIZE) {
    	delete rebuf;
    	rebuf=new char[rebufsize=pkt.size+FF_INPUT_BUFFER_PADDING_SIZE];
    	}
    memcpy(rebuf,pkt.data,pkt.size);
    memset(rebuf+pkt.size,0,FF_INPUT_BUFFER_PADDING_SIZE);
/*/
	uint8_t * &rebuf=pkt.data;
/**/
    int pktused=0;
    static int errcount=0;

    for (;
    	 	pktused<pkt.size
    	 	&& (res= avcodec_decode_audio2(
    			pCodecCtx,
    			(int16_t*)(buffer+bufused),
    			&lbufrest,
    			/*pkt.data*/(unsigned char*)rebuf+pktused,pkt.size-pktused))>0;
    		pktused+=res
    		)
    	{
        dddebug("proceed: decoded %d bytes (used %d bytes of unencoded data)\n",lbufrest,res);

    	// skip samples as need
		if (jumptarget>filepos) {
           ddebug("proceed: skip_samples=%lu...\n",(jumptarget-filepos));
           unsigned long add=(unsigned long)((jumptarget-filepos)<(lbufrest/blockalign)?(jumptarget-filepos):(lbufrest/blockalign));
           filepos+=add;
           lbufrest-=add*blockalign;
           if (lbufrest)
           	memmove(buffer+bufused, buffer+bufused+add*blockalign, lbufrest);
           }
        if (needReport && jumptarget<=filepos) {
        	needReport=0;
            reportSeek();
            }

        bufused+=lbufrest;
        samplesplayed += lbufrest/blockalign;
        bytesplayed+=res;

    	int bufproc=0;
    	for (; bufused-bufproc>=audio_buffersize; bufproc+=audio_buffersize ) {

    		  // write audio to audio device

              filepos += audio_buffersize/blockalign;
              ddebug("proceed: filepos=%ld\n",filepos);

              int written = output_play_samples(a, &format, buffer+bufproc, audio_buffersize, decoder_filepos());
              lastplayed += written;
   #if (DEBUG>2)
              if (fpraw) fwrite(buffer+bufproc,audio_buffersize,1,fpraw);
   #endif
              ddebug("proceed: %d bytes (written %d)\n",audio_buffersize,written);
              if (written < audio_buffersize) {
                 WinPostMsg(hwnd,WM_PLAYERROR,0,0);
                 ddebug("ERROR: write error\n");
                 return -1;
                 }
           }
        if (bufproc) {
              if ( samplesplayed > sample_rate)
               {
                   update_vbr((unsigned long)(double(bytesplayed)*sample_rate/samplesplayed/1000*8));
	               samplesplayed=0;
        		   bytesplayed=0;
                   }
              if (bufused>bufproc)
              	 memmove(buffer,buffer+bufproc,bufused-bufproc);
              bufused-=bufproc;
              lbufrest=bufsize-bufused;
			}
        }
   // check for error
   if (res<=0) {
      ddebug("proceed: avcodec_decode_audio2 report %d (buffer: %d/%d, packet: %d/%d)...\n",res,bufused,bufsize,pktused,pkt.size);
      if (errcount++>3) {
      	ddebug("proceed: errcount exeed the limit - exiting!\n");
      	return -1;
    	}
	  }
   else
    	errcount=0;

   return 0;
}

