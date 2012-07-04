/*
 * ipcam_ringbuf.cpp
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

#include "ipcam_ringbuf.h"
#include <stdio.h>

const unsigned MAX_UNREAD_DATA = 100;

ringbuffer::ringbuffer (unsigned int buffer_space)
{
    fprintf (stderr, "Construct ringbuffer ()\n");
    m_iBufferSpace = buffer_space;
    m_pBuffer = NULL;
    m_iBufferHead = 0;
    m_iMaxLength = 0;
    m_iMaxUnreadData = 0;
    m_iBytesWritten = 0;
    m_iOverflows = 0;

    if (m_iBufferSpace > 0)
    {
        m_pBuffer = new unsigned char[m_iBufferSpace];
        memset(m_pBuffer,0x0,buffer_space);
    }
}

ringbuffer::~ringbuffer ()
{
    fprintf (stderr, "DeConstruct ringbuffer ()\n");
    if(m_pBuffer != NULL)
    {
        delete[] m_pBuffer;
    }
    m_pBuffer = NULL;
}

void ringbuffer::AttachWriter (ringbufferwriter *s)
{
    m_Lists.lock();
    //m_Writers.append(s);
    m_Writers.add(s);
    m_Lists.unlock();
}

void ringbuffer::DetachWriter (ringbufferwriter *s)
{
    m_Lists.lock();

    //int iIndex = m_Writers.indexOf(s,0);
    int iIndex = m_Writers.indexOf(s);
    if(iIndex != -1)
    {
        m_Writers.removeAt(iIndex);
    }
    m_Lists.unlock();
}

int ringbuffer::AddToBuffer (unsigned char *data, int len, bool must_fit)
{
    int plus;
    ringbufferreader *reader;

    m_Head.lock();

    if (len > 0)
    {
        if (m_iBufferHead + len > m_iBufferSpace)
        {
            /* hrmpf. Copy data in 2 strokes */
            plus = m_iBufferSpace - m_iBufferHead;
            memcpy(m_pBuffer + m_iBufferHead, data, plus);
            memcpy(m_pBuffer, data + plus, len - plus);
        }
        else // 'ANSI C++ forbids using pointer of type 'void *' in arithmetic'... How inconvenient.
        {
            memcpy(m_pBuffer + m_iBufferHead, data, len);
        }

        m_iBufferHead = (m_iBufferHead + len) % m_iBufferSpace;
        m_iBytesWritten += len;
    }

    m_Lists.lock(); // prevent list manipulation while we're busy
    int iNumberofReaders = m_Readers.size();
    m_iMaxLength = 0;

    for (int iCount = 0; iCount < iNumberofReaders ; iCount++)
    {
        reader = m_Readers[iCount];
        reader->m_Lock.lock();

        reader->m_iMyBufferLength += len;
        if (reader->m_iMyBufferLength > m_iMaxLength)
        {
            m_iMaxLength = reader->m_iMyBufferLength;
        }

        if (len > 0)
        {
            reader->ReaderVector.add(len);
            if (reader->ReaderVector.size() == 1 || !reader->m_queued)
            {
				//fprintf(stderr,"we have data len %d ... send signal ****", len);
                reader->m_DataReady.signal();
            }
        }
        reader->m_Lock.unlock();
    }

	// update unread data
	m_iMaxUnreadData += len;
	
    m_Lists.unlock();
    m_Head.unlock();
    return len;
}

int ringbuffer::SpaceLeft ()
{
    int n;
    m_Head.lock();
    n = m_iBufferSpace - m_iMaxLength;
    m_Head.unlock();
    return n;
}

int ringbuffer::SpaceUsed ()
{
    ringbufferreader *reader;
    int n;

    m_Head.lock();
    m_Lists.lock(); // prevent list manipulation while we're busy
    m_iMaxLength = 0;
    for (int iCount = 0; iCount < m_Readers.size(); iCount++)
    {
        reader = m_Readers[iCount];
        reader->m_Lock.lock();
        if (reader->m_iMyBufferLength > m_iMaxLength)
        {
            m_iMaxLength = reader->m_iMyBufferLength;
        }
        reader->m_Lock.unlock();
    }
    m_Lists.unlock();

    n = m_iMaxLength;
    m_Head.unlock();
    return n;
}

