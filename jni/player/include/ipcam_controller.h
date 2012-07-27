#ifndef _IPCAM_CONTROLLER_H_
#define _IPCAM_CONTROLLER_H_

#include "ipcam_rtsp.h"

class ipcam_controller
{
    private:
        int bAdapt;               ///< Adaptive Jitter Compensation Flag
        pthread_t TxThread;        ///< Trasmission thread
        pthread_t RxThread;        ///< Reception thread
        ipcam_rtsp_play *IPCAM_rtsp_play; ///< Instance of RTSPRx class for play

		ipcam_rtsp_rec *IPCAM_rtsp_rec; ///< Instance of RTSPRx class for rec
	
		char* pRTSPUrl;		///< RTSP URL pointer
		char* filename;
		int fps;

        static void *StartIPCAMTx (void *arg);
        static void *StartIPCAMRx (void *arg);

    public:
        ipcam_controller (char* URL);
        virtual ~ipcam_controller ();

		int InitMedia ();
		int InitMedia (char* fname, int fps);

        int StartMedia ();
		int StartMedia_Rec ();

        int CloseMedia ();
		int CloseMedia_Rec ();
};

#endif //_IPCAM_CONTROLLER_H_


