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

/*ffmpeg headers*/
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/mathematics.h"
#include "libswscale/swscale.h"
}

AVCodec        *pCodec;
AVCodecContext *pContext;
AVFrame        *pFrame ,*out_pic;;
unsigned char  *picBuffer;
int 			picSize;
struct SwsContext* img_convert_ctx;

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

		if (resultCode != 0) {
			LOGI("failed to setup subsession \n");
			//env << *rtspClient << "Failed to set up the \"" << *scs.subsession << "\" subsession: " << env.getResultMsg() << "\n";
			break;
		}

		LOGI("Set up the subsession : before sink create \n");

		scs.subsession->sink = DecoderSink::createNew(env, *scs.subsession, "rtspURL");

		if (scs.subsession->sink == NULL) {
			LOGI( "Failed to create a data sink for the \" %d \" subsession: %s \n", *scs.subsession, env.getResultMsg());
			break;
		}

		scs.subsession->miscPtr = rtspClient; // a hack to let subsession handle functions get the "RTSPClient" from the subsession
		scs.subsession->sink->startPlaying(*(scs.subsession->readSource()),
				playsubsessionAfterPlaying, scs.subsession);
		// Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
		if (scs.subsession->rtcpInstance() != NULL) {
			scs.subsession->rtcpInstance()->setByeHandler(playsubsessionByeHandler, scs.subsession);
		}
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

		return;
	} while (0);

	// An unrecoverable error occurred with this stream.
	playshutdownStream(rtspClient);
}

// Implementation of the other event handlers:

void playsessionAfterPlaying(void* clientData) {
	RTSPClient* rtspClient = (RTSPClient*)clientData;

	LOGI("%s : %d \n",__func__,__LINE__);
	// Shut down the stream:
	playshutdownStream(rtspClient);
}

void playsubsessionAfterPlaying(void* clientData) {
	MediaSubsession* subsession = (MediaSubsession*)clientData;
	RTSPClient* rtspClient = (RTSPClient*)(subsession->miscPtr);

	LOGI("%s : %d \n",__func__,__LINE__);

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

int ipcam_rtsp_play::Init(char *url)
{
	LOGI ("ipcam_rtsp::Init(): Entering. \n");
	char const* applicationName = "RTSPlayer";
	unsigned char verbosityLevel = 0;
	portNumBits tunnelOverHTTPPortNum = 0;
	int simpleRTPoffsetArg = -1;

	watchVariable = 0;

	rtspClient = playRTSPClient::createNew(*env,url, verbosityLevel, applicationName, tunnelOverHTTPPortNum);
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
		int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum) {
	return new playRTSPClient(env, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum);
}

playRTSPClient::playRTSPClient(UsageEnvironment& env, char const* rtspURL,
		int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum)
: RTSPClient(env,rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum) {
	LOGI( "playRTSPClient created\n");
}

playRTSPClient::~playRTSPClient()
{
	LOGI("~playRTSPClient ");
}


// Implementation of "DecoderSink":
#define DECODER_SINK_RECEIVE_BUFFER_SIZE 100000

DecoderSink* DecoderSink::createNew(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId) {
	return new DecoderSink(env, subsession, streamId);
}

DecoderSink::DecoderSink(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId)
: MediaSink(env),
  fSubsession(subsession) {
	fStreamId = strDup(streamId);
	fReceiveBuffer = new u_int8_t[DECODER_SINK_RECEIVE_BUFFER_SIZE];
	avcodec_init();
	avcodec_register_all();
	pCodec = avcodec_find_decoder (CODEC_ID_MPEG4);
	if (!pCodec)
	{
		LOGI("Could not find codec to decode MPEG2 video\n");
	}

	pFrame = avcodec_alloc_frame();
	pContext = avcodec_alloc_context();
	pContext->bit_rate = 1000;  //temp value
	/* resolution must be a multiple of two */
	pContext->width =  320;   //temp value width
	pContext->height = 240;    //temp value height
	/* frames per second */
	pContext->time_base= (AVRational){1,25};
	pContext->pix_fmt = PIX_FMT_YUV420P;  //old
	pContext->pix_fmt = PIX_FMT_RGB24;    //for storing ppm's
	pContext->pix_fmt =PIX_FMT_RGB565;   //for glsurface

	//calculate picture size and allocate memory
	picSize = avpicture_get_size(pContext->pix_fmt, pContext->width, pContext->height);
	LOGI("Width %d, Height %d picSize %d\n", pContext->width, pContext->height, picSize);

	picBuffer = new uint8_t[picSize];

	if (pCodec->capabilities & CODEC_CAP_TRUNCATED)
	{
		//We do not send one total frame at one time
		pContext->flags |= CODEC_FLAG_TRUNCATED;
		pContext->flags |= CODEC_FLAG_EMU_EDGE;
	}

	if (avcodec_open (pContext, pCodec) < 0)
	{
		LOGI( "Could not open video codec\n");
	}
	LOGI("InitMPEG4Dec complete\n");
}

DecoderSink::~DecoderSink() {
	delete[] fReceiveBuffer;
	delete[] fStreamId;
}

void DecoderSink::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes,
		struct timeval presentationTime, unsigned durationInMicroseconds) {
	DecoderSink* sink = (DecoderSink*)clientData;
	sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

void DecoderSink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
		struct timeval presentationTime, unsigned /*durationInMicroseconds*/) {
	LOGI("%s : %d : \n",__func__,__LINE__);
	// We've just received a frame of data.  (Optionally) print out information about it:
	//	if (fStreamId != NULL) LOGI( "Stream :%s \n",fStreamId);
	//LOGI("%s  /  %s : :\tReceived %d bytes", fSubsession.mediumName(), fSubsession.codecName(), frameSize);
	if (strcmp(fSubsession.mediumName(), "video") == 0)
	{
		//decode data here
		DecVideo ((unsigned char*) fReceiveBuffer, (unsigned int) frameSize);
	}
	if (numTruncatedBytes > 0) envir() << " (with " << numTruncatedBytes << " bytes truncated)";
	char uSecsStr[6+1]; // used to output the 'microseconds' part of the presentation time
	sprintf(uSecsStr, "%06u", (unsigned)presentationTime.tv_usec);
	//	LOGI(".\tPresentation time: %d.%d",(unsigned)presentationTime.tv_sec, uSecsStr);
	if (fSubsession.rtpSource() != NULL && !fSubsession.rtpSource()->hasBeenSynchronizedUsingRTCP()) {
		//		LOGI("!"); // mark the debugging output to indicate that this presentation time is not RTCP-synchronized
	}
	// Then continue, to request the next frame of data:
	continuePlaying();
}

