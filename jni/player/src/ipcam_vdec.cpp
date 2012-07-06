/*
 * ipcam_vdec.cpp
 * This file is part of RTSPlayer
 *
 * Copyright (C) 2011 - Shrirang Bagul
 *
 * RTSPlayer is proprietary software; you cannot redistribute it and/or modify
 * it.
 *
 * RTSPlayer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "ipcam_vdec.h"

#include <pthread.h>

#ifdef PGM_SAVE
#undef PGM_SAVE
#endif

extern void DisplayCb_1 (uint8_t* aData[], int aDataLen);

ipcam_vdec :: ipcam_vdec(int camID, int width, int height)
{
	fprintf (stderr, "Construct video decoder, camID = %d\n", camID);

	// init pointers
	pCodec = NULL;
	pContext = NULL;
	pFrame = NULL;
	picBuffer = NULL;
	switch (camID)
	{
		case 1:
			pCallback = &DisplayCb_1;
			break;
		default:
			pCallback = &DisplayCb_1;
			break;
	}

	mpgParm.bitRate = 1000;
	mpgParm.frameRate = 15;
	mpgParm.width = width;
	mpgParm.height = height;
	mpgParm.streamDelay = 0;
	isAvCodecInitEn = FALSE;

	videoWindow = camID;
}

ipcam_vdec::~ipcam_vdec()
{
    /**
        Release the allocated resources
    */
    fprintf (stderr, "Destroy video decoder\n");
    /** Set callback function to NULL
    */
    pCallback = NULL;

    /** Close software decoder, free all the allocated memory
    */
    if (pContext != NULL)
    {
        avcodec_close (pContext);
        av_free (pContext);
    }

    free(picBuffer);
}

int ipcam_vdec::InitMPEG4Dec()
{

	fprintf (stderr, "Enter InitMPEGDec() %d\n", videoWindow);

	if (isAvCodecInitEn == false)
    {
        avcodec_init();
        avcodec_register_all ();
		isAvCodecInitEn = true;
    }

    pCodec = avcodec_find_decoder (CODEC_ID_MPEG4);
    if (!pCodec)
    {
        fprintf (stderr, "Could not find codec to decode MPEG2 video\n");
        return -1;
    }

    pFrame = avcodec_alloc_frame ();

    pContext = avcodec_alloc_context ();

    pContext->bit_rate = mpgParm.bitRate;
    /* resolution must be a multiple of two */
    pContext->width =  mpgParm.width;
    pContext->height = mpgParm.height;
	/* frames per second */
    pContext->time_base= (AVRational){1,25};
    //pContext->gop_size = 10; /* emit one intra frame every ten frames */
    //pContext->max_b_frames=1;
    pContext->pix_fmt = PIX_FMT_YUV420P;

	//calculate picture size and allocate memory
	picSize = avpicture_get_size(pContext->pix_fmt, mpgParm.width, mpgParm.height);
	fprintf(stderr, "Width %d, Height %d picSize %d\n", mpgParm.width, mpgParm.height, picSize );

	picBuffer = new uint8_t[picSize];

     if (pCodec->capabilities & CODEC_CAP_TRUNCATED)
    {
        fprintf (stderr, "We do not send one total frame at one time\n");
        pContext->flags |= CODEC_FLAG_TRUNCATED;
		pContext->flags |= CODEC_FLAG_EMU_EDGE;
    }

	if (avcodec_open (pContext, pCodec) < 0)
    {
        fprintf (stderr, "Could not open video codec\n");
		isAvCodecInitEn = FALSE;
        return -1;
    }

	fprintf(stderr, "InitMPEG4Dec %d complete\n", videoWindow);

    return 0;
}

char filename[] = "/mnt/sdcard/image.pgm";

static void pgm_save(unsigned char *buf, int wrap, int xsize, int ysize, char *filename)
{
	FILE *f;
	int i;
	int offset = 0;

	f=fopen(filename,"w");
	fprintf(f,"P5\n%d %d\n%d\n",xsize,ysize,255);
	for(i=0;i<ysize;i++) {
		offset = i*wrap;
		fwrite(buf + offset,1,xsize,f);
	}
	fclose(f);
}

int ipcam_vdec::DecVideo(unsigned char* inBuffer, unsigned int bufferSize, void* pPriv)
{
    int frame, gotPicture, len;
#ifdef PGM_SAVE
	char buf[1024];
#endif
	pthread_mutex_t display;

	//fprintf (stderr, "ipcam_vdec::DecVideo, size = %d\n", bufferSize);

    AVPacket avpkt;
    av_init_packet(&avpkt);

    avpkt.size = bufferSize;
    avpkt.data = inBuffer;

    frame = 0;
    while(avpkt.size > 0)
    {
    	len = avcodec_decode_video2(pContext, pFrame, &gotPicture, &avpkt);
    	if (len < 0) {
                fprintf(stderr, "Error while decoding frame %d\n", frame);
                return -1;
        }
		if (gotPicture) {
#ifdef PGM_SAVE
				snprintf(buf, sizeof(buf), filename, frame);
				pgm_save(pFrame->data[0], pFrame->linesize[0],
						 pContext->width, pContext->height, buf);
#endif
				pthread_mutex_lock(&display);
				pCallback(pFrame->data, (mpgParm.width*mpgParm.height*3)/2);
				pthread_mutex_unlock(&display);

		}
		avpkt.size -= len;
		avpkt.data += len;
		frame++;
    }

    return 0;
}
