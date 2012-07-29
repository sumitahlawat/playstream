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
//#include <surfaceflinger/Surface.h>
//#include <surfaceflinger/ISurface.h>
//#include <surfaceflinger/SurfaceComposerClient.h>
//#include <surfaceflinger/ISurfaceComposer.h>

//SoftwareRenderer
//#include "SoftwareRenderer.h"

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

//SoftwareRenderer *renderer1 = NULL;
//SoftwareRenderer *renderer2 = NULL;
//SoftwareRenderer *renderer3 = NULL;
//SoftwareRenderer *renderer4 = NULL;

//sp<SurfaceComposerClient> client1;
//sp<SurfaceControl> surface1;
//sp<ISurface> isurface1;

//sp<SurfaceComposerClient> client2;
//sp<SurfaceControl> surface2;
//sp<ISurface> isurface2;

//sp<SurfaceComposerClient> client3;
//sp<SurfaceControl> surface3;
//sp<ISurface> isurface3;

//sp<SurfaceComposerClient> client4;
//sp<SurfaceControl> surface4;
//sp<ISurface> isurface4;

//namespace android {
//	class Test {
//	public:
//		static const sp<ISurface>& getISurface(const sp<SurfaceControl>& s) {
//			return s->getISurface();
//		}
//	};
//};


void render_frame(uint8_t* aData[], int aDataLen)
{

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

	switch(ID)
	{
		case 1:
			MyIPCAM1 = new ipcam_camera(rtspURL, ID, frame_rate);
			MyIPCAM1->init();      // initialize ring buffers
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
//			surface1->clear();
			LOGD("IPCAM %d DEINIT Complete\n", ID);
			break;
			
		default:
			LOGE("INVALID CAM-ID %d", ID);
			break;
	}
	return 0;	
}
