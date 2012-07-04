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

void rec_subsessionAfterPlaying(void* clientData);
void rec_subsessionByeHandler(void* clientData);
void* rec_StartPlay(void* arg);

ipcam_rtsp_rec::ipcam_rtsp_rec()
{
    TaskScheduler* scheduler = BasicTaskScheduler::createNew();
    env = BasicUsageEnvironment::createNew(*scheduler);

    rtspClient = NULL;
    session = NULL;

    watchVariable = 0;

	filename = NULL;
	fps = 15;
	qtOut = NULL;
}

ipcam_rtsp_rec::~ipcam_rtsp_rec()
{

}

int ipcam_rtsp_rec::Init(char *url, char* fname, int fps)
{
	fprintf (stderr, "ipcam_rtsp::Init- Entering. \n");
	char const* applicationName = "RTSPlayer";
	unsigned char verbosityLevel = 0;
	portNumBits tunnelOverHTTPPortNum = 0;
	int simpleRTPoffsetArg = -1;
	
	LOGD("fname from UI ***** %s", fname);

	this->filename = fname;

	watchVariable = 0;

	rtspClient = RTSPClient::createNew(*env, url, verbosityLevel, applicationName, tunnelOverHTTPPortNum);
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
				   subsession->mediumName(),subsession->codecName(), env->getResultMsg());
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

	// Create QtFileSink here
	unsigned fileSinkBufferSize = 100000;
	madeProgress = False;
	iter.reset();
	
	if (this->filename == NULL)
	{
		char f[] = "/mnt/sdcard/qtcam.mp4";
		this->filename = f;
		LOGD("Using default filename %s", filename);
	}
	
	Boolean packetLossCompensate = False;
	Boolean syncStreams = False;
	Boolean generateHintTracks = False;
	Boolean generateMP4Format = True;

	if (!fps)
		fps = fVideoFPS;
	
	// 	Create a "QuickTimeFileSink", to write to 'stdout':
	qtOut = QuickTimeFileSink::createNew(*env, *session, filename,
									fileSinkBufferSize,
									fVideoWidth, fVideoHeight,
									fps,
									packetLossCompensate,
									syncStreams,
									generateHintTracks,
									generateMP4Format);

	LOGD("Output File is %s", filename);

	if (qtOut == NULL) {
		LOGE("Failed to create QuickTime file sink for %s", filename);
		*env << "Failed to create QuickTime file sink for stdout: " << env->getResultMsg();
		Close ();
	}

	qtOut->startPlaying(rec_subsessionAfterPlaying, NULL);


	fprintf(stderr,  "[MSH] ipcam_rtsp::Init_Rec(): Leaving.\n");
	return 1;
}

int ipcam_rtsp_rec::StartRecv()
{
    if (!pthread_create(&rtsp_thread, NULL, rec_StartPlay, this))
    {
        return 1;
    }
    else
    {
        return -1;
    }
}

bool ipcam_rtsp_rec::SetupStreams()
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
				   subsession->codecName(), env->getResultMsg());
		}
		else
		{
			LOGV(  "Setup \"%s/%s\" subsession (client ports %d-%d\n",subsession->mediumName(),
				   subsession->codecName(),subsession->clientPortNum(),subsession->clientPortNum()+1);

			// for video subsession initiate video params
			if (strcmp(subsession->mediumName(), "video") == 0)
			{
				fVideoHeight = subsession->videoHeight();
				if(fVideoHeight == 0){
					fVideoHeight = 242;
					LOGV(  "fVideoHeight guessed %d\n", fVideoHeight);
				}
				fVideoWidth = subsession->videoWidth();
				if(fVideoWidth == 0){
					fVideoWidth = 192;
					LOGV(   "fVideoWidth guessed %d\n", fVideoWidth);
				}
				fVideoFPS = subsession->videoFPS();
				if(fVideoFPS == 0) {
					fVideoFPS = 15;
					LOGV(   "FPS guessed %d\n", fVideoFPS);
				}
				fCodecName = (char*) subsession->codecName();

				LOGV(  "Video Details: Size: %dx%d, FPS: %d, Codec:%s \n",
				fVideoWidth, fVideoHeight, fVideoFPS, fCodecName);
			}
			madeProgress = True;
		}
	}
	return madeProgress;
}

int ipcam_rtsp_rec::Close()
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

void ipcam_rtsp_rec::CloseMediaSinks()
{
	Medium::close(qtOut);
    MediaSubsession* subsession;
    
	if(session) {
		MediaSubsessionIterator iter(*session);
		while ((subsession = iter.next()) != NULL)
		{
			Medium::close(subsession->sink);
			subsession->sink = NULL;
		}
	}
}

bool ipcam_rtsp_rec::TearDownStreams()
{
	bool val = False;

	if (session) {
		((RTSPClient*)rtspClient)->teardownMediaSession(*session);
		val = True;
	}
	return val;
}

void* rec_StartPlay(void* arg)
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

void rec_subsessionAfterPlaying(void* clientData)
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

void rec_subsessionByeHandler(void* clientData)
{
    MediaSubsession* subsession = (MediaSubsession*)clientData;
    // Act now as if the subsession had closed:
    rec_subsessionAfterPlaying (subsession);
}
