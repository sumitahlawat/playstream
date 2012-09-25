#ifndef _IPCAM_RTSP_H_
#define _IPCAM_RTSP_H_

#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "liveMedia.hh"
#include "ipcam_vdec.h"
#include <pthread.h>

class StreamClientState;
class ourRTSPClient;
class playRTSPClient;

class ipcam_rtsp
{
public:
	unsigned short fVideoHeight;
	unsigned short fVideoWidth;
	unsigned fVideoFPS;
	char* fCodecName;
	char watchVariable;    ///< a flag to stop doEventLoop() set to nonzero will return from doEventLoop()

public:
	virtual int StartRecv ()=0;
	virtual int Close ()=0;

	unsigned short videoWidth() const { return fVideoWidth; }
	unsigned short videoHeight() const { return fVideoHeight; }
	unsigned videoFPS() const { return fVideoFPS; }
	char const* codecName() const { return fCodecName; }

	virtual ~ipcam_rtsp() { };

public:
	pthread_t rtsp_thread; ///< the thread hanlder of created RTSP thread
	UsageEnvironment* env; ///< Specify the environment parameters
};

class ipcam_rtsp_rec :  public ipcam_rtsp
{
public:
	char* filename;
	int fps;

public:
	int StartRecv ();
	int Close ();

	int Init(char *url, char* filename, int fps);
	ourRTSPClient* rtspClient;
	ipcam_rtsp_rec ();
	~ipcam_rtsp_rec ();
};

class ipcam_rtsp_play :  public ipcam_rtsp
{
public:
	int StartRecv ();
	int Close ();

	playRTSPClient* rtspClient;
	ipcam_vdec *decoder;
	int Init (char *url);
	ipcam_rtsp_play (int id);
	~ipcam_rtsp_play ();
	int idx;
};

// Define a class to hold per-stream state that we maintain throughout each stream's lifetime:
class StreamClientState {
public:
	StreamClientState();
	virtual ~StreamClientState();

public:
	MediaSubsessionIterator* iter;
	MediaSession* session;
	MediaSubsession* subsession;
	TaskToken streamTimerTask;
	double duration;
};

class playRTSPClient: public RTSPClient {
public:
	static playRTSPClient* createNew(UsageEnvironment& env, char const* rtspURL,
			int verbosityLevel = 0,
			char const* applicationName = NULL,
			portNumBits tunnelOverHTTPPortNum = 0,
			int camidx = 1 );

protected:
	playRTSPClient(UsageEnvironment& env, char const* rtspURL,
			int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum, int camidx);
	// called only by createNew();
	virtual ~playRTSPClient();

public:
	StreamClientState scs;
	int camidx;
};

class ourRTSPClient: public RTSPClient {
public:
	static ourRTSPClient* createNew(UsageEnvironment& env, char const* rtspURL,
			int verbosityLevel = 0,
			char const* applicationName = NULL,
			portNumBits tunnelOverHTTPPortNum = 0,
			char const* filename = NULL);

protected:
	ourRTSPClient(UsageEnvironment& env, char const* rtspURL,
			int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum, char const* filename);
	// called only by createNew();
	virtual ~ourRTSPClient();

public:
	char fname[200];
	StreamClientState scs;
	QuickTimeFileSink* qtOut;
};

//temp class to try direct decoding, instead of using ringbuffer

class DecoderSink: public MediaSink {
public:
	static DecoderSink* createNew(UsageEnvironment& env,
			MediaSubsession& subsession, // identifies the kind of data that's being received
			char const* streamId = NULL,// identifies the stream itself (optional)
			int camidx = 1   //to identify camera number
			);

private:
	DecoderSink(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId, int camidx);
	virtual ~DecoderSink();

	static void afterGettingFrame(void* clientData, unsigned frameSize,
			unsigned numTruncatedBytes,
			struct timeval presentationTime,
			unsigned durationInMicroseconds);
	void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
			struct timeval presentationTime, unsigned durationInMicroseconds);
private:
	// redefined virtual functions:
	virtual Boolean continuePlaying();

private:
	u_int8_t* fReceiveBuffer;
	MediaSubsession& fSubsession;
	char* fStreamId;
	int camidx;
};
#endif // _IPCAM_RTSP_H_