void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
	FILE *pFile;
	char szFilename[32];
	int  y;

	// Open file
	sprintf(szFilename, "/mnt/sdcard/ipcam1/frame%d.ppm", iFrame);
	pFile=fopen(szFilename, "wb");
	if(pFile==NULL)
		return;

	// Write header
	fprintf(pFile, "P6\n%d %d\n255\n", width, height);

	// Write pixel data
	for(y=0; y<height; y++)
		fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);

	fclose(pFile);
}

int savep = 0;

int DecoderSink::DecVideo(unsigned char* inBuffer, unsigned int bufferSize)
{
	int frame, gotPicture, len;
	//	char buf[1024];
	uint8_t *buf;
	int numBytes=avpicture_get_size(PIX_FMT_RGB565, pContext->width, pContext->height);
	buf=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));
	LOGI("%s : %d : bufferSize :%d\n",__func__,__LINE__,bufferSize);
	AVPacket avpkt;
	av_init_packet(&avpkt);

	avpkt.size = bufferSize;
	avpkt.data = inBuffer;

	frame = 0;
	while(avpkt.size > 0)
	{
		len = avcodec_decode_video2(pContext, pFrame, &gotPicture, &avpkt);
		if (len < 0) {
			LOGI("Error while decoding frame %d\n", frame);
			return -1;
		}
		if (gotPicture) {
			out_pic = avcodec_alloc_frame();
			if (!out_pic)
				return -1;
			img_convert_ctx = sws_getContext(pContext->width, pContext->height, pContext->pix_fmt,
					pContext->width, pContext->height, PIX_FMT_RGB565,SWS_BICUBIC, NULL, NULL, NULL);
			if (!img_convert_ctx)
				return -1;

			avpicture_fill((AVPicture *)out_pic, buf, PIX_FMT_RGB565, pContext->width, pContext->height);
			sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize, 0, pContext->height, out_pic->data, out_pic->linesize);
			sws_freeContext(img_convert_ctx);
			img_convert_ctx = NULL;
			savep++;
			//			if((savep%15)==0)
			//				SaveFrame(out_pic, pContext->width,	pContext->height, savep);

			//find a way to display now
		}
		avpkt.size -= len;
		avpkt.data += len;
		frame++;
	}
	return 0;
}

Boolean DecoderSink::continuePlaying() {
	if (fSource == NULL) return False; // sanity check (should not happen)

	// Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
	fSource->getNextFrame(fReceiveBuffer, DECODER_SINK_RECEIVE_BUFFER_SIZE,
			afterGettingFrame, this,
			onSourceClosure, this);
	return True;
}
