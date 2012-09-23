#ifndef __ipcam_camera_H__
#define __ipcam_camera_H__

#include "ipcam_controller.h"
#include "ipcam_vdec.h"

class ipcam_camera
{
private:
	ipcam_controller* pIpCam;
	char* url;
	char* recfilename;
	int fps;

public:
	int camID;
	int	isPlaying;
	int	isRecording;

public:
	ipcam_camera(char* URL, int ID, int frame_rate);
	~ipcam_camera();

	ipcam_vdec*		pVDec;
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

public:
	#define VIDEOBUFFER_SZ	2*1024*1024
	#define AUDIOBUFFER_SZ	2*24*1024
};

#endif /* __ipcam_camera_H__ */
