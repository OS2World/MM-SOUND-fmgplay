#ifndef __Decoder_h_
#define __Decoder_h_

#include <string.h>
#include <io.h>
#include "pm123/format.h"
#include "pm123/decoder_plug.h"
#include "pm123/plugin.h"
extern "C" {
#include "ffmpeg/avcodec.h"
#include "ffmpeg/avformat.h"
};

//#define NO_WAV_FILE 200

/* definitions for id3tag.cpp */
#ifndef FILE_T
#define FILE_T FILE*
#endif
#ifndef OFF_T
#define OFF_T signed long
#endif
typedef signed int Int;


#ifndef STRNEWDUP_DEFINED
inline char* strnewdup(const char* str) {
    return str?strcpy(new char[strlen(str)+1],str):NULL;
    }
#define STRNEWDUP_DEFINED
#endif

void decoder_thread(void *arg);


#ifndef DEBUG_DEFINED
#define DEBUG_DEFINED
#if defined(DEBUG)
int debug(const char* str, ...) __attribute__ ((format (printf, 1, 2)));
#if (DEBUG>1)
#define  ddebug debug
#if (DEBUG>2)
#define dddebug debug
#else
inline int dddebug(const char* str, ...) { return -1; }
#endif
#else
inline int  ddebug(const char* str, ...) { return -1; }
#endif
#else
inline int dddebug(const char* str, ...) { return -1; }
inline int  ddebug(const char* str, ...) { return -1; }
inline int   debug(const char* str, ...) { return -1; }
#endif
#endif

/*
Int Read_APE_Tags ( FILE_T fp, TagInfo_t* tip );
Int Read_ID3V1_Tags ( FILE_T fp, TagInfo_t* tip );
*/

extern "C" int register_all_we_need();

class Decoder: public DECODER_PARAMS {
protected:

	AVFormatContext *pFormatCtx;
	AVCodecContext  *pCodecCtx;
	AVCodec         *pCodec;
	int				 selectedstream;
	AVPacket		 pkt;

	FORMAT_INFO format;
    HEV play,ok;
    int decodertid;
    int status;
    int stop;
	unsigned long filepos; // in samples
	unsigned long jumptarget; // in samples

	char* buffer;
	int bufused;
	int bufsize;

    const char* file2open;
//*** use pFormatCtx->filename instead!
//    char* filenamei;

	unsigned sample_rate;
	unsigned channels;
	unsigned bps;
	long last_length;
	int blockalign;

						// parameters for forward/rewind:
    int64_t		  samples2play;	// amount to play (better is factor of audio_buffersize)
	int64_t       samples2skip;	// amount to skip

   	unsigned long lastplayed;   // samples played (ajter last jump/skip)

	unsigned long saved_vbr; 	// saved vbr level
								// for vbr calculation
    unsigned long samplesplayed;
    unsigned long bytesplayed;

//(debug)
    #if (DEBUG>2)
	FILE* fpraw;
    #endif

// ******** no separate (from ffmpeg) tags support this time !
//
//     /* general technical information string */
//     char  *tech_info;
//     /* meta information */
//     char  *title  ;
//     char  *artist ;
//     char  *album  ;
//     char  *year   ;
//     char  *comment;
//     char  *genre  ;
//
//     /* added since PM123 1.32 */
//     char  *track    ;
//     char  *copyright;
//
//     char  *REPLAYGAIN_REFERENCE_LOUDNESS;
//     char  *REPLAYGAIN_TRACK_GAIN;
//     char  *REPLAYGAIN_TRACK_PEAK;
//     char  *REPLAYGAIN_ALBUM_GAIN;
//     char  *REPLAYGAIN_ALBUM_PEAK;
//

/*   HEADER_UNION riff;
   unsigned int riff_set;
*/

   int needReport;
   int64_t duration;   		// in AV_TIME_BASE fractions!
   ULONG   ms_duration;  	// in miliseconnds
   int64_t total_samples;	// in samples
   double time_base;
   unsigned long filesize;
public:
    Decoder();
    virtual ~Decoder();

	void decoder_thread();
	ULONG decoder_length();
	ULONG decoder_command(ULONG msg, DECODER_PARAMS *params);
    int decoder_status() { return status; }

	int res;
	int eof;

protected:
      int decoder_open(const char *filename);
      int decoder_init();
      int decoder_close();

	  unsigned long decoder_filepos() {
	    	return (unsigned long)(double(filepos)*1000/sample_rate);
	    }
	  int flush() {
	    //filepos+=bufused/blockalign;
	    return ((bufused=0));
		}
      int reportSeek() {
        WinPostMsg(hwnd,WM_SEEKSTOP,0,0);
        return 0;
        }

      int decoder_jumpto_frame(int64_t frame, int report=0) {
            dddebug("decoder_jumpto_frame: %Ld\n",frame);
            int res=pFormatCtx?av_seek_frame(pFormatCtx,selectedstream,frame,0):0;
            if (res<0) {
                debug("decoder_jumpto_frame: av_seek_frame report error %d at %Ld\n",res,frame);
                return -1;
            	}
            else {
                  jumptarget=ULONG(frame*time_base*sample_rate);
                  eof=0;
                  lastplayed=0;
                  needReport=report;
            	  }
            }
      int decoder_jumpto_sample(ULONG sample, int report=0) {
            dddebug("decoder_jumpto_sample: %ld\n",sample);
            return decoder_jumpto_frame(int64_t(double(sample)/sample_rate/time_base),report);
        	}
      int decoder_jumpto(long offset, int report=0) {
            dddebug("decoder_jumpto: %ld\n",offset);
            offset=offset<0?0:offset;
            return decoder_jumpto_frame(int64_t(double(offset)/1000/time_base),report);
            }

      ULONG decoder_filelength() {
      		return ms_duration?ms_duration:(ULONG)-1;
      		}

	  int proceed();

      unsigned long decoder_vbr() {
		return pFormatCtx?pFormatCtx->bit_rate:0;
        }
      void update_vbr(unsigned long vbr) {
         dddebug("update_vbr: %ld\n",vbr);
         if (vbr!=saved_vbr) {
            WinPostMsg(hwnd,WM_CHANGEBR,MPFROMLONG(saved_vbr=vbr),0);
            }
    	 }

friend ULONG _System decoder_fileinfo(char *filename, DECODER_INFO *info);
friend int _System decoder_init(void **W);
friend BOOL _System decoder_uninit(void *W);

};


#endif // defined __Decoder_h_


