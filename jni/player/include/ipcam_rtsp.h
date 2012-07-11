
#ifndef _IPCAM_RTSP_H_
#define _IPCAM_RTSP_H_

#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "liveMedia.hh"

#include "ipcam_ringsink.h"

//#include <utils/threads.h>
class StreamClientState;
class ourRTSPClient;

class ipcam_rtsp
{
	public:
		unsigned short fVideoHeight;
		unsigned short fVideoWidth;
		unsigned fVideoFPS;
		char* fCodecName;

		char watchVariable;    ///< a flag to stop doEventLoop() set to nonzero will return from doEventLoop()

	public:
		virtual void CloseMediaSinks ()=0;
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

class ipcam_rtsp_play :  public ipcam_rtsp
{
    private:
		ringbufferwriter *pVideoBuffer; ///< video buffer to save the received depacketized video stream
		ringbufferwriter *pAudioBuffer; ///< audio buffer to save the received depacketized audio stream

    public:
		void CloseMediaSinks ();
		int StartRecv ();
		int Close ();
		
		ipcam_rtsp_play ();
		~ipcam_rtsp_play ();
		RTSPClient* rtspClient;
		int Init (char *url, ringbufferwriter *pCodecHRtspVideoBuffer,
					ringbufferwriter *pCodecHRtspAudioBuffer);

};

class ipcam_rtsp_rec :  public ipcam_rtsp
{
	public:
		char* filename;
		int fps;
		
	public:
		void CloseMediaSinks ();
		int StartRecv ();
		int Close ();
		
		int Init(char *url, char* filename, int fps);
		ourRTSPClient* rtspClient;
		ipcam_rtsp_rec ();
		~ipcam_rtsp_rec ();
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


#endif // _IPCAM_RTSP_H_
