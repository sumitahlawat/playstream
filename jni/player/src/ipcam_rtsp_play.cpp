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
    fprintf (stderr, "ipcam_rtsp::Init(): Entering. \n");
	pVideoBuffer = pCodecHRtspVideoBuffer;
	pAudioBuffer = pCodecHRtspAudioBuffer;
    char const* applicationName = "RTSPlayer";
    unsigned char verbosityLevel = 0;
    portNumBits tunnelOverHTTPPortNum = 0;
    int simpleRTPoffsetArg = -1;

    watchVariable = 0;

    rtspClient = RTSPClient::createNew(*env, verbosityLevel, applicationName, tunnelOverHTTPPortNum);
    if (rtspClient == NULL)
    {
        Close();
        return -1;
    }
    // Begin by sending an "OPTIONS" command:
    char* optionsResponse = ((RTSPClient*)rtspClient)->sendOptionsCmd(url);
    if (optionsResponse != NULL)
    {
        LOGV( " \"OPTIONS\" request returned: %s \n", optionsResponse);
    }
    delete [] optionsResponse;

    // Open the URL, to get a SDP description:
    char* sdpDescription = ((RTSPClient*)rtspClient)->describeURL(url);
    if (sdpDescription == NULL)
    {
        Close();
        return -1;
    }
    else
    {
		LOGV(  " Opened URL \" %s \", returning a SDP description:\n %s \n", url, sdpDescription);
    }

    // Create a media session object from this SDP description:
    session = MediaSession::createNew(*env, sdpDescription);
    delete[] sdpDescription;
    if (session == NULL)
    {
        fprintf (stderr, "Failed to create a MediaSession object from the SDP description: %s \n", env->getResultMsg());
        Close();
        return -1;
    }
    else if (!session->hasSubsessions ())
    {
		LOGE(  "This session has no media subsessions (i.e., \"m=\" lines)\n");
        Close();
        return -1;
    }

    // Then, setup the "RTPSource"s for the session:
    MediaSubsessionIterator iter(*session);
    MediaSubsession *subsession;
    Boolean madeProgress = False;

    while ((subsession = iter.next()) != NULL)
    {
        if (!subsession->initiate(simpleRTPoffsetArg))
        {
			LOGE(  "Unable to create receiver for \"%s/%s\" subsession: %s\n",
				   subsession->mediumName(),subsession->codecName(),env->getResultMsg());
        }
        else
        {
			LOGV(  "Created receiver for \"%s/%s\" subsession (client ports %d-%d)\n",
				   subsession->mediumName(),subsession->codecName(),
				   subsession->clientPortNum(),subsession->clientPortNum()+1);
            madeProgress = True;

            if (subsession->rtpSource() != NULL)
            {
                unsigned const thresh = 5000; // 5ms second
                subsession->rtpSource()->setPacketReorderingThresholdTime(thresh);
            }
        }
    }
    if (!madeProgress)
    {
        Close ();
        return -1;
    }

    // Perform additional 'setup' on each subsession, before playing them:
    if (!SetupStreams())
    {
        Close ();
        return -1;
    }

	// Create RingSink here
	madeProgress = False;
	iter.reset();
	unsigned fileSinkBufferSize = 100000;

	while ((subsession = iter.next ()) != NULL)
	{
		if (subsession->readSource () == NULL)
		{
			continue; // was not initiated
		}
		
		ipcam_ringsink* ringBufSink = NULL;
		if (strcmp(subsession->mediumName(), "video") == 0)
		{
			ringBufSink = ipcam_ringsink::createNew(*env, pVideoBuffer, fileSinkBufferSize);
			if(!ringBufSink) {
				LOGE( "ringBufSink is NULL --- Check this\n");
				exit(1);
			}
			fVideoHeight = subsession->videoHeight();
			if(fVideoHeight == 0){
				fVideoHeight = 240;
				//fprintf(stderr, "fVideoHeight guessed %d\n", fVideoHeight);
			}
			fVideoWidth = subsession->videoWidth();
			if(fVideoWidth == 0){
				fVideoWidth = 320;
				//fprintf(stderr, "fVideoWidth guessed %d\n", fVideoWidth);
			}
			fVideoFPS = subsession->videoFPS();
			if(fVideoFPS == 0) {
				fVideoFPS = 15;
				//fprintf(stderr, "FPS guessed %d\n", fVideoFPS);
			}
			fCodecName = (char*) subsession->codecName();
			
			LOGV("Video Details: Size: %dx%d, FPS: %d, Codec:%s \n",
					fVideoWidth, fVideoHeight, fVideoFPS, fCodecName);
		}
		else if (strcmp(subsession->mediumName(), "audio") == 0)
		{
			ringBufSink = ipcam_ringsink::createNew(*env, pAudioBuffer, fileSinkBufferSize);
		}
		
		subsession->sink = ringBufSink;
		if (subsession->sink == NULL)
		{
			LOGE ("Failed to create ipcam_ringsink for \"%s\": %s\n",
					 subsession->mediumName(),env->getResultMsg());
		}
		else
		{
			if(strcmp(subsession->codecName(), "MP4V-ES") == 0 &&
				subsession->fmtp_config() != NULL) {
					// For MPEG-4 video RTP streams, the 'config' information
					// from the SDP description contains useful VOL etc. headers.
					// Insert this data at the front of the output file:
					unsigned configLen;
					unsigned char* configData
					= parseGeneralConfigStr(subsession->fmtp_config(), configLen);
					struct timeval timeNow;
					gettimeofday(&timeNow, NULL);
					ringBufSink->addData(configData, configLen, timeNow);
					delete[] configData;
				}
				subsession->sink->startPlaying(*(subsession->readSource()),
											   subsessionAfterPlaying, subsession);
				
				if (subsession->rtcpInstance() != NULL)
				{
					subsession->rtcpInstance()->setByeHandler(subsessionByeHandler, subsession);
				}
				madeProgress = True;
		}
	}
	if (!madeProgress)
	{
		Close ();
		return -1;
	}

	LOGV(  "[MSH] ipcam_rtsp::Init(): Leaving.\n");
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

