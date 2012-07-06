/*
 * ipcam_camera.cpp
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

#include "ipcam_camera.h"
#include "player.h"

//forward
void *ipcam_camera_thread (void *p);

ipcam_camera::ipcam_camera(char* URL, int ID, int frame_rate)
{
    pIpCam = NULL;

	pVRBCam = NULL;
	pARBCam = NULL;
	pVRBWriter = NULL;
	pARBWriter = NULL;
	pVRBReader = NULL;
	pARBReader = NULL;

	recfilename = NULL;
	fps = frame_rate;
	url = URL;
	camID = ID;
    pVDec = NULL;
	isPlaying = 0;
	isRecording = 0;
}

ipcam_camera::~ipcam_camera()
{
	pIpCam = NULL;
	pVRBCam = NULL;
	pARBCam = NULL;
	pVRBWriter = NULL;
	pARBWriter = NULL;
	pVRBReader = NULL;
	pARBReader = NULL;
	recfilename = NULL;
	fps = 0;
	url = NULL;
	pVDec = NULL;
	pIpCam = NULL;
}

int ipcam_camera::init()
{
    int width, height;

    //Create IPCAM Controller
    pIpCam = new ipcam_controller(url);

	//Create Ringbuffers
	pVRBCam = new ringbuffer(VIDEOBUFFER_SZ);
	pARBCam = new ringbuffer(AUDIOBUFFER_SZ);
	
	//Create Buffer Writers and Readers
	pVRBWriter = new ringbufferwriter (pVRBCam);
	pVRBReader = new ringbufferreader (pVRBCam);
	pVRBReader->SetLowWaterMark (4 * 1024);
	
	pARBWriter = new ringbufferwriter (pARBCam);
	pARBReader = new ringbufferreader (pARBCam);
	pARBReader->SetLowWaterMark (1 * 1024);

	return 0;
}

void ipcam_camera::deinit()
{
	if(pVRBCam)
		delete pVRBCam;

	if(pARBCam)
		delete pARBCam;

	if(pIpCam)
		delete pIpCam;
}

int ipcam_camera::play_connect()
{
	//char filename[] = "/mnt/sdcard/qtcam.mp4";
	return pIpCam->InitMedia (pVRBWriter, pARBWriter);
}

int ipcam_camera::rec_connect()
{
	//char filename[] = "/mnt/sdcard/qtcam.mp4";
	return pIpCam->InitMedia (recfilename, fps);
}

int ipcam_camera::start_playback()
{
	int status = pIpCam->StartMedia();
	if (status)
		isPlaying = 1;
	else
		return -1;

 	if(isPlaying)
 		pthread_create(&ringThreadVCam, NULL, ipcam_camera_thread, this);

	return status;
}

int ipcam_camera::stop_playback()
{
	void *res;
	int status;

//	status = pthread_join(ringThreadVCam, &res);
	ringThreadVCam = NULL;

	pIpCam->CloseMedia();
	isPlaying = 0;
	return 0;
}

void ipcam_camera::set_recFile(char* fname)
{
	recfilename = fname;
}

int ipcam_camera::start_recording()
{
	if (isRecording)
		return 0;

	if(recfilename != NULL)
	{
		int status = pIpCam->StartMedia_Rec();
	}
	else
		return -1;

	isRecording = 1;

	return 0;
}

int ipcam_camera::stop_recording()
{
	pIpCam->CloseMedia_Rec();
	isRecording = 0;
	return 0;
}

#if 1
#define FRAME_SZ 115200

void *ipcam_camera_thread (void *p)
{
	int oldPosition = 0;
	int offset1 = 0;
	int status;
	unsigned char buffer[FRAME_SZ];
	ipcam_camera* cam = reinterpret_cast<ipcam_camera*> (p);
	ipcam_vdec* videoDecode = cam->getVideoDec();

	LOGD ("Enter %s for %d\n", __func__, cam->camID);

	usleep(1000);

	while(cam->isPlaying)
	{
		//fprintf(stderr,"Enter Decoding mode *******************\n");
		status = cam->pVRBReader->ReadFromBufferTail(buffer, FRAME_SZ);
		offset1 += status;
		//fwrite((const void*) buffer, sizeof(unsigned char), status, cam->fp);
		usleep(500);
		//fprintf(stderr,"bytes written to rec file: %d\n", status);
		
		videoDecode->DecVideo ((unsigned char*) buffer,
								(unsigned int) status, &oldPosition);
	}
	LOGD ("Exit %s for %d\n", __func__, cam->camID);
	pthread_exit(0);

	return 0;
}
#endif