void ringbuffer::AttachReader (ringbufferreader *r)
{
    m_Head.lock(); // make sure BufferHead doesn't change
    r->m_iBufferTail = m_iBufferHead;
    m_Lists.lock();
    //m_Readers.append(r);
    m_Readers.add(r);
    m_Lists.unlock();
    m_Head.unlock();
}

void ringbuffer::DetachReader (ringbufferreader *r)
{
    m_Lists.lock();
    //int iIndex = m_Readers.indexOf(r,0);
    int iIndex = m_Readers.indexOf(r);
    if(iIndex != -1)
    {
        m_Readers.removeAt(iIndex);
    }
    m_Lists.unlock();
}

void ringbuffer::Flush ()
{
    ringbufferreader *reader = 0;

    m_Head.lock(); // make sure BufferHead doesn't change
    m_Lists.lock();
    m_iMaxLength = 0;
    for (int iCount = 0; iCount < m_Readers.size(); iCount++)
    {
        reader = m_Readers[iCount];
        reader->m_Lock.lock();
        reader->m_iBufferTail = m_iBufferHead; // reset poiinters

        reader->ReaderVector.clear();
        reader->m_iMyBufferLength = 0;
        reader->m_queued = false;
        reader->m_Lock.unlock();
    }
    m_Lists.unlock();
    m_Head.unlock();
}

unsigned int ringbuffer::iGetBufferLength ()
{
    return m_iBufferSpace;
}

unsigned int ringbuffer::iGetUnreadBufferLength ()
{
	return m_iMaxUnreadData;
}

/****************************************************************************/

ringbufferwriter::ringbufferwriter (ringbuffer *ring)
{
    m_pRing = ring;
    m_pRing->AttachWriter(this);
    fprintf (stderr, "Construct ringbufferwriter ()\n");
}

ringbufferwriter::~ringbufferwriter ()
{
    fprintf (stderr, "DeConstruct ringbufferwriter ()\n");
    m_pRing->DetachWriter(this);
}

int ringbufferwriter::WriteToBuffer (unsigned char *data, int len, bool must_fit) const
{
	int status = m_pRing->AddToBuffer(data, len, must_fit);
    return status;
}

int ringbufferwriter::SpaceLeft () const
{
    return m_pRing->SpaceLeft();
}

int ringbufferwriter::SpaceUsed () const
{
    return m_pRing->SpaceUsed();
}

void ringbufferwriter::Flush () const
{
    m_pRing->Flush();
}

/****************************************************************************/

ringbufferreader::ringbufferreader (ringbuffer *ring)
{
    m_pRing = ring;
    m_iMyBufferLength = 0;
    m_iLowWaterMark = m_iHighWaterMark = 0;
    ReaderVector.clear();
    m_pRing->AttachReader(this);
    m_queued = false;
    fprintf (stderr, "Construct ringbufferreader ()\n");
}

ringbufferreader::~ringbufferreader ()
{
    fprintf (stderr, "DeConstruct ringbufferreader ()\n");
    m_pRing->DetachReader(this);
}

int ringbufferreader::SpaceUsed () const
{
    return m_iMyBufferLength;
}

