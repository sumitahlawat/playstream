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

ipcam_controller::ipcam_controller (char* URL)
{
    fprintf (stderr, "Contruct IPCAM control engine\n");

    IPCAM_rtsp = new ipcam_rtsp_play();
	IPCAM_rtsp_rec = new ipcam_rtsp_rec();

    pRTSPUrl = URL;
	filename = NULL;

    fprintf (stderr, "Contruct IPCAM control engine completed\n");
}

ipcam_controller::~ipcam_controller ()
{
    fprintf (stderr, "Destroy IPCAM control engine\n");

	filename = NULL;

    fprintf (stderr, "Destroy IPCAM control engine completed\n");
}

int ipcam_controller::InitMedia (ringbufferwriter *pVideoWriter,
								 ringbufferwriter *pAudioWriter)
{
    fprintf (stderr, "Enter InitMedia()\n");

	m_pRBufferAudioWriterRx = pAudioWriter;
	m_pRBufferVideoWriterRx = pVideoWriter;

	if (IPCAM_rtsp->Init(pRTSPUrl, m_pRBufferVideoWriterRx, m_pRBufferAudioWriterRx) != 1)
    {
        fprintf (stderr, "setupVideoCall:RTSP Rx Configuration failure\n");
        return -1;
    }

    fprintf (stderr, "IPCAM%d InitMedia() completed\n", bAdapt);

    return 1;
}

int ipcam_controller::InitMedia (char* fname, int fps)
{
	this->filename = fname;
	
	if (IPCAM_rtsp_rec->Init(pRTSPUrl, filename, fps) != 1)
	{
		fprintf (stderr, "setupVideoCall:RTSP Rx Configuration failure\n");
		return -1;
	}
	return 1;
}

int ipcam_controller::StartMedia ()
{
    fprintf (stderr, "Enter StartMedia()\n");

    IPCAM_rtsp->StartRecv();

    fprintf (stderr, "StartMedia() completed\n");

    return 1;
}

int ipcam_controller::StartMedia_Rec()
{
	IPCAM_rtsp_rec->StartRecv();
	return 1;
}

int ipcam_controller::CloseMedia ()
{
    fprintf (stderr, "Enter CloseMedia()\n");

    IPCAM_rtsp->Close();

    fprintf (stderr, "CloseMedia() completed\n");

    return 1;
}

int ipcam_controller::CloseMedia_Rec ()
{
	IPCAM_rtsp_rec->Close();	
	return 1;
}

void *ipcam_controller::StartIPCAMTx (void *arg)
{
    fprintf (stderr, "Enter StartIPCAMTx()\n");

    ipcam_controller * IPCAM_controller = (ipcam_controller *)arg;

    fprintf (stderr, "StartIPCAMTx() completed\n");

    return 0;
}

void *ipcam_controller::StartIPCAMRx (void *arg)
{
    fprintf (stderr, "Enter StartIPCAMRx()\n");
    ipcam_controller * IPCAM_controller = (ipcam_controller *)arg;

    fprintf (stderr, "StartIPCAMRx() completed\n");

	return 0;
}


