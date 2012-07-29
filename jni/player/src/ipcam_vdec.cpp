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
#include "player.h"

extern void DisplayCb_1 (uint8_t* aData[], int aDataLen);
extern void DisplayCb_2 (uint8_t* aData[], int aDataLen);
extern void DisplayCb_3 (uint8_t* aData[], int aDataLen);
extern void DisplayCb_4 (uint8_t* aData[], int aDataLen);

ipcam_vdec :: ipcam_vdec(int camID, int width, int height)
{
	LOGI("Construct video decoder, camID = %d\n", camID);

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
	case 2:
		pCallback = &DisplayCb_2;
		break;
	case 3:
		pCallback = &DisplayCb_3;
		break;
	case 4:
		pCallback = &DisplayCb_4;
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

	videoWindow = camID;
}

ipcam_vdec::~ipcam_vdec()
{
	LOGI("Destroy video decoder\n");

	pCallback = NULL; ///** Set callback function to NULL

	if (pContext != NULL) // Close software decoder, free all the allocated memory
	{
		avcodec_close (pContext);
		av_free (pContext);
	}
	free(picBuffer);
}

int ipcam_vdec::InitMPEG4Dec()
{
	LOGI( "Enter InitMPEGDec() %d\n", videoWindow);
	avcodec_init();
	avcodec_register_all();
	pCodec = avcodec_find_decoder (CODEC_ID_MPEG4);
	if (!pCodec) {
		LOGI("Could not find codec to decode MPEG2 video\n");
	}

	pFrame = avcodec_alloc_frame();
	pContext = avcodec_alloc_context();
	pContext->bit_rate = 1000;  //temp value
	/* resolution must be a multiple of two */
	pContext->width =  320;   //temp value width
	pContext->height = 240;    //temp value height
	/* frames per second */
	pContext->time_base= (AVRational){1,25};
	pContext->pix_fmt = PIX_FMT_YUV420P;  //old
	pContext->pix_fmt = PIX_FMT_RGB24;    //for storing ppm's
	pContext->pix_fmt =PIX_FMT_RGB565;   //for glsurface

	//calculate picture size and allocate memory
	picSize = avpicture_get_size(pContext->pix_fmt, pContext->width, pContext->height);
	LOGI("Width %d, Height %d picSize %d\n", pContext->width, pContext->height, picSize);

	picBuffer = new uint8_t[picSize];

	if (pCodec->capabilities & CODEC_CAP_TRUNCATED)	{
		//We do not send one total frame at one time
		pContext->flags |= CODEC_FLAG_TRUNCATED;
		pContext->flags |= CODEC_FLAG_EMU_EDGE;
	}

	if (avcodec_open (pContext, pCodec) < 0) {
		LOGI( "Could not open video codec\n");
	}
	LOGI("InitMPEG4Dec complete\n");
	return 0;
}

int savep = 0;
//works only for RGB24 data
void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
	FILE *pFile;
	char szFilename[32];
	int  y;

	// Open file
	sprintf(szFilename, "/mnt/sdcard/ipcam1/frame%d.ppm", iFrame);
	pFile=fopen(szFilename, "wb");
	if(pFile==NULL)
		return;

	// Write header
	fprintf(pFile, "P6\n%d %d\n255\n", width, height);

	// Write pixel data
	for(y=0; y<height; y++)
		fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);

	fclose(pFile);
}

int ipcam_vdec::DecVideo(unsigned char* inBuffer, unsigned int bufferSize, void* pPriv)
{

	int frame, gotPicture, len;
	//	char buf[1024];
	uint8_t *buf;
	int numBytes=avpicture_get_size(PIX_FMT_RGB565, pContext->width, pContext->height);
	buf=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
	LOGI("%s : %d : bufferSize :%d\n",__func__,__LINE__,bufferSize);
	AVPacket avpkt;
	av_init_packet(&avpkt);

	avpkt.size = bufferSize;
	avpkt.data = inBuffer;

	frame = 0;
	while(avpkt.size > 0)
	{
		len = avcodec_decode_video2(pContext, pFrame, &gotPicture, &avpkt);
		if (len < 0) {
			LOGI("Error while decoding frame %d\n", frame);
			return -1;
		}
		if (gotPicture) {
			out_pic = avcodec_alloc_frame();
			if (!out_pic)
				return -1;
			img_convert_ctx = sws_getContext(pContext->width, pContext->height, pContext->pix_fmt,
					pContext->width, pContext->height, PIX_FMT_RGB565,SWS_BICUBIC, NULL, NULL, NULL);
			if (!img_convert_ctx)
				return -1;

			avpicture_fill((AVPicture *)out_pic, buf, PIX_FMT_RGB565, pContext->width, pContext->height);
			sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize, 0, pContext->height, out_pic->data, out_pic->linesize);
			sws_freeContext(img_convert_ctx);
			img_convert_ctx = NULL;

			savep++;
			//			if((savep%15)==0)
			//				SaveFrame(out_pic, pContext->width,	pContext->height, savep);

			//find a way to display now
		}
		avpkt.size -= len;
		avpkt.data += len;
		frame++;
	}
	return 0;
}