int ringbufferreader::ReadFromBufferTail (unsigned char *data, unsigned int requested_len)
{
    unsigned int buflen =0,len =0;

    { // critical section
        //fprintf(stderr, "Entering critical section\n");
		//fprintf(stderr, "\t\t m_iMyBufferLength %d, m_queued %d, m_iLowWaterMark %d \n",
			//m_iMyBufferLength, m_queued, m_iLowWaterMark);
        m_Lock.lock();
        while (m_iMyBufferLength == 0 || (!m_queued && m_iMyBufferLength < m_iLowWaterMark))
        {
			//if (!m_DataReady.waitRelative(m_Lock, seconds(2)))
			/*
			m_DataReady.waitRelative(m_Lock, seconds(2));
            {
				fprintf(stderr, "no data? quit \n");
                return 0; // no data? quit
            }
            */
			//m_DataReady.wait(m_Lock);
			m_DataReady.waitRelative(m_Lock, milliseconds(20));
        }
        len = _FetchNextPacketLength();
		//fprintf (stderr, "_FetchNextPacketLength %d\n", len);
        buflen = m_iMyBufferLength;
        //fprintf (stderr, "Enough data queued %d\n", buflen);
        m_queued = true;
		m_Lock.unlock();
    }

    if (len > buflen)
    {
        len = buflen;
    }
    if (requested_len < len)
    {
        len = requested_len;
    }
    if (len > 0)
    {
        if (m_iBufferTail + len > m_pRing->m_iBufferSpace)
        {
            buflen = m_pRing->m_iBufferSpace - m_iBufferTail;
            memcpy(data, m_pRing->m_pBuffer + m_iBufferTail, buflen);
            memcpy(data + buflen, m_pRing->m_pBuffer, len - buflen);
        }
        else
        {
            memcpy(data, m_pRing->m_pBuffer + m_iBufferTail, len);
        }
    }
    //QMutexLocker lock(&m_Lock); // keep this lock minimal
    m_Lock.lock();
    m_iMyBufferLength -= len;
    if ((requested_len == 160))
    {
        m_iBufferTail = (m_iBufferTail + len) % m_pRing->m_iBufferSpace;
    }
    else
    {
        m_iBufferTail = (m_iBufferTail + len) % m_pRing->m_iBufferSpace;
    }

    //update Unread data
    m_pRing->m_iMaxUnreadData -= len;
    m_Lock.unlock();
   return len;
}

int ringbufferreader::ReadFromBufferHead (unsigned char *data, unsigned int len, bool clear, unsigned long time)
{
    unsigned int buflen, head;

    m_Lock.lock();
    len = _FetchNextPacketLength();
    if (m_iMyBufferLength <= 0)
    {
        if (m_DataReady.waitRelative(m_Lock, (nsecs_t)time)) {}
    }

    m_Lock.unlock();
    m_Lock.lock();
    buflen = m_iMyBufferLength;
    if (len > buflen)
    {
        len = buflen;
    }
    head = (m_iBufferTail + buflen - len) % m_pRing->m_iBufferSpace;
    m_Lock.unlock();

    if (len > 0)
    {
        if (head + len > m_pRing->m_iBufferSpace)
        {
            buflen = m_pRing->m_iBufferSpace - head; // restant
            memcpy(data, m_pRing->m_pBuffer + head, buflen);
            memcpy(data + buflen, m_pRing->m_pBuffer, len - buflen);
        }
        else
        {
            memcpy(data, m_pRing->m_pBuffer + head, len);
        }
    }
    if (clear)
    {
        m_Lock.lock();
        m_iMyBufferLength = 0;

        ReaderVector.clear();
        m_iBufferTail = m_pRing->m_iBufferHead;
        m_Lock.unlock();
    }
    return len;
}

void ringbufferreader::SetLowWaterMark (unsigned int mark)
{
    if (mark > m_pRing->m_iBufferSpace)
    {
        return;
    }
    m_iLowWaterMark = mark;
}

unsigned int ringbufferreader::GetLowWaterMark () const
{
    return m_iLowWaterMark;
}

void ringbufferreader::SetHighWaterMark (unsigned int mark)
{
    if (mark > m_pRing->m_iBufferSpace)
    {
        return;
    }
    m_iHighWaterMark = mark;
}

unsigned int ringbufferreader::GetHighWaterMark () const
{
    return m_iHighWaterMark;
}

unsigned int ringbufferreader :: _FetchNextPacketLength ()
{
    if (!ReaderVector.isEmpty())
    {
    	/*
        android::Vector<unsigned int>::iterator itVectorData;
        itVectorData = ReaderVector.begin();
        unsigned int datalength = *itVectorData;
        ReaderVector.erase(itVectorData);
        */
		int index = ReaderVector.size() - 1;
        unsigned int datalength = ReaderVector.itemAt(index);
		ReaderVector.pop();
        return datalength;
    }
    return 0;
}

unsigned int ringbufferreader :: GetNumberOfElements () const
{
    return ReaderVector.size();
}


