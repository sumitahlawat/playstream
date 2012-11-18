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


ipcam_vdec* ipcam_vdec::m_Decoder1=NULL;
ipcam_vdec* ipcam_vdec::m_Decoder2=NULL;
ipcam_vdec* ipcam_vdec::m_Decoder3=NULL;
ipcam_vdec* ipcam_vdec::m_Decoder4=NULL;

extern void DisplayCb_1 (AVFrame *Frame);
extern void DisplayCb_2 (AVFrame *Frame);
extern void DisplayCb_3 (AVFrame *Frame);
extern void DisplayCb_4 (AVFrame *Frame);

ipcam_vdec* ipcam_vdec::getInstance(int id)
{
	switch (id)
	{
	case 1:
		if (!m_Decoder1)
			m_Decoder1 = new ipcam_vdec(1);
		return m_Decoder1;
		break;
	case 2:
		if (!m_Decoder2)
			m_Decoder2 = new ipcam_vdec(2);
		return m_Decoder2;
		break;
	case 3:
		if (!m_Decoder3)
			m_Decoder3 = new ipcam_vdec(3);
		return m_Decoder3;
		break;
	case 4:
		if (!m_Decoder4)
			m_Decoder4 = new ipcam_vdec(4);
		return m_Decoder4;
		break;
	default:
		if (!m_Decoder1)
			m_Decoder1 = new ipcam_vdec(1);
		return m_Decoder1;
		break;
	}
}

