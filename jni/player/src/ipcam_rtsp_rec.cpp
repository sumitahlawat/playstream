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

unsigned fileSinkBufferSize = 100000;
unsigned short movieWidth = 320; // default
unsigned short movieHeight = 240; // default
unsigned movieFPS = 15; // default
Boolean packetLossCompensate = False;
Boolean syncStreams = False;
Boolean generateHintTracks = False;
Boolean generateMP4Format = True;

TaskToken interPacketGapCheckTimerTask = NULL;
unsigned interPacketGapMaxTime = 10;
unsigned totNumPacketsReceived = ~0; // used if checking inter-packet gaps


void rec_subsessionAfterPlaying(void* clientData);
void rec_subsessionByeHandler(void* clientData);
void* rec_StartPlay(void* arg);

// Forward function definitions:

// RTSP 'response handlers':
void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString);
void recsubsessionAfterPlaying(void* clientData); // called when a stream's subsession (e.g., audio or video substream) ends
void sessionAfterPlaying(void* clientData = NULL);

void recsubsessionByeHandler(void* clientData); // called when a RTCP "BYE" is received for a subsession
void streamTimerHandler(void* clientData);
void checkInterPacketGaps(void* clientData);
// called at the end of a stream's expected duration (if the stream has not already signaled its end using a RTCP "BYE")

// Used to iterate through each stream's 'subsessions', setting up each one:
void setupNextSubsession(RTSPClient* rtspClient);

// Used to shut down and close a stream (including its "RTSPClient" object):
void shutdownStream(RTSPClient* rtspClient, int exitCode = 1);

// A function that outputs a string that identifies each stream (for debugging output).  Modify this if you wish:
UsageEnvironment& operator<<(UsageEnvironment& env, const RTSPClient& rtspClient) {
	return env << "[URL:\"" << rtspClient.url() << "\"]: ";
}

// A function that outputs a string that identifies each subsession (for debugging output).  Modify this if you wish:
UsageEnvironment& operator<<(UsageEnvironment& env, const MediaSubsession& subsession) {
	return env << subsession.mediumName() << "/" << subsession.codecName();
}

// Implementation of the RTSP 'response handlers':

void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString) {
	do {
		UsageEnvironment& env = rtspClient->envir(); // alias
		StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

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
		setupNextSubsession(rtspClient);
		return;
	} while (0);

	// An unrecoverable error occurred with this stream.
	shutdownStream(rtspClient);
}

void setupNextSubsession(RTSPClient* rtspClient) {
	UsageEnvironment& env = rtspClient->envir(); // alias
	StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

	scs.subsession = scs.iter->next();
	if (scs.subsession != NULL) {
		if (!scs.subsession->initiate()) {
			LOGI("Failed to initiate the subsession : %s  \n",  env.getResultMsg() );
			env << *rtspClient << "Failed to initiate the \"" << *scs.subsession << "\" subsession: " << env.getResultMsg() << "\n";
			setupNextSubsession(rtspClient); // give up on this subsession; go to the next one
		} else {
			LOGI("subsession initiated : %d  \n",  scs.subsession->clientPortNum());
			env << *rtspClient << "Initiated the \"" << *scs.subsession
					<< "\" subsession (client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum()+1 << ")\n";

			// Continue setting up this subsession, by sending a RTSP "SETUP" command:
			rtspClient->sendSetupCommand(*scs.subsession, continueAfterSETUP);
		}
		return;
	}

	// We've finished setting up all of the subsessions.  Now, send a RTSP "PLAY" command to start the streaming:
	scs.duration = scs.session->playEndTime() - scs.session->playStartTime();
	rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY);
}

void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString) {
	do {
		UsageEnvironment& env = rtspClient->envir(); // alias
		StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

		if (resultCode != 0) {
			LOGI("failed to setup subsession \n");
			env << *rtspClient << "Failed to set up the \"" << *scs.subsession << "\" subsession: " << env.getResultMsg() << "\n";
			break;
		}

		LOGI("Set up the subsession : before sink create : \n");
		LOGI("Set up the subsession : before sink create : %s\n",((ourRTSPClient*)rtspClient)->fname);
		env << *rtspClient << "Set up the \"" << *scs.subsession
				<< "\" subsession (client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum()+1 << ")\n";

		// Having successfully setup the subsession, create a data sink for it, and call "startPlaying()" on it.
		// (This will prepare the data sink to receive data; the actual flow of data from the client won't start happening until later,
		// after we've sent a RTSP "PLAY" command.)


		// Create a "QuickTimeFileSink", to write to 'stdout': -- can be changed to filename
		((ourRTSPClient*)rtspClient)->qtOut = QuickTimeFileSink::createNew(env, *(scs.session), ((ourRTSPClient*)rtspClient)->fname,
				fileSinkBufferSize,
				movieWidth, movieHeight,
				movieFPS,
				packetLossCompensate,
				syncStreams,
				generateHintTracks,
				generateMP4Format);
		if (((ourRTSPClient*)rtspClient)->qtOut == NULL) {
			LOGI("Failed to create QuickTime file sink for stdout: %s \n", env.getResultMsg());
			break;
		}

		((ourRTSPClient*)rtspClient)->qtOut->startPlaying(sessionAfterPlaying, NULL);

		// Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
		if (scs.subsession->rtcpInstance() != NULL) {
			scs.subsession->rtcpInstance()->setByeHandler(recsubsessionByeHandler, scs.subsession);
		}
	} while (0);

	// Set up the next subsession, if any:
	setupNextSubsession(rtspClient);
}

