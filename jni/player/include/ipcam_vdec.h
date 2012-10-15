#ifndef _IPCAM_VDEC_H_
#define _IPCAM_VDEC_H_

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/mathematics.h"
#include "libswscale/swscale.h"
};

typedef void (*DisplayCallback) (AVFrame *Frame);

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

	DisplayCallback pCallback;
	int videoWindow;
	unsigned int framesize;

	ipcam_vdec (int camID);
	~ipcam_vdec ();

	static ipcam_vdec* m_Decoder1;
	static ipcam_vdec* m_Decoder2;
	static ipcam_vdec* m_Decoder3;
	static ipcam_vdec* m_Decoder4;

public:
	AVCodec        *pCodec;
	AVCodecContext *pContext;
	AVCodecParserContext *h264_parser;

	AVFrame        *pFrame ,*out_pic;;
	unsigned char  *picBuffer;
	int 			picSize;
	struct SwsContext* img_convert_ctx;

	MPEG_DECODING_PRAMS_t   mpgParm;

	static ipcam_vdec* getInstance(int camID);

	void setparam(int vwidth, int vheight);
	int InitMPEG4Dec ();
	int InitH264Dec();
	int DecVideo (unsigned char* pBuffer, unsigned int bufferSize);
	int DecVideoH264 (unsigned char* pBuffer, unsigned int bufferSize);
};

#endif // _IPCAM_VDEC_H_
