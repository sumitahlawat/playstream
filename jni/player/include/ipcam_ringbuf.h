
#ifndef _IPCAM_RINGBUF_H_
#define _IPCAM_RINGBUF_H_

#include <vector>
//#include <utils/SortedVector.h>
#include <list>
#include <pthread.h>
//#include <threads.h>

//using namespace std;

class ringbufferwriter;
class ringbufferreader;

class ringbuffer
{
    friend class ringbufferwriter;
    friend class ringbufferreader;

    private:
        pthread_mutex_t m_Head;         ///< The head-pointer lock
        pthread_mutex_t m_Lists;        ///< For manipulating the lists

        std::vector <ringbufferwriter*> m_Writers;
        std::vector <ringbufferreader*> m_Readers;

        unsigned int m_iBufferSpace, m_iBufferHead, m_iMaxLength;
   
        unsigned char *m_pBuffer;
        unsigned int m_iMaxUnreadData;

        /* Statistical data */
        long m_iBytesWritten;    ///< Total amount of bytes put in buffer
        int m_iOverflows;    ///< Number of times our buffer got full

        /* Called by Writers */
        void AttachWriter (ringbufferwriter *pBufferWriter);

        void DetachWriter (ringbufferwriter *pBufferWriter);

        int AddToBuffer (unsigned char *pData, int iLen, bool bMust_fit);

        int SpaceLeft ();

        int SpaceUsed ();

        /* Called by Readers */
        void AttachReader (ringbufferreader *pBufferReader);

        void DetachReader (ringbufferreader *pBufferReader);
    public:
        ringbuffer (unsigned int iBuffer_space);

        ~ringbuffer ();

        void Flush ();

        unsigned int iGetBufferLength ();
		unsigned int iGetUnreadBufferLength ();
};

class ringbufferwriter
{
    private:
        ringbuffer *m_pRing;

    public:
        ringbufferwriter (ringbuffer *pRingBuffer);

        ~ringbufferwriter ();

        int WriteToBuffer (unsigned char *pBuffer, int iLen, bool bMust_fit = false) const;

        int SpaceLeft () const;

        int SpaceUsed () const;

        void Flush () const;
};

class ringbufferreader
{
        friend class ringbuffer;
    private:

        ringbuffer *m_pRing;

        pthread_mutex_t m_Lock;
        pthread_cond_t m_DataReady;
        unsigned int m_iBufferTail, m_iMyBufferLength;
        unsigned int m_iLowWaterMark, m_iHighWaterMark;
        std::vector <unsigned int> ReaderVector;
     
        unsigned int _FetchNextPacketLength ();
        bool m_queued;

    public:
        ringbufferreader ();

        ringbufferreader (ringbuffer *pRingBuffer);

        ~ringbufferreader ();

        int SpaceUsed () const;

        int ReadFromBufferTail (unsigned char *pBuffer, unsigned int iLen);

        int ReadFromBufferHead (unsigned char *pBuffer, unsigned int iLen, bool bClear = false, unsigned long iTime = ULONG_MAX);

        void SetLowWaterMark (unsigned int bookMark);

        unsigned int GetLowWaterMark () const;

        void SetHighWaterMark (unsigned int bookMark);

        unsigned int GetHighWaterMark () const;

        unsigned int GetNumberOfElements () const;
};

#endif //_IPCAM_RINGBUF_H_