ipcam_vdec::ipcam_vdec(int camID)
{
	LOGI("Construct video decoder, camID = %d\n", camID);
	// init pointers
	pCodec = NULL;
	codec = NULL;
	pContext = NULL;
	c = NULL;
	pFrame = NULL;
	picBuffer = NULL;
	picture = NULL;
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

void ipcam_vdec::setparam(int vwidth, int vheight)
{
	mpgParm.bitRate = 1000;
	mpgParm.frameRate = 15;
	mpgParm.width = vwidth;
	mpgParm.height = vheight;
	mpgParm.streamDelay = 0;
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
	pContext->width =  mpgParm.width;   //temp value width
	pContext->height = mpgParm.height;    //temp value height
	/* frames per second */
	pContext->time_base= (AVRational){1,25};
	pContext->pix_fmt = PIX_FMT_YUV420P;  //old
	//	pContext->pix_fmt = PIX_FMT_RGB24;    //for storing ppm's
	//pContext->pix_fmt =PIX_FMT_RGB565;   //for glsurface

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

int ipcam_vdec::InitH264Dec()
{
	LOGI( "Enter InitH264Dec() %d\n", videoWindow);

	/* must be called before using avcodec lib */
	avcodec_init();

	/* register all the codecs */
	avcodec_register_all();

	/* find the h264 video decoder */
	codec = avcodec_find_decoder(CODEC_ID_H264);
	if (!codec) {
		LOGI("Could not find codec to decode H264 video\n");
	}
	/* allocate memory for video decoder */
	c = avcodec_alloc_context();
	if (!c) {
		LOGI("Could not allocate context to decode H264 video\n");
	}

	c->width = mpgParm.width;   //temp value width;
	c->height = mpgParm.height;    //temp value height

	/* allocate frame buffer */
	picture = avcodec_alloc_frame();
	if (!picture) {
		LOGI("Could not allocate frame to decode H264 video\n");
	}

	if(codec->capabilities & CODEC_CAP_DR1)
		c->flags |= CODEC_FLAG_EMU_EDGE;

	// Inform the codec that we can handle truncated bitstreams -- i.e.,
	// bitstreams where frame boundaries can fall in the middle of packets
	if(codec->capabilities & CODEC_CAP_TRUNCATED)
		c->flags|=CODEC_FLAG_TRUNCATED;

	c->flags|=CODEC_CAP_DELAY;

	/* open codec */
	if (avcodec_open(c, codec) < 0)	{
		LOGI("Could not open codec \n");
	}

	h264_parser = av_parser_init(CODEC_ID_H264);
	if (!h264_parser){
		LOGI("Could not init codec \n");
	}

	LOGI("InitH264Dec complete\n");
	return 0;
}

int savep = 0;

//works only for RGB24 data
void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
	FILE *pFile;
	char szFilename[32];
	int  y;

	LOGI("Save Frame %d\n", iFrame);

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

int ipcam_vdec::DecVideoH264(unsigned char* inBuffer, unsigned int bufferSize)
{
	LOGI("%s : %d %u %u %u %u\n",__func__,__LINE__,inBuffer[0],inBuffer[1],inBuffer[2],inBuffer[3]);
	int got_picture = 0, len = 0;
	uint8_t *buf;
	int numBytes=avpicture_get_size(PIX_FMT_RGB24, c->width, c->height);
	buf=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
	AVPacket avpkt;
	av_init_packet(&avpkt);
	avpkt.size = bufferSize;
	avpkt.data = inBuffer;

	unsigned char *inBuf = inBuffer;
	int bufLen = bufferSize;
	while (bufLen > 0){
		len = av_parser_parse2(h264_parser, c, &avpkt.data, &avpkt.size, inBuf, bufLen, AV_NOPTS_VALUE, AV_NOPTS_VALUE, AV_NOPTS_VALUE);
		if (avpkt.size)
			len = avcodec_decode_video2(c, picture, &got_picture, &avpkt);

		LOGI("%s : %d   fsize : %d len:%d got_pic:%d\n",__func__,__LINE__, avpkt.size, len, got_picture);

		if (got_picture)
		{
			LOGI("%s : %d\n",__func__,__LINE__);
			out_pic = avcodec_alloc_frame();
			if (!out_pic)
				return -1;
			img_convert_ctx = sws_getContext(c->width, c->height, PIX_FMT_YUV420P,
					c->width, c->height, PIX_FMT_RGB24,SWS_BICUBIC, NULL, NULL, NULL);
			if (!img_convert_ctx)
				return -1;
			avpicture_fill((AVPicture *)out_pic, buf, PIX_FMT_RGB24, c->width, c->height);
			sws_scale(img_convert_ctx, picture->data, picture->linesize, 0, c->height, out_pic->data, out_pic->linesize);
			sws_freeContext(img_convert_ctx);
			img_convert_ctx = NULL;
			pCallback(out_pic);
			av_free(out_pic);
			av_free(buf);
			out_pic = NULL;
		}
		inBuf += len;
		bufLen -= len;
		av_free_packet(&avpkt);
	}
	return 0;
}

int ipcam_vdec::DecVideo(unsigned char* inBuffer, unsigned int bufferSize)
{
	//LOGI("%s : %d\n",__func__,__LINE__);
	int frame, gotPicture, len;
	//	char buf[1024];
	uint8_t *buf;
	int numBytes=avpicture_get_size(PIX_FMT_RGB24, pContext->width, pContext->height);
	buf=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
	AVPacket avpkt;
	av_init_packet(&avpkt);

	avpkt.size = bufferSize;
	avpkt.data = inBuffer;

	frame = 0;
	while(avpkt.size > 0)
	{
		len = avcodec_decode_video2(pContext, pFrame, &gotPicture, &avpkt);
		//LOGI("%s : %d   fsize : %d len:%d got_pic:%d\n",__func__,__LINE__, avpkt.size, len, gotPicture);
		if (len < 0) {
			LOGI("Error while decoding frame %d\n", frame);
			av_free(buf);
			return -1;
		}
		if (gotPicture) {
			out_pic = avcodec_alloc_frame();
			if (!out_pic)
				return -1;
			img_convert_ctx = sws_getContext(pContext->width, pContext->height, pContext->pix_fmt,
					pContext->width, pContext->height, PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);
			if (!img_convert_ctx)
				return -1;

			avpicture_fill((AVPicture *)out_pic, buf, PIX_FMT_RGB24, pContext->width, pContext->height);
			sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize, 0, pContext->height, out_pic->data, out_pic->linesize);
			sws_freeContext(img_convert_ctx);
			img_convert_ctx = NULL;
			pCallback(out_pic);
			savep++;
			//			if((savep%15)==0)
			//				SaveFrame(out_pic, pContext->width,	pContext->height, savep);
			av_free(buf);
			av_free(out_pic);
			out_pic = NULL;
		}
		avpkt.size -= len;
		avpkt.data += len;
		frame++;
	}
	return 0;
}
