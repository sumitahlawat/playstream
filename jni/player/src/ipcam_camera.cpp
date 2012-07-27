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

ipcam_camera::ipcam_camera(char* URL, int ID, int frame_rate)
{
    pIpCam = NULL;
	recfilename = NULL;
	fps = frame_rate;
	url = URL;
	camID = ID;
	isPlaying = 0;
	isRecording = 0;
}

ipcam_camera::~ipcam_camera()
{
	pIpCam = NULL;
	recfilename = NULL;
	fps = 0;
	url = NULL;
}

int ipcam_camera::init()
{
    int width, height;
    //Create IPCAM Controller
    pIpCam = new ipcam_controller(url);
	return 0;
}

void ipcam_camera::deinit()
{
}

int ipcam_camera::play_connect()
{
	return pIpCam->InitMedia();
}

int ipcam_camera::rec_connect()
{
	return pIpCam->InitMedia (recfilename, fps);
}

int ipcam_camera::start_playback()
{
	int status = pIpCam->StartMedia();
	if (status)
		isPlaying = 1;
	else
		return -1;

	return status;
}

int ipcam_camera::stop_playback()
{
	int status;

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

	if(recfilename != NULL)	{
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
