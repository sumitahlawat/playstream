/*
 * uis_monitor_Rtsplayer.cpp
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

#undef PixelFormat

#define VIDEO_WIDTH	192//320
#define VIDEO_HEIGHT	242//240

// SurfaceFlinger
#include <surfaceflinger/Surface.h>
#include <surfaceflinger/ISurface.h>
#include <surfaceflinger/SurfaceComposerClient.h>
#include <surfaceflinger/ISurfaceComposer.h>

//SoftwareRenderer
#include "SoftwareRenderer.h"

#include "ipcam_camera.h"
#include "player.h"

#include "com_uis_monitor_Rtsplayer.h"

ipcam_camera *MyIPCAM1 = NULL;
ipcam_camera *MyIPCAM2 = NULL;
ipcam_camera *MyIPCAM3 = NULL;
ipcam_camera *MyIPCAM4 = NULL;

static int errorCam1 = 0;
static int errorCam2 = 0;
static int errorCam3 = 0;
static int errorCam4 = 0;

ipcam_vdec *videoDecode1 = NULL;
ipcam_vdec *videoDecode2 = NULL;
ipcam_vdec *videoDecode3 = NULL;
ipcam_vdec *videoDecode4 = NULL;

SoftwareRenderer *renderer1 = NULL;
SoftwareRenderer *renderer2 = NULL;
SoftwareRenderer *renderer3 = NULL;
SoftwareRenderer *renderer4 = NULL;

sp<SurfaceComposerClient> client1;
sp<SurfaceControl> surface1;
sp<ISurface> isurface1;

sp<SurfaceComposerClient> client2;
sp<SurfaceControl> surface2;
sp<ISurface> isurface2;

sp<SurfaceComposerClient> client3;
sp<SurfaceControl> surface3;
sp<ISurface> isurface3;

sp<SurfaceComposerClient> client4;
sp<SurfaceControl> surface4;
sp<ISurface> isurface4;

namespace android {
	class Test {
	public:
		static const sp<ISurface>& getISurface(const sp<SurfaceControl>& s) {
			return s->getISurface();
		}
	};
};

void DisplayCb_1 (uint8_t* aData[], int aDataLen)
{
	size_t offset = 0;
	unsigned fsize = (aDataLen*2)/3;
	//LOGV("DisplayCb callback -- display decoded framelength %d\n", aDataLen);
	
	uint8_t *tmpbuf = new uint8_t[aDataLen];
	memset(tmpbuf, 0, aDataLen);
	
	memcpy(tmpbuf, aData[0], size_t(fsize));
	offset += size_t(fsize);
	memcpy(tmpbuf+offset, aData[1], size_t(fsize/4));
	offset += size_t(fsize/4);
	memcpy(tmpbuf+offset, aData[2], size_t(fsize/4));
	offset += size_t(fsize/4);
	
	if(renderer1 != NULL) {
		renderer1->render(tmpbuf,  aDataLen, NULL);
		//LOGV("Rendering CAM 1");
	}
	
	//fprintf(stderr, "*********total frame size %d, camID %d\n",offset,camID );
		
		free (tmpbuf);
}

void DisplayCb_2 (uint8_t* aData[], int aDataLen)
{
	size_t offset = 0;
	unsigned fsize = (aDataLen*2)/3;
	//fprintf(stderr, "DisplayCb callback -- display decoded framelength %d\n", ysize/*aDataLen*/);
	
	uint8_t *tmpbuf = new uint8_t[aDataLen];
	memset(tmpbuf, 0, aDataLen);
	
	memcpy(tmpbuf, aData[0], size_t(fsize));
	offset += size_t(fsize);
	memcpy(tmpbuf+offset, aData[1], size_t(fsize/4));
	offset += size_t(fsize/4);
	memcpy(tmpbuf+offset, aData[2], size_t(fsize/4));
	offset += size_t(fsize/4);
	
	if(renderer2 != NULL)
		renderer2->render(tmpbuf,  aDataLen, NULL);
	
	//fprintf(stderr, "*********total frame size %d, camID %d\n",offset,camID );
		
		free (tmpbuf);
}

