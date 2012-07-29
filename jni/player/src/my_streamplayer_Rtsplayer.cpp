#include <android/bitmap.h>

const char *classPathName = "my/streamplayer/Rtsplayer";

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

void DisplayCb_1 (uint8_t* aData[], int aDataLen)
{
	LOGD("display frame data after callback");
}

void DisplayCb_2 (uint8_t* aData[], int aDataLen)
{
	LOGD("display frame data after callback");
}

void DisplayCb_3 (uint8_t* aData[], int aDataLen)
{
	LOGD("display frame data after callback");
}

void DisplayCb_4 (uint8_t* aData[], int aDataLen)
{
	LOGD("display frame data after callback");
}

#ifndef _Included_my_streamplayer_Rtsplayer
#define _Included_my_streamplayer_Rtsplayer
#ifdef __cplusplus
extern "C" {
#endif

static int errorCam1 = 0;
static JavaVM *gJavaVM;
static jobject gInterfaceObject;

/*
 * Class:     my_streamplayer_Rtsplayer
 * Method:    CreateRec
 * Signature: (Landroid/graphics/Bitmap;Ljava/lang/String;Ljava/lang/String;IIII)V
 */
void Java_my_streamplayer_Rtsplayer_CreateRec
(JNIEnv *env, jobject, jstring bitmap, jstring URL, jstring recfile, jint ID, jint x, jint y, jint frame_rate)
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
		//			MyIPCAM1->rec_connect();

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

void initClassHelper(JNIEnv *env, const char *path, jobject *objptr)
{
	jclass cls = env->FindClass(path);

	if(!cls) {
		LOGI("initClassHelper: failed to get %s class reference", path);
		return;
	}

	jmethodID constr = env->GetMethodID(cls, "<init>", "()V");
	if(!constr) {
		LOGI("initClassHelper: failed to get %s constructor", path);
		return;
	}

	jobject obj = env->NewObject(cls, constr);
	if(!obj) {
		LOGI("initClassHelper: failed to create a %s object", path);
		return;
	}
	(*objptr) = env->NewGlobalRef(obj);
	LOGI("initClassHelper done");
}

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
	JNIEnv *env;
	gJavaVM = vm;
	jclass clazz;
	int result;

	LOGI("JNI_OnLoad called");
	if (gJavaVM->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
		LOGI("(JNI_OnLoad()) .... Failed to get the environment using GetEnv()");
		return -1;
	}

	initClassHelper(env, classPathName, &gInterfaceObject);
	JNINativeMethod methods[] = {
		{"native_gl_resize", "(II)V", (void *) Java_my_streamplayer_Rtsplayer_native_1gl_1resize},
		{"native_gl_render", "()V", (void *) Java_my_streamplayer_Rtsplayer_native_1gl_1render},
		{"CreateRec", "(IIII)V", (void *) Java_my_streamplayer_Rtsplayer_CreateRec},
		{"StartRec", "(I)V", (void *) Java_my_streamplayer_Rtsplayer_StartRec},
		{"StopRec", "(I)V", (void *) Java_my_streamplayer_Rtsplayer_StopRec},
		{"DestroyRec", "(I)V", (void *) Java_my_streamplayer_Rtsplayer_DestroyRec},
	};
	clazz = env->FindClass(classPathName);

	//LOGI(" result is %d",(*env)->RegisterNatives(env, clazz, methods, sizeof(methods)/sizeof(JNINativeMethod)));
	if(env->RegisterNatives(clazz, methods, sizeof(methods)/sizeof(JNINativeMethod)) != JNI_OK)
	{
		LOGI("Failed to register native methods");
		return -1;
	}
	result = JNI_VERSION_1_4;
	LOGI("initClassHelper done 12333");
	return result;;
}


#ifdef __cplusplus
}
#endif
#endif
