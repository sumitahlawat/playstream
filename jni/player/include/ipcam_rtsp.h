
#ifndef _IPCAM_RTSP_H_
#define _IPCAM_RTSP_H_

#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "liveMedia.hh"

#include "ipcam_ringsink.h"

//#include <utils/threads.h>

class ipcam_rtsp
{
	public:
		unsigned short fVideoHeight;
		unsigned short fVideoWidth;
		unsigned fVideoFPS;
		char* fCodecName;

		char watchVariable;    ///< a flag to stop doEventLoop() set to nonzero will return from doEventLoop()

	public:
		virtual bool SetupStreams()= 0;
		virtual void CloseMediaSinks ()=0;
		virtual int StartRecv ()=0;
		virtual int Close ()=0;
		virtual bool TearDownStreams ()=0;

		unsigned short videoWidth() const { return fVideoWidth; }
		unsigned short videoHeight() const { return fVideoHeight; }
		unsigned videoFPS() const { return fVideoFPS; }
		char const* codecName() const { return fCodecName; }
		
		virtual ~ipcam_rtsp() { };


	public:
		pthread_t rtsp_thread; ///< the thread hanlder of created RTSP thread
		UsageEnvironment* env; ///< Specify the environment parameters
		Medium* rtspClient;    ///< Specify the rtsp client
		MediaSession* session; ///< Specify the rtsp sessions

};

class ipcam_rtsp_play :  public ipcam_rtsp
{
    private:
		ringbufferwriter *pVideoBuffer; ///< video buffer to save the received depacketized video stream
		ringbufferwriter *pAudioBuffer; ///< audio buffer to save the received depacketized audio stream

    public:
		bool SetupStreams();
		void CloseMediaSinks ();
		int StartRecv ();
		int Close (){ return 0;};
		bool TearDownStreams ();
		
		ipcam_rtsp_play ();
		~ipcam_rtsp_play ();

		int Init (char *url, ringbufferwriter *pCodecHRtspVideoBuffer,
					ringbufferwriter *pCodecHRtspAudioBuffer);

};

class ipcam_rtsp_rec :  public ipcam_rtsp
{
	public:
		char* filename;
		int fps;
		
	private:
		QuickTimeFileSink* qtOut;

	public:
		bool SetupStreams();
		void CloseMediaSinks ();
		int StartRecv ();
		int Close ();
		bool TearDownStreams ();
		
		int Init(char *url, char* filename, int fps);

		ipcam_rtsp_rec ();
		~ipcam_rtsp_rec ();
};

#endif // _IPCAM_RTSP_H_
