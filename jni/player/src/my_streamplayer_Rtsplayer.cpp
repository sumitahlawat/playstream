#include <android/bitmap.h>

#undef PixelFormat

#define VIDEO_WIDTH		320
#define VIDEO_HEIGHT	240

#include "ipcam_vdec.h"
#include "ipcam_camera.h"
#include "player.h"

#include "my_streamplayer_Rtsplayer.h"

ipcam_camera *MyIPCAM1 = NULL;
ipcam_camera *MyIPCAM2 = NULL;
ipcam_camera *MyIPCAM3 = NULL;
ipcam_camera *MyIPCAM4 = NULL;

ipcam_vdec *videoDecode1 = NULL;

static int errorCam1 = 0;

void DisplayCb_1 (uint8_t* aData[], int aDataLen)
{
	LOGI("%s : %d  %d\n",__func__,__LINE__,aDataLen);
}

void DisplayCb_2 (uint8_t* aData[], int aDataLen)
{
	LOGI("display frame data after callback");
}

void DisplayCb_3 (uint8_t* aData[], int aDataLen)
{
	LOGI("display frame data after callback");
}

void DisplayCb_4 (uint8_t* aData[], int aDataLen)
{
	LOGI("display frame data after callback");
}

/*
 * Class:     my_streamplayer_Rtsplayer
 * Method:    CreateRec
 * Signature: (Landroid/graphics/Bitmap;Ljava/lang/String;Ljava/lang/String;IIII)V
 */
void Java_my_streamplayer_Rtsplayer_CreateRec
(JNIEnv *env, jobject, jstring URL, jstring recfile, jint ID, jint x, jint y, jint frame_rate)
{
	jboolean isCopy;
	char* rtspURL = (char*) env->GetStringUTFChars(URL, &isCopy);
	char* RecFile = (char*) env->GetStringUTFChars(recfile, &isCopy);

	LOGI("CAM ID %d \tURL: %s \t FILE: %s\n ", ID, rtspURL, RecFile);

	switch(ID)
	{
	case 1:
		MyIPCAM1 = new ipcam_camera(rtspURL, ID, frame_rate);
		MyIPCAM1->init();      // initialize ring buffers
		MyIPCAM1->set_recFile(RecFile);

		//create FFMPEG Decoder
		videoDecode1 = ipcam_vdec::getInstance(1);
		MyIPCAM1->pVDec = videoDecode1;

		usleep (1000);
		videoDecode1->InitMPEG4Dec();

		errorCam1 = MyIPCAM1->play_connect();
		//	MyIPCAM1->rec_connect();

		LOGD("IPCAM %d errorCam1 %d\n", ID, errorCam1);
		break;

	default:
		LOGE("INVALID CAM-ID %d", ID);
		break;
	}
}

/*
 * Class:     my_streamplayer_Rtsplayer
 * Method:    StartRec
 * Signature: (I)V
 */
void  Java_my_streamplayer_Rtsplayer_StartRec
(JNIEnv *env, jobject obj, jint ID)
{
	switch(ID)
	{
	case 1:
		//			MyIPCAM1->start_recording();
		if(errorCam1 == 1)
		{
			MyIPCAM1->start_playback();
		}
		break;

	default:
		LOGE("INVALID CAM-ID %d", ID);
		break;
	}
}

/*
 * Class:     my_streamplayer_Rtsplayer
 * Method:    StopRec
 * Signature: (I)V
 */
void Java_my_streamplayer_Rtsplayer_StopRec
(JNIEnv *env, jobject obj, jint ID)
{
	switch(ID)
	{
	case 1:
		//			MyIPCAM1->stop_recording();
		MyIPCAM1->stop_playback();
		LOGD("IPCAM %d Recording Stopped\n", ID);
		break;

	default:
		LOGE("INVALID CAM-ID %d", ID);
		break;
	}
}

/*
 * Class:     my_streamplayer_Rtsplayer
 * Method:    DestroyRec
 * Signature: (I)V
 */
void  Java_my_streamplayer_Rtsplayer_DestroyRec
(JNIEnv *env, jobject obj, jint ID)
{
	switch(ID)
	{
	case 1:
		MyIPCAM1->deinit();
		LOGD("IPCAM %d DEINIT Complete\n", ID);
		break;

	default:
		LOGE("INVALID CAM-ID %d", ID);
		break;
	}
}
