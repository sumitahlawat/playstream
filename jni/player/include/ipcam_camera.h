#ifndef __ipcam_camera_H__
#define __ipcam_camera_H__

#include "ipcam_ringbuf.h"
#include "ipcam_controller.h"
#include "ipcam_vdec.h"
#include <pthread.h>

class ipcam_camera
{
private:
	ipcam_controller*	pIpCam;
	ringbuffer*		pVRBCam;
	ringbuffer*		pARBCam;
	ringbufferwriter*	pVRBWriter;
	ringbufferwriter*	pARBWriter;
	pthread_t		ringThreadVCam;
	char*			url;
	char*			recfilename;
	int 			fps;

public:
	ipcam_vdec*		pVDec;
	ringbufferreader*	pVRBReader;
	ringbufferreader*	pARBReader;

	int 			camID;
	int				isPlaying;
	int				isRecording;

public:
	ipcam_camera(char* URL, int ID, int frame_rate);
	~ipcam_camera();

	int init();
	void deinit();

	int play_connect();
	int rec_connect();
	int disconnect();

	int start_playback();
	int stop_playback();
	int start_recording();
	int stop_recording();

	void set_recFile(char* fname);

	ipcam_vdec* getVideoDec() {return pVDec; }

	ringbuffer* getVideoBuffer() {return pVRBCam; }
	ringbuffer* getAudioBuffer() {return pARBCam; }
	ringbufferwriter* getVideoWriter() {return pVRBWriter; }
	ringbufferwriter* getAudioWriter() {return pARBWriter; }
	ringbufferreader* getVideoReader() {return pVRBReader; }
	ringbufferreader* getAudioReader() {return pARBReader; }

public:
	#define VIDEOBUFFER_SZ	2*1024*1024
	#define AUDIOBUFFER_SZ	2*24*1024
};


#endif /* __ipcam_camera_H__ */