bool ipcam_rtsp_play::SetupStreams ()
{
    MediaSubsessionIterator iter(*session);
    MediaSubsession *subsession;
    bool madeProgress = False;
    bool streamUsingTCP = False;

    while ((subsession = iter.next ()) != NULL)
    {
        if (subsession->clientPortNum () == 0)
        {
            continue; // port # was not set
        }

        if (!((RTSPClient*)rtspClient)->setupMediaSubsession (*subsession, False, streamUsingTCP))
        {
			LOGE(  "Failed to setup \"%s/%s\" subsession: %s\n",subsession->mediumName(),
				   subsession->codecName(),env->getResultMsg());
        }
        else
        {
			LOGV(  "Setup \"%s/%s\" subsession (client ports %d-%d\n",subsession->mediumName(),
				   subsession->codecName(),subsession->clientPortNum(),subsession->clientPortNum()+1);

            madeProgress = True;
        }
    }
    return madeProgress;
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

    // Teardown, then shutdown, any outstanding RTP/RTCP subsessions
    if (!TearDownStreams())
    {
        return -1;
    }
    else
    {
		LOGV(   "Teardown completed\n");
    }
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

bool ipcam_rtsp_play::TearDownStreams()
{
	bool val = False;
    if (session) {
		((RTSPClient*)rtspClient)->teardownMediaSession(*session);
		val = True;
	}
	return val;
}

void* StartPlay(void* arg)
{
    ipcam_rtsp *pSHRtspRx = (ipcam_rtsp*)arg;
    RTSPClient *rtspClient = (RTSPClient*)pSHRtspRx->rtspClient;
    MediaSession* session = pSHRtspRx->session;
    UsageEnvironment* env = pSHRtspRx->env;

    // Start playing streams
    if (!rtspClient->playMediaSession(*session))
    {
		LOGD(   "Failed to start playing session: %s\n",env->getResultMsg());
        pSHRtspRx->Close();
    }
    else
    {
		LOGV(   "Started playing session watch %d ..... \n",
			pSHRtspRx->watchVariable);
    }

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


