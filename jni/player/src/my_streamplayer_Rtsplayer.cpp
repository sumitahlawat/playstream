#include <android/bitmap.h>

#undef PixelFormat

#include "ipcam_vdec.h"
#include "ipcam_camera.h"
#include "player.h"

#include "my_streamplayer_Rtsplayer.h"

ipcam_camera *MyIPCAM1 = NULL;
ipcam_camera *MyIPCAM2 = NULL;
ipcam_camera *MyIPCAM3 = NULL;
ipcam_camera *MyIPCAM4 = NULL;

ipcam_vdec *videoDecode1 = NULL;
ipcam_vdec *videoDecode2 = NULL;
ipcam_vdec *videoDecode3 = NULL;
ipcam_vdec *videoDecode4 = NULL;

AndroidBitmapInfo  info;

static int errorCam1 = 0;
static int errorCam2 = 0;
static int errorCam3 = 0;
static int errorCam4 = 0;

void* pixel1;
void* pixel2;
void* pixel3;
void* pixel4;

JavaVM *gJavaVM;
jmethodID mid_cb_string;
jclass      mClass;     // Reference to jtxRemSkt class
jobject     mObject;    // Weak ref to jtxRemSkt Java object to call on

void return_Message_to_Java(char * ch_msg);

/*
 * Write a frame worth of video (in pFrame) into the Android bitmap
 * described by info using the raw pixel buffer.  It's a very inefficient
 * draw routine, but it's easy to read. Relies on the format of the
 * bitmap being 8bits per color component plus an 8bit alpha channel.
 */

static void fill_bitmap(AndroidBitmapInfo*  info, void *pixels, AVFrame *pFrame)
{
	uint8_t *frameLine;
	int  yy;
	for (yy = 0; yy < info->height; yy++) {
		uint8_t*  line = (uint8_t*)pixels;
		frameLine = (uint8_t *)pFrame->data[0] + (yy * pFrame->linesize[0]);

		int xx;
		for (xx = 0; xx < info->width; xx++) {
			int out_offset = xx * 4;
			int in_offset = xx * 3;

			line[out_offset] = frameLine[in_offset];
			line[out_offset+1] = frameLine[in_offset+1];
			line[out_offset+2] = frameLine[in_offset+2];
			line[out_offset+3] = 0;
		}
		pixels = (char*)pixels + info->stride;
	}
}
/**
 * return message string to the Java side
 *  @ch_mag char array of message
 */
void return_Message_to_Java(char * ch_msg)
{
	int status;
	JNIEnv *env;
	bool isAttached = false;

	env=NULL;

	status = gJavaVM->GetEnv((void **) &env, JNI_VERSION_1_4);
	if(status < 0) {
		//	LOGE("callback_handler: failed to get JNI environment, "				"assuming native thread");
		status = gJavaVM->AttachCurrentThread(&env, NULL);
		if(status < 0) {
			LOGE("callback_handler: failed to attach "
					"current thread");
		}
		isAttached = true;
	}
	jstring str_cb = env->NewStringUTF(ch_msg);
	env->CallStaticVoidMethod(mClass, mid_cb_string, str_cb);
	(env)->DeleteLocalRef(str_cb);

	if(isAttached)
		gJavaVM->DetachCurrentThread();
}

void DisplayCb_1 (AVFrame *Frame)
{
//	LOGI("%s : %d\n",__func__,__LINE__);
	//send data back to java side to display
	fill_bitmap(&info, pixel1, Frame);
	return_Message_to_Java("disp1");
}

void DisplayCb_2 (AVFrame *Frame)
{
//	LOGI("%s : %d\n",__func__,__LINE__);
	fill_bitmap(&info, pixel2, Frame);
	return_Message_to_Java("disp2");
}

void DisplayCb_3 (AVFrame *Frame)
{
//	LOGI("%s : %d\n",__func__,__LINE__);
	fill_bitmap(&info, pixel3, Frame);
	return_Message_to_Java("disp3");
}

void DisplayCb_4 (AVFrame *Frame)
{
//	LOGI("%s : %d\n",__func__,__LINE__);
	fill_bitmap(&info, pixel4, Frame);
	return_Message_to_Java("disp4");
}

/*
 * Class:     my_streamplayer_Rtsplayer
 * Method:    nativeSetup
 * Signature: (Ljava/lang/Object;)V
 */
void Java_my_streamplayer_Rtsplayer_init(JNIEnv *env,jobject thiz, jobject weak_this)
{
	LOGI("do init once");
	jclass clazz = env->GetObjectClass(thiz);
	mClass = (jclass)env->NewGlobalRef(clazz);
	mObject  = env->NewGlobalRef(weak_this);
	mid_cb_string = env->GetStaticMethodID(mClass, "rcmcallback_string","(Ljava/lang/String;)V");
	return;
}

