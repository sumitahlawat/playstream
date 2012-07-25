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

ipcam_ringsink::ipcam_ringsink (UsageEnvironment& env,
								ringbufferwriter *pBufferWriter,
								unsigned bufferSize) :
								MediaSink(env), fBufferSize(bufferSize)
{
	LOGI("%s : %d \n",__func__,__LINE__);
    fBuffer = new unsigned char[bufferSize];
    pBuffer = pBufferWriter;
    memset((void*)&initTime,0x0,sizeof(timeval));
}

ipcam_ringsink::~ipcam_ringsink ()
{
	LOGI("%s : %d \n",__func__,__LINE__);
    delete[] fBuffer;
}

ipcam_ringsink* ipcam_ringsink::createNew (UsageEnvironment& env,
								ringbufferwriter *pBufferWriter,
								unsigned bufferSize)
{
	LOGI("%s : %d \n",__func__,__LINE__);
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

    LOGI("%s : %d \n",__func__,__LINE__);
    return True;
}

void ipcam_ringsink::afterGettingFrame (void* clientData, unsigned frameSize,
										  unsigned /*numTruncatedBytes*/,
										  struct timeval presentationTime,
										  unsigned /*durationInMicroseconds*/)
{
	LOGI("%s : %d \n",__func__,__LINE__);
    ipcam_ringsink* sink = (ipcam_ringsink*)clientData;
    sink->afterGettingFrame1 (frameSize, presentationTime);
}

void ipcam_ringsink::addData (unsigned char* data, unsigned dataSize,
										  struct timeval presentationTime)
{
	LOGI ("%s,  dataSize = %d spaceleft :%d \n",__func__, dataSize ,pBuffer->SpaceLeft ());
    struct timeval tval =  presentationTime;
    if (initTime.tv_sec == 0 && initTime.tv_usec == 0)
    {
        initTime = presentationTime;
    }

    if (pBuffer->SpaceLeft () < dataSize)
    {
        LOGI ("dataSize = %d, spaceleft = %d\n", dataSize, pBuffer->SpaceLeft ());
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
	LOGI("%s : %d \n",__func__,__LINE__);
    addData (fBuffer, frameSize, presentationTime);
    // Then try getting the next frame:
    continuePlaying ();
}


