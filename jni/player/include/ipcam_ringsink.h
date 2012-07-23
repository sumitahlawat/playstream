
#ifndef _IPCAM_RINGSINK_H_
#define _IPCAM_RINGSINK_H_

#ifndef _MEDIA_SINK_HH
#include "MediaSink.hh"
#endif

#include "player.h"
#include "ipcam_ringbuf.h"

class ipcam_ringsink: public MediaSink
{
    public:
        static ipcam_ringsink* createNew (UsageEnvironment& env,
										  ringbufferwriter *pBufferWriter,
										  unsigned bufferSize = 20000);

        void addData (unsigned char* data, unsigned dataSize,
										  struct timeval presentationTime);

    protected:
        ipcam_ringsink (UsageEnvironment& env, ringbufferwriter *pBufferWriter,
						unsigned bufferSize);

        virtual ~ipcam_ringsink ();

    protected:
        static void afterGettingFrame (void* clientData, unsigned frameSize,
										unsigned numTruncatedBytes,
										struct timeval presentationTime,
										unsigned durationInMicroseconds);

        virtual void afterGettingFrame1 (unsigned frameSize,
										struct timeval presentationTime);

        unsigned char* fBuffer; ///< Frame Buffer
        unsigned fBufferSize; ///< Frame Buffer Size

        ringbufferwriter *pBuffer; ///< Ring Buffer
        struct timeval initTime; ///< Ring Buffer Size

    private:

        virtual Boolean continuePlaying ();
};

#endif