/*
 * Class:     my_streamplayer_Rtsplayer
 * Method:    CreateRec
 * Signature: (Landroid/graphics/Bitmap;Ljava/lang/String;Ljava/lang/String;IIII)V
 */
void Java_my_streamplayer_Rtsplayer_CreateRec
(JNIEnv *env, jobject, jstring URL, jstring recfile, jint ID, jint x, jint y, jint frame_rate, jobject bitmap)
{
	jboolean isCopy;
	char* rtspURL = (char*) env->GetStringUTFChars(URL, &isCopy);
	char* RecFile = (char*) env->GetStringUTFChars(recfile, &isCopy);
	LOGI("CAM ID %d \tURL: %s \t FILE: %s\n ", ID, rtspURL, RecFile);

	int ret;

	switch(ID)
	{
	case 1:
		if ((ret = AndroidBitmap_getInfo(env, bitmap, &info)) < 0) {
			LOGI("AndroidBitmap_getInfo() failed ! error=%d", ret);
			return;
		}
		LOGI("Checked on the bitmap 1 ");

		if ((ret = AndroidBitmap_lockPixels(env, bitmap, &pixel1)) < 0) {
			LOGE("AndroidBitmap_lockPixels() failed ! error=%d", ret);
		}

		MyIPCAM1 = new ipcam_camera(rtspURL, ID, frame_rate);
		MyIPCAM1->init();      // initialize ring buffers
		MyIPCAM1->set_recFile(RecFile);

		//create FFMPEG Decoder - must be getting created here
		videoDecode1 = ipcam_vdec::getInstance(1);
		videoDecode1->setparam(x, y);
		MyIPCAM1->pVDec = videoDecode1;

		usleep (1000);
		videoDecode1->InitMPEG4Dec();

		errorCam1 = MyIPCAM1->play_connect();
		//	MyIPCAM1->rec_connect();

		LOGD("IPCAM %d errorCam1 %d  widthframe : %d, heightframe =%d \n", ID, errorCam1 , x, y);
		break;

	case 2:
		if ((ret = AndroidBitmap_getInfo(env, bitmap, &info)) < 0) {
			LOGI("AndroidBitmap_getInfo() failed ! error=%d", ret);
			return;
		}
		LOGI("Checked on the bitmap 2");

		if ((ret = AndroidBitmap_lockPixels(env, bitmap, &pixel2)) < 0) {
			LOGE("AndroidBitmap_lockPixels() failed ! error=%d", ret);
		}

		MyIPCAM2 = new ipcam_camera(rtspURL, ID, frame_rate);
		MyIPCAM2->init();      // initialize ring buffers
		MyIPCAM2->set_recFile(RecFile);

		//create FFMPEG Decoder - must be getting created here
		videoDecode2 = ipcam_vdec::getInstance(2);
		videoDecode2->setparam(x, y);
		MyIPCAM2->pVDec = videoDecode2;
		//now initialize init
		usleep (1000);
		videoDecode2->InitMPEG4Dec();
		errorCam2 = MyIPCAM2->play_connect();

		LOGD("IPCAM %d errorCam2 %d  widthframe : %d, heightframe =%d \n", ID, errorCam2 , x, y);
		break;

	case 3:
		if ((ret = AndroidBitmap_getInfo(env, bitmap, &info)) < 0) {
			LOGI("AndroidBitmap_getInfo() failed ! error=%d", ret);
			return;
		}
		LOGI("Checked on the bitmap");

		if ((ret = AndroidBitmap_lockPixels(env, bitmap, &pixel3)) < 0) {
			LOGE("AndroidBitmap_lockPixels() failed ! error=%d", ret);
		}
		LOGI("Grabbed the pixels");

		MyIPCAM3 = new ipcam_camera(rtspURL, ID, frame_rate);
		MyIPCAM3->init();      // initialize ring buffers
		MyIPCAM3->set_recFile(RecFile);

		//create FFMPEG Decoder - must be getting created here
		videoDecode3 = ipcam_vdec::getInstance(3);
		videoDecode3->setparam(x, y);
		MyIPCAM3->pVDec = videoDecode3;

		usleep (1000);
		videoDecode3->InitMPEG4Dec();

		errorCam3 = MyIPCAM3->play_connect();
		//	MyIPCAM1->rec_connect();

		LOGD("IPCAM %d errorCam3 %d  widthframe : %d, heightframe =%d \n", ID, errorCam3 , x, y);
		break;

	case 4:
		if ((ret = AndroidBitmap_getInfo(env, bitmap, &info)) < 0) {
			LOGI("AndroidBitmap_getInfo() failed ! error=%d", ret);
			return;
		}
		LOGI("Checked on the bitmap");

		if ((ret = AndroidBitmap_lockPixels(env, bitmap, &pixel4)) < 0) {
			LOGE("AndroidBitmap_lockPixels() failed ! error=%d", ret);
		}
		LOGI("Grabbed the pixels");

		MyIPCAM4 = new ipcam_camera(rtspURL, ID, frame_rate);
		MyIPCAM4->init();      // initialize ring buffers
		MyIPCAM4->set_recFile(RecFile);

		//create FFMPEG Decoder - must be getting created here
		videoDecode4 = ipcam_vdec::getInstance(4);
		videoDecode4->setparam(x, y);
		MyIPCAM4->pVDec = videoDecode4;

		usleep (1000);
		videoDecode4->InitMPEG4Dec();

		errorCam4 = MyIPCAM4->play_connect();
		//	MyIPCAM1->rec_connect();

		LOGD("IPCAM %d errorCam4 %d  widthframe : %d, heightframe =%d \n", ID, errorCam4 , x, y);
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
		//	MyIPCAM1->start_recording();
		if(errorCam1 == 1)
		{
			MyIPCAM1->start_playback();
		}
		break;
	case 2:
		if(errorCam2 == 1)
		{
			MyIPCAM2->start_playback();
		}
		break;
	case 3:
		if(errorCam3 == 1)
		{
			MyIPCAM3->start_playback();
		}
		break;
	case 4:
		if(errorCam4 == 1)
		{
			MyIPCAM4->start_playback();
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
	case 2:
		MyIPCAM2->stop_playback();
		LOGD("IPCAM %d Recording Stopped\n", ID);
		break;
	case 3:
		MyIPCAM3->stop_playback();
		LOGD("IPCAM %d Recording Stopped\n", ID);
		break;
	case 4:
		MyIPCAM4->stop_playback();
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
	case 2:
		MyIPCAM2->deinit();
		LOGD("IPCAM %d DEINIT Complete\n", ID);
		break;
	case 3:
		MyIPCAM3->deinit();
		LOGD("IPCAM %d DEINIT Complete\n", ID);
		break;
	case 4:
		MyIPCAM4->deinit();
		LOGD("IPCAM %d DEINIT Complete\n", ID);
		break;
	default:
		LOGE("INVALID CAM-ID %d", ID);
		break;
	}
}



static const char *classPathName = "my/streamplayer/Rtsplayer";

static JNINativeMethod methods[] = {
		{"CreateRec", "(Ljava/lang/String;Ljava/lang/String;IIIILandroid/graphics/Bitmap;)V", (void *)Java_my_streamplayer_Rtsplayer_CreateRec},
		{"StartRec", "(I)V",	(void *)Java_my_streamplayer_Rtsplayer_StartRec},
		{"StopRec", "(I)V", (void *)Java_my_streamplayer_Rtsplayer_StopRec},
		{"DestroyRec", "(I)V", (void *)Java_my_streamplayer_Rtsplayer_DestroyRec},
};

/*
 * Register several native methods for one class.
 */
static int registerNativeMethods(JNIEnv* env, const char* className,
		JNINativeMethod* gMethods, int numMethods)
{
	jclass clazz;

	clazz = env->FindClass(className);
	if (clazz == NULL) {
		LOGE("Native registration unable to find class '%s'", className);
		return JNI_FALSE;
	}
	if (env->RegisterNatives(clazz, gMethods, numMethods) < 0) {
		LOGE("RegisterNatives failed for '%s'", className);
		return JNI_FALSE;
	}

	return JNI_TRUE;
}

static int registerNatives(JNIEnv* env)
{
	if (!registerNativeMethods(env, classPathName,
			methods, sizeof(methods) / sizeof(methods[0]))) {
		return JNI_FALSE;
	}
	return JNI_TRUE;
}

jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
	JNIEnv *env;
	gJavaVM = vm;
	int result;

	LOGI("JNI_OnLoad called");
	if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
		LOGE("(JNI_OnLoad()) .... Failed to get the environment using GetEnv()");
		return -1;
	}

	if (registerNatives(env) != JNI_TRUE) {
		LOGE("ERROR: registerNatives failed .... (JNI_OnLoad())");
		goto bail;
	}

	result = JNI_VERSION_1_4;

	bail:
	return result;
}