void DisplayCb_3 (uint8_t* aData[], int aDataLen)
{
	size_t offset = 0;
	unsigned fsize = (aDataLen*2)/3;
	//fprintf(stderr, "DisplayCb callback -- display decoded framelength %d\n", ysize/*aDataLen*/);
	
	uint8_t *tmpbuf = new uint8_t[aDataLen];
	memset(tmpbuf, 0, aDataLen);
	
	memcpy(tmpbuf, aData[0], size_t(fsize));
	offset += size_t(fsize);
	memcpy(tmpbuf+offset, aData[1], size_t(fsize/4));
	offset += size_t(fsize/4);
	memcpy(tmpbuf+offset, aData[2], size_t(fsize/4));
	offset += size_t(fsize/4);
	
	if(renderer3 != NULL)
		renderer3->render(tmpbuf,  aDataLen, NULL);
	
	//fprintf(stderr, "*********total frame size %d, camID %d\n",offset,camID );
		
		free (tmpbuf);
}

void DisplayCb_4 (uint8_t* aData[], int aDataLen)
{
	size_t offset = 0;
	unsigned fsize = (aDataLen*2)/3;
	//fprintf(stderr, "DisplayCb callback -- display decoded framelength %d\n", ysize/*aDataLen*/);
	
	uint8_t *tmpbuf = new uint8_t[aDataLen];
	memset(tmpbuf, 0, aDataLen);
	
	memcpy(tmpbuf, aData[0], size_t(fsize));
	offset += size_t(fsize);
	memcpy(tmpbuf+offset, aData[1], size_t(fsize/4));
	offset += size_t(fsize/4);
	memcpy(tmpbuf+offset, aData[2], size_t(fsize/4));
	offset += size_t(fsize/4);
	
	if(renderer4 != NULL)
		renderer4->render(tmpbuf,  aDataLen, NULL);
	
	//fprintf(stderr, "*********total frame size %d, camID %d\n",offset,camID );
		
		free (tmpbuf);
}

void create_surfaces(int ID, int x, int y)
{
	if(ID==1)
	{
		//Create Surfaces
		client1 = new SurfaceComposerClient();
		client1->setOrientation(0, ISurfaceComposer::eOrientationDefault, 0);
		// create pushbuffer surface
		surface1 = client1->createSurface(getpid(), 0,	VIDEO_WIDTH,
														VIDEO_HEIGHT,
														PIXEL_FORMAT_UNKNOWN,
														ISurfaceComposer::ePushBuffers);

		LOGD("Created SurfaceControl 1\n");

		client1->openTransaction();
		surface1->setLayer(1500000);
		surface1->setPosition(x,y);
		client1->closeTransaction();

		// get to the isurface
		isurface1 = Test::getISurface(surface1);
		LOGD("isurface = %p\n", isurface1.get());

		renderer1 = new SoftwareRenderer(OMX_COLOR_FormatYUV420Planar, isurface1,
										VIDEO_WIDTH, VIDEO_HEIGHT, VIDEO_WIDTH, VIDEO_HEIGHT);

		LOGD("///// Surface 1 Created ////// \n");
	}

	if(ID==2)
	{
		client2 = new SurfaceComposerClient();
		client2->setOrientation(0, ISurfaceComposer::eOrientationDefault, 0);

		// create pushbuffer surface
		surface2 = client2->createSurface(getpid(), 0,	VIDEO_WIDTH,
														VIDEO_HEIGHT,
														PIXEL_FORMAT_UNKNOWN,
														ISurfaceComposer::ePushBuffers);
		LOGD("Created SurfaceControl 2\n");

		client2->openTransaction();
		surface2->setLayer(100000);
		surface2->setPosition(x,y);
		client2->closeTransaction();

		// get to the isurface
		isurface2 = Test::getISurface(surface2);
		LOGD("isurface = %p\n", isurface2.get());

		renderer2 = new SoftwareRenderer(OMX_COLOR_FormatYUV420Planar, isurface2,
										VIDEO_WIDTH, VIDEO_HEIGHT, VIDEO_WIDTH, VIDEO_HEIGHT);

		LOGD("///// Surface 2 Created ////// \n");
	}

	if(ID==3)
	{
		client3 = new SurfaceComposerClient();
		client3->setOrientation(0, ISurfaceComposer::eOrientationDefault, 0);

		// create pushbuffer surface
		surface3 = client3->createSurface(getpid(), 0,	VIDEO_WIDTH,
														VIDEO_HEIGHT,
														PIXEL_FORMAT_UNKNOWN,
														ISurfaceComposer::ePushBuffers);
		LOGD("Created SurfaceControl 3\n");

		client3->openTransaction();
		surface3->setLayer(100000);
		surface3->setPosition(x,y);
		client3->closeTransaction();

		// get to the isurface
		isurface3 = Test::getISurface(surface3);
		LOGD("isurface = %p\n", isurface3.get());

		renderer3 = new SoftwareRenderer(OMX_COLOR_FormatYUV420Planar, isurface3,
										VIDEO_WIDTH, VIDEO_HEIGHT, VIDEO_WIDTH, VIDEO_HEIGHT);

		LOGD("///// Surface 3 Created ////// \n");
	}

	if (ID==4)
	{
		client4 = new SurfaceComposerClient();
		client4->setOrientation(0, ISurfaceComposer::eOrientationDefault, 0);

		// create pushbuffer surface
		surface4 = client4->createSurface(getpid(), 0,	VIDEO_WIDTH,
														VIDEO_HEIGHT,
														PIXEL_FORMAT_UNKNOWN,
														ISurfaceComposer::ePushBuffers);
		LOGD("Created SurfaceControl 4\n");

		client4->openTransaction();
		surface4->setLayer(500000);
		surface4->setPosition(x,y);
		client4->closeTransaction();

		// get to the isurface
		isurface4 = Test::getISurface(surface4);
		LOGD("isurface = %p\n", isurface4.get());

		renderer4 = new SoftwareRenderer(OMX_COLOR_FormatYUV420Planar, isurface4,
										VIDEO_WIDTH, VIDEO_HEIGHT, VIDEO_WIDTH, VIDEO_HEIGHT);

		LOGD("///// Surface 4 Created ////// \n");
	}	
}

