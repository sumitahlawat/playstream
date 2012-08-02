/*
 * ipcam_controller.cpp
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

#include "ipcam_controller.h"
#include "player.h"

ipcam_controller::ipcam_controller (char* URL)
{
    IPCAM_rtsp_play = new ipcam_rtsp_play();
	IPCAM_rtsp_rec = new ipcam_rtsp_rec();

    pRTSPUrl = URL;
	filename = NULL;
}

ipcam_controller::~ipcam_controller ()
{
    LOGI ( "Destroy IPCAM control engine\n");
	filename = NULL;
    LOGI ("Destroy IPCAM control engine completed\n");
}

int ipcam_controller::InitMedia ()
{
    LOGI ("Enter ipcam_controller::InitMedia Player \n");
	if (IPCAM_rtsp_play->Init(pRTSPUrl) != 1)
    {
        LOGI ("setupVideoCall:RTSP Rx Configuration failure\n");
        return -1;
    }
    return 1;
}

int ipcam_controller::InitMedia (char* fname, int fps)
{
	this->filename = fname;
	LOGI("Init recorder \n");
	if (IPCAM_rtsp_rec->Init(pRTSPUrl, filename, fps) != 1)
	{
		LOGI ("setupVideoCall:RTSP Rx Configuration failure\n");
		return -1;
	}
	return 1;
}

int ipcam_controller::StartMedia ()
{
    LOGI ("Enter StartMedia()\n");
    IPCAM_rtsp_play->StartRecv();
    LOGI ("StartMedia() completed\n");
    return 1;
}

int ipcam_controller::StartMedia_Rec()
{
	IPCAM_rtsp_rec->StartRecv();
	return 1;
}

int ipcam_controller::CloseMedia ()
{
    LOGI ("Enter CloseMedia()\n");
    IPCAM_rtsp_play->Close();
    LOGI ( "CloseMedia() completed\n");
    return 1;
}

int ipcam_controller::CloseMedia_Rec ()
{
	IPCAM_rtsp_rec->Close();	
	return 1;
}

void *ipcam_controller::StartIPCAMTx (void *arg)
{
    LOGI ( "Enter StartIPCAMTx()\n");
    ipcam_controller * IPCAM_controller = (ipcam_controller *)arg;
    LOGI ( "StartIPCAMTx() completed\n");
    return 0;
}

void *ipcam_controller::StartIPCAMRx (void *arg)
{
    LOGI ( "Enter StartIPCAMRx()\n");
    ipcam_controller * IPCAM_controller = (ipcam_controller *)arg;
    LOGI ( "StartIPCAMRx() completed\n");
	return 0;
}
