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


void* StartPlay(void* arg);


// RTSP 'response handlers':
void playcontinueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString);
void playcontinueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString);
void playcontinueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString);
void playsubsessionAfterPlaying(void* clientData); // called when a stream's subsession (e.g., audio or video substream) ends
void playsessionAfterPlaying(void* clientData = NULL);

void playsubsessionByeHandler(void* clientData); // called when a RTCP "BYE" is received for a subsession
void playstreamTimerHandler(void* clientData);
// called at the end of a stream's expected duration (if the stream has not already signaled its end using a RTCP "BYE")

// Used to iterate through each stream's 'subsessions', setting up each one:
void playsetupNextSubsession(RTSPClient* rtspClient);

// Used to shut down and close a stream (including its "RTSPClient" object):
void playshutdownStream(RTSPClient* rtspClient, int exitCode = 1);

// Implementation of the RTSP 'response handlers':

void playcontinueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString) {
	do {
		UsageEnvironment& env = rtspClient->envir(); // alias
		StreamClientState& scs = ((playRTSPClient*)rtspClient)->scs; // alias

		if (resultCode != 0) {
			LOGI("Failed to get a SDP description: %s \n", resultString);
			break;
		}

		char* sdpDescription = resultString;
		LOGI("Got a SDP description: \n %s \n", sdpDescription);

		// Create a media session object from this SDP description:
		scs.session = MediaSession::createNew(env, sdpDescription);
		delete[] sdpDescription; // because we don't need it anymore
		if (scs.session == NULL) {
			LOGI( "Failed to create a MediaSession object from the SDP description: %s \n" ,env.getResultMsg());
			break;
		} else if (!scs.session->hasSubsessions()) {
			LOGI("This session has no media subsessions (i.e., no \"m=\" lines)\n");
			break;
		}

		// Then, create and set up our data source objects for the session.  We do this by iterating over the session's 'subsessions',
		// calling "MediaSubsession::initiate()", and then sending a RTSP "SETUP" command, on each one.
		// (Each 'subsession' will have its own data source.)
		scs.iter = new MediaSubsessionIterator(*scs.session);
		playsetupNextSubsession(rtspClient);
		return;
	} while (0);

	// An unrecoverable error occurred with this stream.
	playshutdownStream(rtspClient);
}

void playsetupNextSubsession(RTSPClient* rtspClient) {
	UsageEnvironment& env = rtspClient->envir(); // alias
	StreamClientState& scs = ((playRTSPClient*)rtspClient)->scs; // alias

	scs.subsession = scs.iter->next();
	if (scs.subsession != NULL) {
		if (!scs.subsession->initiate()) {
			LOGI("Failed to initiate the subsession : %s  \n",  env.getResultMsg() );
			playsetupNextSubsession(rtspClient); // give up on this subsession; go to the next one
		} else {
			LOGI("subsession initiated : %d  - %d codec :  %s\n",  scs.subsession->clientPortNum() , scs.subsession->clientPortNum()+1 , scs.subsession->codecName());
			LOGI(" subsession info : %s  \n",  scs.subsession->mediumName() );
			// Continue setting up this subsession, by sending a RTSP "SETUP" command:
			//	if (strcmp(scs.subsession->mediumName(), "video") == 0)
			rtspClient->sendSetupCommand(*scs.subsession, playcontinueAfterSETUP);

		}
		return;
	}

	// We've finished setting up all of the subsessions.  Now, send a RTSP "PLAY" command to start the streaming:
	scs.duration = scs.session->playEndTime() - scs.session->playStartTime();
	rtspClient->sendPlayCommand(*scs.session, playcontinueAfterPLAY);
}

void playcontinueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString) {
	do {
		UsageEnvironment& env = rtspClient->envir(); // alias
		StreamClientState& scs = ((playRTSPClient*)rtspClient)->scs; // alias

		unsigned short fVideoHeight;
		unsigned short fVideoWidth;
		unsigned fVideoFPS;
		char* fCodecName;
		unsigned fileSinkBufferSize = 100000;

		if (resultCode != 0) {
			LOGI("failed to setup subsession \n");
			//env << *rtspClient << "Failed to set up the \"" << *scs.subsession << "\" subsession: " << env.getResultMsg() << "\n";
			break;
		}

		LOGI("Set up the subsession : before sink create \n");
		ipcam_ringsink* ringBufSink = NULL;
		ringBufSink = ipcam_ringsink::createNew(env, ((playRTSPClient*)rtspClient)->vbuffer, fileSinkBufferSize);
		if(!ringBufSink) {
			LOGI( "ringBufSink is NULL --- Check this\n");
			exit(1);
		}
		fVideoHeight = scs.subsession->videoHeight();
		if(fVideoHeight == 0){
			fVideoHeight = 240;
		}
		fVideoWidth = scs.subsession->videoWidth();
		if(fVideoWidth == 0){
			fVideoWidth = 320;
		}
		fVideoFPS = scs.subsession->videoFPS();
		if(fVideoFPS == 0) {
			fVideoFPS = 15;
		}
		fCodecName = (char*) scs.subsession->codecName();

		LOGI("Video Details: Size: %dx%d, FPS: %d, Codec:%s \n",
				fVideoWidth, fVideoHeight, fVideoFPS, fCodecName);

		scs.subsession->sink = ringBufSink;
		if (scs.subsession->sink == NULL)
		{
			LOGI ("Failed to create ipcam_ringsink for \"%s\": %s\n",
					scs.subsession->mediumName(),env.getResultMsg());
		}
		else
		{
			if(strcmp(scs.subsession->codecName(), "MP4V-ES") == 0 &&
					scs.subsession->fmtp_config() != NULL) {
				// For MPEG-4 video RTP streams, the 'config' information
				// from the SDP description contains useful VOL etc. headers.
				// Insert this data at the front of the output file:
				LOGI( "add mp4v data to ringbuf\n");
				unsigned configLen;
				unsigned char* configData
				= parseGeneralConfigStr(scs.subsession->fmtp_config(), configLen);
				struct timeval timeNow;
				gettimeofday(&timeNow, NULL);
				ringBufSink->addData(configData, configLen, timeNow);
				LOGI("Video Details configData : %s \n", configData);
				delete[] configData;
			}
			scs.subsession->sink->startPlaying(*(scs.subsession->readSource()), playsubsessionAfterPlaying, scs.subsession);

			// Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
			if (scs.subsession->rtcpInstance() != NULL) {
				scs.subsession->rtcpInstance()->setByeHandler(playsubsessionByeHandler, scs.subsession);
			}
		}

		/*
		while ((scs.subsession = scs.iter->next ()) != NULL)
		{
			if (scs.subsession->readSource () == NULL)
			{
				continue; // was not initiated
			}

			ipcam_ringsink* ringBufSink = NULL;
			if (strcmp(scs.subsession->mediumName(), "video") == 0)
			{
				ringBufSink = ipcam_ringsink::createNew(env, ((playRTSPClient*)rtspClient)->vbuffer, fileSinkBufferSize);
				if(!ringBufSink) {
					LOGE( "ringBufSink is NULL --- Check this\n");
					exit(1);
				}
				fVideoHeight = scs.subsession->videoHeight();
				if(fVideoHeight == 0){
					fVideoHeight = 240;
				}
				fVideoWidth = scs.subsession->videoWidth();
				if(fVideoWidth == 0){
					fVideoWidth = 320;
				}
				fVideoFPS = scs.subsession->videoFPS();
				if(fVideoFPS == 0) {
					fVideoFPS = 15;
				}
				fCodecName = (char*) scs.subsession->codecName();

				LOGI("Video Details: Size: %dx%d, FPS: %d, Codec:%s \n",
						fVideoWidth, fVideoHeight, fVideoFPS, fCodecName);
			}
			else if (strcmp(scs.subsession->mediumName(), "audio") == 0)
			{
				//ringBufSink = ipcam_ringsink::createNew(*env, pAudioBuffer, fileSinkBufferSize);
			}

			scs.subsession->sink = ringBufSink;
			if (scs.subsession->sink == NULL)
			{
				LOGE ("Failed to create ipcam_ringsink for \"%s\": %s\n",
						scs.subsession->mediumName(),env.getResultMsg());
			}
			else
			{
				if(strcmp(scs.subsession->codecName(), "MP4V-ES") == 0 &&
						scs.subsession->fmtp_config() != NULL) {
					// For MPEG-4 video RTP streams, the 'config' information
					// from the SDP description contains useful VOL etc. headers.
					// Insert this data at the front of the output file:
					unsigned configLen;
					unsigned char* configData
					= parseGeneralConfigStr(scs.subsession->fmtp_config(), configLen);
					struct timeval timeNow;
					gettimeofday(&timeNow, NULL);
					ringBufSink->addData(configData, configLen, timeNow);
					delete[] configData;
				}
				scs.subsession->sink->startPlaying(*(scs.subsession->readSource()), playsubsessionAfterPlaying, scs.subsession);

				// Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
				if (scs.subsession->rtcpInstance() != NULL) {
					scs.subsession->rtcpInstance()->setByeHandler(playsubsessionByeHandler, scs.subsession);
				}
			}
		}
		 */
	}while (0);

	// Set up the next subsession, if any:
	playsetupNextSubsession(rtspClient);
}

void playcontinueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString) {
	do {
		UsageEnvironment& env = rtspClient->envir(); // alias

		if (resultCode != 0) {
			LOGI( "Failed to start playing session: %s \n" ,resultString);
			break;
		}

		LOGI( "Started playing session");
		LOGI("...\n");

		return;
	} while (0);

	// An unrecoverable error occurred with this stream.
	playshutdownStream(rtspClient);
}


// Implementation of the other event handlers:

void playsessionAfterPlaying(void* clientData) {
	RTSPClient* rtspClient = (RTSPClient*)clientData;

	LOGI("sessionAfterPlaying \n");
	// Shut down the stream:
	playshutdownStream(rtspClient);
}

void playsubsessionAfterPlaying(void* clientData) {
	MediaSubsession* subsession = (MediaSubsession*)clientData;
	RTSPClient* rtspClient = (RTSPClient*)(subsession->miscPtr);

	LOGI("recsubsessionAfterPlaying \n\n");

	// Begin by closing this subsession's stream:
	Medium::close(subsession->sink);
	subsession->sink = NULL;

	// Next, check whether *all* subsessions' streams have now been closed:
	MediaSession& session = subsession->parentSession();
	MediaSubsessionIterator iter(session);
	while ((subsession = iter.next()) != NULL) {
		if (subsession->sink != NULL) return; // this subsession is still active
	}

	// All subsessions' streams have now been closed, so shutdown the client:
	playshutdownStream(rtspClient);
}

void playsubsessionByeHandler(void* clientData) {
	MediaSubsession* subsession = (MediaSubsession*)clientData;
	RTSPClient* rtspClient = (RTSPClient*)subsession->miscPtr;
	UsageEnvironment& env = rtspClient->envir(); // alias

	LOGI("Received RTCP \"BYE\" on subsession \n" );

	// Now act as if the subsession had closed:
	playsubsessionAfterPlaying(subsession);
}

void playstreamTimerHandler(void* clientData) {
	RTSPClient* rtspClient = (RTSPClient*)clientData;

	LOGI("time out on subsession \n" );
	// Shut down the stream:
	playshutdownStream(rtspClient);
}

