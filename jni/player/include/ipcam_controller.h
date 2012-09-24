#ifndef _IPCAM_CONTROLLER_H_
#define _IPCAM_CONTROLLER_H_

#include "ipcam_rtsp.h"

class ipcam_controller
{
    private:

        ipcam_rtsp_play *IPCAM_rtsp_play; ///< Instance of RTSPRx class for play
		ipcam_rtsp_rec *IPCAM_rtsp_rec; ///< Instance of RTSPRx class for rec
	
		char* pRTSPUrl;		///< RTSP URL pointer
		char* filename;
		int fps;
		int ID;

    public:
        ipcam_controller (char* URL, int camid);
        virtual ~ipcam_controller ();

		int InitMedia ();
		int InitMedia (char* fname, int fps);

        int StartMedia ();
		int StartMedia_Rec ();

        int CloseMedia ();
		int CloseMedia_Rec ();
};

#endif //_IPCAM_CONTROLLER_H_
