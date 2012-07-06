/*
 * ipcam_rtsp.cpp
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

#include "ipcam_rtsp.h"
#include "player.h"

#ifndef mmioFOURCC
#define mmioFOURCC( ch0, ch1, ch2, ch3 )				\
	( (uint32_t)(uint8_t)(ch0) | ( (uint32_t)(uint8_t)(ch1) << 8 ) |	\
	( (uint32_t)(uint8_t)(ch2) << 16 ) | ( (uint32_t)(uint8_t)(ch3) << 24 ) )
#endif

void subsessionAfterPlaying(void* clientData);
void subsessionByeHandler(void* clientData);
void* StartPlay(void* arg);

ipcam_rtsp_play::ipcam_rtsp_play()
{
    TaskScheduler* scheduler = BasicTaskScheduler::createNew();
    env = BasicUsageEnvironment::createNew(*scheduler);
    rtspClient = NULL;
    session = NULL;

    watchVariable = 0;
}

ipcam_rtsp_play::~ipcam_rtsp_play()
{

}

int ipcam_rtsp_play::Init(char *url, ringbufferwriter *pCodecHRtspVideoBuffer,
					 ringbufferwriter *pCodecHRtspAudioBuffer)
{
    LOGI ("ipcam_rtsp::Init(): Entering. \n");
	pVideoBuffer = pCodecHRtspVideoBuffer;
	pAudioBuffer = pCodecHRtspAudioBuffer;
    char const* applicationName = "RTSPlayer";
    unsigned char verbosityLevel = 0;
    portNumBits tunnelOverHTTPPortNum = 0;
    int simpleRTPoffsetArg = -1;

    watchVariable = 0;

	rtspClient = ourRTSPClient::createNew(*env, url, verbosityLevel, applicationName, 0, NULL);
	if (rtspClient == NULL) {
		LOGI("Failed to create a RTSP client for URL %s storename %s, %s \n" , url, env->getResultMsg() );
		return -1;
	}

	//rtspClient->sendDescribeCommand(continueAfterDESCRIBE);
    return 1;
}

int ipcam_rtsp_play::StartRecv()
{
	if (!pthread_create(&rtsp_thread, NULL, StartPlay, this))
    {
        return 1;
    }
    else
    {
        return -1;
    }
}

int ipcam_rtsp::Close()
{
	LOGD(   "ipcam_rtsp::Close(): Entering.\n");
    // Set watchVariable to end doEventLoop
    watchVariable = 10;
    usleep(50000);
    watchVariable = 10;
    usleep(50000);
    watchVariable = 10;
    usleep(50000);
    watchVariable = 10;

    // Close RingBuf Sink:
    CloseMediaSinks();

    Medium::close(session);
    Medium::close(rtspClient);

	LOGD(   "ipcam_rtsp::Close(): Leaving.\n");
    return 1;
}

void ipcam_rtsp_play::CloseMediaSinks()
{
    MediaSubsession* subsession;
    
	if (session)
	{
		MediaSubsessionIterator iter(*session);
		while ((subsession = iter.next()) != NULL)
		{
			Medium::close(subsession->sink);
			subsession->sink = NULL;
		}
	}
}

void* StartPlay(void* arg)
{
    ipcam_rtsp *pSHRtspRx = (ipcam_rtsp*)arg;
    RTSPClient *rtspClient = (RTSPClient*)pSHRtspRx->rtspClient;
    MediaSession* session = pSHRtspRx->session;
    UsageEnvironment* env = pSHRtspRx->env;

    env->taskScheduler().doEventLoop(&(pSHRtspRx->watchVariable)); // does not return

    return NULL;
}

void subsessionAfterPlaying(void* clientData)
{
	fprintf(stderr,"+Enter %s",__func__);
    // Begin by closing this media subsession's stream:
    MediaSubsession* subsession = (MediaSubsession*)clientData;
    Medium::close(subsession->sink);
    subsession->sink = NULL;

    // Next, check whether *all* subsessions' streams have now been closed:
    MediaSession& session = subsession->parentSession();
    MediaSubsessionIterator iter(session);
    while ((subsession = iter.next()) != NULL)
    {
        if (subsession->sink != NULL)
        {
            return; // this subsession is still active
        }
    }
    fprintf(stderr,"-Exit %s",__func__);
}

void subsessionByeHandler(void* clientData)
{
    MediaSubsession* subsession = (MediaSubsession*)clientData;
    // Act now as if the subsession had closed:
    subsessionAfterPlaying (subsession);
}