void playshutdownStream(RTSPClient* rtspClient, int exitCode) {
	UsageEnvironment& env = rtspClient->envir(); // alias
	StreamClientState& scs = ((playRTSPClient*)rtspClient)->scs; // alias
	int clientid=0;

	// First, check whether any subsessions have still to be closed:
	if (scs.session != NULL) {
		Boolean someSubsessionsWereActive = False;
		MediaSubsessionIterator iter(*scs.session);
		MediaSubsession* subsession;

		while ((subsession = iter.next()) != NULL) {
			if (subsession->sink != NULL) {
				Medium::close(subsession->sink);
				subsession->sink = NULL;

				if (subsession->rtcpInstance() != NULL) {
					subsession->rtcpInstance()->setByeHandler(NULL, NULL); // in case the server sends a RTCP "BYE" while handling "TEARDOWN"
				}
				someSubsessionsWereActive = True;
			}
		}

		if (someSubsessionsWereActive) {
			// Send a RTSP "TEARDOWN" command, to tell the server to shutdown the stream.
			// Don't bother handling the response to the "TEARDOWN".
			LOGI( "Teardown session\n");
			//	rtspClient->sendTeardownCommand(*scs.session, NULL);
		}
	}

	LOGI( "Closing the stream :%d.\n",clientid);
	// Note that this will also cause this stream's "StreamClientState" structure to get reclaimed.


	LOGI( "this client closed.\n");
	//no need to exit
	//exit(exitCode);

	TaskScheduler* scheduler = &(env.taskScheduler());
	env.reclaim();
	usleep(5);
	//	delete scheduler;
	//	LOGI( "scheduler deleted\n");
	return;
}

ipcam_rtsp_play::ipcam_rtsp_play()
{
	TaskScheduler* scheduler = BasicTaskScheduler::createNew();
	env = BasicUsageEnvironment::createNew(*scheduler);
	rtspClient = NULL;

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


	rtspClient = playRTSPClient::createNew(*env,url, verbosityLevel, applicationName, tunnelOverHTTPPortNum, pVideoBuffer, pAudioBuffer);
	if (rtspClient == NULL) {
		LOGI("Failed to create a RTSP client for URL %s storename %s, %s \n" , url, env->getResultMsg() );
		return -1;
	}

	LOGI ("ipcam_rtsp::Init(): done  \n");
	return 1;
}

int ipcam_rtsp_play::StartRecv()
{
	LOGI( "ipcam_rtsp_play : play thread start\n");
	if (!pthread_create(&rtsp_thread, NULL, StartPlay, this))
	{
		return 1;
	}
	else
	{
		return -1;
	}
}

int ipcam_rtsp_play::Close()
{
	LOGD(   "ipcam_rtsp_play::Close(): Entering.\n");
	usleep(10);
	watchVariable = ~0;

	LOGD("ipcam_rtsp_play::Close(): Leaving.\n");
	return 1;
}

void* StartPlay(void* arg)
{
	ipcam_rtsp_play *pSHRtspRx = (ipcam_rtsp_play*)arg;
	playRTSPClient *rtspClient = (playRTSPClient*)pSHRtspRx->rtspClient;
	StreamClientState& scs = ((playRTSPClient*)rtspClient)->scs; // alias
	MediaSession* session = scs.session; // alias
	UsageEnvironment* env = pSHRtspRx->env;

	LOGI( "\t play begins now\n");

	if (rtspClient == NULL) {
		LOGI("Failed to create a RTSP client for : %s \n" ,  env->getResultMsg() );
		return NULL;
	}

	rtspClient->sendDescribeCommand(playcontinueAfterDESCRIBE);
	env->taskScheduler().doEventLoop(&(pSHRtspRx->watchVariable)); // does not return

	LOGI( "\t checking program return\n");
	playshutdownStream(rtspClient);

	return NULL;
}

// Implementation of "playRTSPClient":
playRTSPClient* playRTSPClient::createNew(UsageEnvironment& env, char const* rtspURL,
		int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum, ringbufferwriter *vbuffer, ringbufferwriter * abuffer) {
	return new playRTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, vbuffer, abuffer);
}

playRTSPClient::playRTSPClient(UsageEnvironment& env, char const* rtspURL,
		int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum, ringbufferwriter *vidbuffer, ringbufferwriter * audbuffer)
: RTSPClient(env,rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum) {
	vbuffer = vidbuffer;
	abuffer = audbuffer;
	LOGI( "playRTSPClient created\n");
}

playRTSPClient::~playRTSPClient()
{
	LOGI("~playRTSPClient ");
}
