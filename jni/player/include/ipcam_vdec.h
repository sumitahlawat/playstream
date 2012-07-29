#ifndef _IPCAM_VDEC_H_
#define _IPCAM_VDEC_H_

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/mathematics.h"
#include "libswscale/swscale.h"
};

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
	DisplayCallback pCallback;
	int videoWindow;
	unsigned int framesize;

	ipcam_vdec (int camID, int width, int height);
	~ipcam_vdec ();

	static ipcam_vdec* m_Decoder1;
	static ipcam_vdec* m_Decoder2;
	static ipcam_vdec* m_Decoder3;
	static ipcam_vdec* m_Decoder4;

public:
	AVCodec        *pCodec;
	AVCodecContext *pContext;
	AVFrame        *pFrame ,*out_pic;;
	unsigned char  *picBuffer;
	int 			picSize;
	struct SwsContext* img_convert_ctx;

	static ipcam_vdec* getInstance(int camID);

	int InitMPEG4Dec ();
	int DecVideo (unsigned char* pBuffer, unsigned int bufferSize);
};

#endif // _IPCAM_VDEC_H_