jint Java_com_uis_monitor_Rtsplayer_CreateRec(JNIEnv *env, jobject obj,
										 jstring URL, jstring recfile,
										 jint ID, jint x, jint y,
										 jint frame_rate)
{
	jboolean isCopy;
	char* rtspURL = (char*) env->GetStringUTFChars(URL, &isCopy);
	char* RecFile = (char*) env->GetStringUTFChars(recfile, &isCopy);

	LOGD("CAM ID %d \tURL: %s \t FILE: %s\n ", ID, rtspURL, RecFile);

	create_surfaces(ID, x, y);
	
	switch(ID)
	{
		case 1:
			MyIPCAM1 = new ipcam_camera(rtspURL, ID, frame_rate);
			MyIPCAM1->init();
			MyIPCAM1->set_recFile(RecFile);
			
			errorCam1 = MyIPCAM1->play_connect();
			MyIPCAM1->rec_connect();

			//create FFMPEG Decoder
			videoDecode1 = new ipcam_vdec(1, VIDEO_WIDTH, VIDEO_HEIGHT);
			MyIPCAM1->pVDec = videoDecode1;
			
			usleep (1000);			
			videoDecode1->InitMPEG4Dec();
			
			LOGD("IPCAM %d errorCam1 %d\n", ID, errorCam1);
			break;

		case 2:
			MyIPCAM2 = new ipcam_camera(rtspURL, ID, frame_rate);
			MyIPCAM2->init();
			MyIPCAM2->set_recFile(RecFile);
			
			errorCam2 = MyIPCAM2->play_connect();
			MyIPCAM2->rec_connect();

			videoDecode2 = new ipcam_vdec(2, VIDEO_WIDTH, VIDEO_HEIGHT);
			MyIPCAM2->pVDec = videoDecode2;			
			usleep (1000);			
			videoDecode2->InitMPEG4Dec();
			
			break;

		case 3:
			MyIPCAM3 = new ipcam_camera(rtspURL, ID, frame_rate);
			MyIPCAM3->init();
			MyIPCAM3->set_recFile(RecFile);
			
			errorCam3 = MyIPCAM3->play_connect();
			MyIPCAM3->rec_connect();
			
			videoDecode3 = new ipcam_vdec(3, VIDEO_WIDTH, VIDEO_HEIGHT);
			MyIPCAM3->pVDec = videoDecode3;			
			usleep (1000);			
			videoDecode3->InitMPEG4Dec();
			
			break;

		case 4:
			MyIPCAM4 = new ipcam_camera(rtspURL, ID, frame_rate);
			MyIPCAM4->init();
			MyIPCAM4->set_recFile(RecFile);
			
			errorCam4 = MyIPCAM4->play_connect();
			MyIPCAM4->rec_connect();
			
			videoDecode4 = new ipcam_vdec(4, VIDEO_WIDTH, VIDEO_HEIGHT);
			MyIPCAM4->pVDec = videoDecode4;			
			usleep (1000);			
			videoDecode4->InitMPEG4Dec();
			
			break;

		default:
			LOGE("INVALID CAM-ID %d", ID);
			break;			
	}

	return 0;	
}