void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString) {
	do {
		UsageEnvironment& env = rtspClient->envir(); // alias
		StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

		if (resultCode != 0) {
			LOGI( "Failed to start playing session: %s \n" ,resultString);
			break;
		}

		// Set a timer to be handled at the end of the stream's expected duration (if the stream does not already signal its end
		// using a RTCP "BYE").  This is optional.  If, instead, you want to keep the stream active - e.g., so you can later
		// 'seek' back within it and do another RTSP "PLAY" - then you can omit this code.
		// (Alternatively, if you don't want to receive the entire stream, you could set this timer for some shorter value.)
		if (scs.duration > 0) {
			unsigned const delaySlop = 2; // number of seconds extra to delay, after the stream's expected duration.  (This is optional.)
			scs.duration += delaySlop;
			unsigned uSecsToDelay = (unsigned)(scs.duration*1000000);
			scs.streamTimerTask = env.taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*)streamTimerHandler, rtspClient);
		}

		LOGI( "Started playing session");
		if (scs.duration > 0) {
			LOGI(" (for up to %f seconds )", scs.duration);
		}
		// Watch for incoming packets (if desired):
		//checkForPacketArrival(NULL);
		totNumPacketsReceived = ~0;
		checkInterPacketGaps(rtspClient);

		LOGI("...\n");

		return;
	} while (0);

	// An unrecoverable error occurred with this stream.
	shutdownStream(rtspClient);
}

// Implementation of the other event handlers:

void sessionAfterPlaying(void* clientData) {
	ourRTSPClient* rtspClient = (ourRTSPClient*)clientData;
	StreamClientState& scs = rtspClient->scs; // alias

	scs.streamTimerTask = NULL;
	LOGI("sessionAfterPlaying \n");
	// Shut down the stream:
	shutdownStream(rtspClient);
}

void recsubsessionAfterPlaying(void* clientData) {
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
	shutdownStream(rtspClient);
}

void recsubsessionByeHandler(void* clientData) {
	MediaSubsession* subsession = (MediaSubsession*)clientData;
	RTSPClient* rtspClient = (RTSPClient*)subsession->miscPtr;
	UsageEnvironment& env = rtspClient->envir(); // alias

	env << *rtspClient << "Received RTCP \"BYE\" on \"" << *subsession << "\" subsession\n";
	LOGI("Received RTCP \"BYE\" on subsession \n" );

	// Now act as if the subsession had closed:
	recsubsessionAfterPlaying(subsession);
}

void streamTimerHandler(void* clientData) {
	ourRTSPClient* rtspClient = (ourRTSPClient*)clientData;
	StreamClientState& scs = rtspClient->scs; // alias

	scs.streamTimerTask = NULL;
	LOGI("time out on subsession \n" );
	// Shut down the stream:
	shutdownStream(rtspClient);
}

