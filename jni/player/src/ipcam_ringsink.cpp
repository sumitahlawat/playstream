/*
 * ipcam_ringsink.cpp
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

#include "ipcam_ringsink.h"
#include "GroupsockHelper.hh"

ipcam_ringsink::ipcam_ringsink (UsageEnvironment& env,
								ringbufferwriter *pBufferWriter,
								unsigned bufferSize) :
								MediaSink(env), fBufferSize(bufferSize)
{
    fBuffer = new unsigned char[bufferSize];
    pBuffer = pBufferWriter;
    memset((void*)&initTime,0x0,sizeof(timeval));
}

ipcam_ringsink::~ipcam_ringsink ()
{
    delete[] fBuffer;
}

ipcam_ringsink* ipcam_ringsink::createNew (UsageEnvironment& env,
								ringbufferwriter *pBufferWriter,
								unsigned bufferSize)
{
    do
    {
        return new ipcam_ringsink (env, pBufferWriter, bufferSize);
    }
    while (0);

    return NULL;
}

Boolean ipcam_ringsink::continuePlaying ()
{
    if (fSource == NULL)
    {
        return False;
    }
    fSource->getNextFrame (fBuffer, fBufferSize, afterGettingFrame, this,
										  onSourceClosure, this);

	//fprintf(stderr, "^^^^fSource FrameSize %d\n", fSource->fFrameSize);
    return True;
}

void ipcam_ringsink::afterGettingFrame (void* clientData, unsigned frameSize,
										  unsigned /*numTruncatedBytes*/,
										  struct timeval presentationTime,
										  unsigned /*durationInMicroseconds*/)
{
    ipcam_ringsink* sink = (ipcam_ringsink*)clientData;
    sink->afterGettingFrame1 (frameSize, presentationTime);
}

void ipcam_ringsink::addData (unsigned char* data, unsigned dataSize,
										  struct timeval presentationTime)
{
    struct timeval tval =  presentationTime;

    if (initTime.tv_sec == 0 && initTime.tv_usec == 0)
    {
        initTime = presentationTime;
    }

    if (pBuffer->SpaceLeft () < dataSize)
    {
        fprintf (stderr, "dataSize = %d, spaceleft = %d\n", dataSize,
				 pBuffer->SpaceLeft ());
        pBuffer-> Flush ();
    }

    while (pBuffer->WriteToBuffer(data, dataSize, true) == 0)
    {
        usleep(2000);
    }
}

void ipcam_ringsink::afterGettingFrame1 (unsigned frameSize,
										 struct timeval presentationTime)
{
    addData (fBuffer, frameSize, presentationTime);
    // Then try getting the next frame:
    continuePlaying ();
}