jint Java_com_uis_monitor_Rtsplayer_StartRec(JNIEnv *env, jobject obj, jint ID)
{
	
	switch(ID)
	{
		case 1:
			if(errorCam1 == 1)
			{
				MyIPCAM1->start_playback();
				MyIPCAM1->start_recording();
				LOGD("IPCAM %d Recording\n", ID);
			}
			break;

		case 2:
			if(errorCam2 == 1)
			{
				MyIPCAM2->start_playback();
				MyIPCAM2->start_recording();
				LOGD("IPCAM %d Recording\n", ID);
			}
			break;

		case 3:
			if(errorCam3 == 1)
			{
				MyIPCAM3->start_playback();
				MyIPCAM3->start_recording();
				LOGD("IPCAM %d Recording\n", ID);
			}
			break;

		case 4:
			if(errorCam4 == 1)
			{
				MyIPCAM4->start_playback();
				MyIPCAM4->start_recording();
				LOGD("IPCAM %d Recording\n", ID);
			}
			break;

		default:
			LOGE("INVALID CAM-ID %d", ID);
			break;
	}

	return 0;
}

jint Java_com_uis_monitor_Rtsplayer_StopRec(JNIEnv *env, jobject obj, jint ID)
{
	
	switch(ID)
	{
		case 1:
			MyIPCAM1->stop_recording();
			MyIPCAM1->stop_playback();
			LOGD("IPCAM %d Recording Stopped\n", ID);
			break;
			
		case 2:
			MyIPCAM2->stop_recording();
			MyIPCAM2->stop_playback();
			LOGD("IPCAM %d Recording Stopped\n", ID);
			break;
			
		case 3:
			MyIPCAM3->stop_recording();
			MyIPCAM3->stop_playback();
			LOGD("IPCAM %d Recording Stopped\n", ID);
			break;
			
		case 4:
			MyIPCAM4->stop_recording();
			MyIPCAM4->stop_playback();
			LOGD("IPCAM %d Recording Stopped\n", ID);
			break;
			
		default:
			LOGE("INVALID CAM-ID %d", ID);
			break;
	}
	
	return 0;
}

jint Java_com_uis_monitor_Rtsplayer_DestroyRec(JNIEnv *env, jobject obj, jint ID)
{
	
	switch(ID)
	{
		case 1:
			MyIPCAM1->deinit();
			surface1->clear();
			LOGD("IPCAM %d DEINIT Complete\n", ID);
			break;
			
		case 2:
			MyIPCAM2->deinit();
			surface2->clear();
			LOGD("IPCAM %d DEINIT Complete\n", ID);
			break;
			
		case 3:
			MyIPCAM3->deinit();
			surface3->clear();
			LOGD("IPCAM %d DEINIT Complete\n", ID);
			break;
			
		case 4:
			MyIPCAM4->deinit();
			surface4->clear();
			LOGD("IPCAM %d DEINIT Complete\n", ID);
			break;
			
		default:
			LOGE("INVALID CAM-ID %d", ID);
			break;
	}
	
	return 0;	
}

