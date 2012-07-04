
#ifndef _IPCAM_VDEC_H_
#define _IPCAM_VDEC_H_

extern "C" {
#include <libavcodec/avcodec.h>
};

#ifndef BOOL
typedef bool BOOL;
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif


typedef void (*DisplayCallback) (uint8_t* aData[], int aDataLen);

typedef struct {
    unsigned int   bitRate;
    unsigned int   frameRate;
    unsigned int   width;
    unsigned int   height;
    unsigned int   streamDelay;
    DisplayCallback           pCallback;
} MPEG_DECODING_PRAMS_t;

class ipcam_vdec
{
	private:

        MPEG_DECODING_PRAMS_t   mpgParm;
        BOOL    isAvCodecInitEn;
        BOOL    isAvCodecInitDe;
		DisplayCallback pCallback;
		int videoWindow;

		unsigned int framesize;

    public:

        AVCodec        *pCodec;
        AVCodecContext *pContext;
        AVFrame        *pFrame;
        unsigned char  *picBuffer;
		int 			picSize;

        ipcam_vdec (int camID, int width, int height);
        ~ipcam_vdec ();

        int InitMPEG4Dec ();
        int DecVideo (unsigned char* pBuffer, unsigned int bufferSize, void* pPriv);

};

#endif // _IPCAM_VDEC_H_