void shutdownStream(RTSPClient* rtspClient, int exitCode) {
	UsageEnvironment& env = rtspClient->envir(); // alias
	StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias
	int clientid=0;
	if (strstr(((ourRTSPClient*)rtspClient)->fname,"ipcam1")!=NULL)	{
		clientid=1;
		//trec1->fEventLoopStopFlag = ~0;
	}

	if (strstr(((ourRTSPClient*)rtspClient)->fname,"ipcam2")!=NULL)	{
		clientid=2;
		//trec2->fEventLoopStopFlag = ~0;
	}

	if (strstr(((ourRTSPClient*)rtspClient)->fname,"ipcam3")!=NULL) {
		clientid=3;
		//trec3->fEventLoopStopFlag = ~0;
	}

	if (strstr(((ourRTSPClient*)rtspClient)->fname,"ipcam4")!=NULL) {
		clientid=4;
		//trec4->fEventLoopStopFlag = ~0;
	}
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
	Medium::close(((ourRTSPClient*)rtspClient)->qtOut);
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

void checkInterPacketGaps(void* clientData) {
	ourRTSPClient* rtspClient = (ourRTSPClient*)clientData;
	UsageEnvironment& env = rtspClient->envir(); // alias
	StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

	LOGI( "interPacketGapMaxTime:  %d \n",  interPacketGapMaxTime);

	if (interPacketGapMaxTime == 0) return; // we're not checking

	// Check each subsession, counting up how many packets have been received:
	unsigned newTotNumPacketsReceived = 0;

	// First, check whether any subsessions have still to be closed:
	MediaSubsessionIterator iter(*scs.session);
	MediaSubsession* subsession;
	while ((subsession = iter.next()) != NULL) {
		if (subsession->sink != NULL) {
			RTPSource* src = subsession->rtpSource();
			LOGI( "pkt received:  %d \n",  src->receptionStatsDB().totNumPacketsReceived());
			if (src == NULL) continue;
			newTotNumPacketsReceived += src->receptionStatsDB().totNumPacketsReceived();
		}
	}

	LOGI( "Closing session, because we stopped receiving packets totNumPacketsReceived:  %d  and  newTotNumPacketsReceived : %d.\n", totNumPacketsReceived, newTotNumPacketsReceived);

	if (newTotNumPacketsReceived == totNumPacketsReceived) {
		// No additional packets have been received since the last time we
		// checked, so end this stream:
		env << "Closing session, because we stopped receiving packets.\n";
		interPacketGapCheckTimerTask = NULL;
		sessionAfterPlaying(rtspClient);
	} else {
		//totNumPacketsReceived = newTotNumPacketsReceived;
		// Check again, after the specified delay:
		interPacketGapCheckTimerTask
		= env.taskScheduler().scheduleDelayedTask(interPacketGapMaxTime*1000000,
				(TaskFunc*)checkInterPacketGaps, rtspClient);
	}
}

ipcam_rtsp_rec::ipcam_rtsp_rec()
{
	TaskScheduler* scheduler = BasicTaskScheduler::createNew();
	env = BasicUsageEnvironment::createNew(*scheduler);

	rtspClient = NULL;

	watchVariable = 0;

	filename = NULL;
	fps = 15;
}

ipcam_rtsp_rec::~ipcam_rtsp_rec()
{

}

int ipcam_rtsp_rec::Init(char *url, char* fname, int fps)
{
	char const* applicationName = "RTSPlayer";
	unsigned char verbosityLevel = 0;
	portNumBits tunnelOverHTTPPortNum = 0;
	int simpleRTPoffsetArg = -1;

	LOGD("fname from UI ***** %s", fname);

	watchVariable = 0;

	this->filename = fname;
//	rtspClient = ourRTSPClient::createNew(*env, url, verbosityLevel, applicationName, 0, fname);
//	if (rtspClient == NULL) {
//		LOGI("Failed to create a RTSP client for URL %s storename %s, %s \n" , url, fname,  env->getResultMsg() );
//		return -1;
//	}

	LOGI( "ipcam_rtsp::Init_Rec(): Leaving.\n");
	return 1;
}

int ipcam_rtsp_rec::StartRecv()
{
	LOGI( "ipcam_rtsp_rec : recording thread start\n");
	if (!pthread_create(&rtsp_thread, NULL, rec_StartPlay, this))
	{
		return 1;
	}
	else
	{
		return -1;
	}
}

int ipcam_rtsp_rec::Close()
{
	LOGD(   "ipcam_rtsp::Close(): Entering.\n");
	shutdownStream(rtspClient);
	LOGD("ipcam_rtsp::Close(): Leaving.\n");
	return 1;
}

void ipcam_rtsp_rec::CloseMediaSinks()
{

}

void* rec_StartPlay(void* arg)
{
    ipcam_rtsp_rec *pSHRtspRx = (ipcam_rtsp_rec*)arg;
    ourRTSPClient *rtspClient = (ourRTSPClient*)pSHRtspRx->rtspClient;
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias
    MediaSession* session = scs.session; // alias
    UsageEnvironment* env = pSHRtspRx->env;

    LOGI( "\t rec begins now\n");
	rtspClient = ourRTSPClient::createNew(*env, "rtsp://ahlawat.servehttp.com/live.sdp", 0, "RTSPLayer", 0, "/mnt/sdcard/rec.mov");
	if (rtspClient == NULL) {
		LOGI("Failed to create a RTSP client for : %s \n" ,  env->getResultMsg() );
		return NULL;
	}

	rtspClient->sendDescribeCommand(continueAfterDESCRIBE);
    env->taskScheduler().doEventLoop(&(pSHRtspRx->watchVariable)); // does not return

	LOGI( "\t checking program return\n");
	shutdownStream(rtspClient);
	return NULL;
}


// Implementation of "ourRTSPClient":
ourRTSPClient* ourRTSPClient::createNew(UsageEnvironment& env, char const* rtspURL,
		int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum, char const* filename) {
	return new ourRTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, filename);
}

ourRTSPClient::ourRTSPClient(UsageEnvironment& env, char const* rtspURL,
		int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum, char const* filename)
: RTSPClient(env,rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum) {
	strcpy(this->fname, filename);
	fname[strlen(filename)]='\0';
	qtOut=NULL;
	//	this->fname=filename;
	LOGI( "ourRTSPClient created\n");
}

ourRTSPClient::~ourRTSPClient()
{
	LOGI("~ourRTSPClient ");
}


// Implementation of "StreamClientState":

StreamClientState::StreamClientState()
: iter(NULL), session(NULL), subsession(NULL), streamTimerTask(NULL), duration(0.0) {
}

StreamClientState::~StreamClientState() {
	delete iter;
	if (session != NULL) {
		// We also need to delete "session", and unschedule "streamTimerTask" (if set)
		UsageEnvironment& env = session->envir(); // alias

		env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
		Medium::close(session);
	}
}
