/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2012 Live Networks, Inc.  All rights reserved.
// A subclass of "ServerMediaSession" that can be used to create a (unicast) RTSP servers that acts as a 'proxy' for
// another (unicast or multicast) RTSP/RTP stream.
// Implementation

#include "liveMedia.hh"
#include "GroupsockHelper.hh" // for "our_random()"

// A subclass of "RTSPClient", used to refer to the particular "ProxyServerMediaSession" object being used.  Definition:

class ProxyRTSPClient: public RTSPClient {
public:
  ProxyRTSPClient(class ProxyServerMediaSession& ourServerMediaSession, char const* rtspURL,
		  char const* username, char const* password,
		  portNumBits tunnelOverHTTPPortNum, int verbosityLevel);
  virtual ~ProxyRTSPClient();

  Authenticator* auth() { return fOurAuthenticator; }

  void continueAfterDESCRIBE(char const* sdpDescription);
  void continueAfterOPTIONS(int resultCode);
  void continueAfterSETUP();

  void scheduleLivenessCommand();
  static void sendLivenessCommand(void* clientData);

  void scheduleDESCRIBECommand();
  static void sendDESCRIBE(void* clientData);

  static void subsessionTimeout(void* clientData);
  void handleSubsessionTimeout();

private:
  void reset();

private:
  friend class ProxyServerMediaSubsession;
  ProxyServerMediaSession& fOurServerMediaSession;
  Authenticator* fOurAuthenticator;
  Boolean fStreamRTPOverTCP;
  class ProxyServerMediaSubsession *fSetupQueueHead, *fSetupQueueTail;
  unsigned fNumSetupsDone;
  TaskToken fLivenessCommandTask, fDESCRIBECommandTask, fSubsessionTimerTask;
  unsigned fNextDESCRIBEDelay; // in seconds
  Boolean fLastCommandWasPLAY;
};


// A "OnDemandServerMediaSubsession" subclass, used to implement a unicast RTSP server that's proxying another RTSP stream:

class ProxyServerMediaSubsession: public OnDemandServerMediaSubsession {
public:
  ProxyServerMediaSubsession(MediaSubsession& mediaSubsession);

  char const* codecName() const { return fClientMediaSubsession.codecName(); }

private: // redefined virtual functions
  virtual FramedSource* createNewStreamSource(unsigned clientSessionId,
                                              unsigned& estBitrate);
  virtual void closeStreamSource(FramedSource *inputSource);
  virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock,
                                    unsigned char rtpPayloadTypeIfDynamic,
                                    FramedSource* inputSource);

private:
  static void subsessionByeHandler(void* clientData);
  void subsessionByeHandler();

  int verbosityLevel() const { return ((ProxyServerMediaSession*)fParentSession)->fVerbosityLevel; }
  void reset();

private:
  friend class ProxyRTSPClient;
  MediaSubsession& fClientMediaSubsession; // the 'client' media subsession object that corresponds to this 'server' media subsession
  ProxyServerMediaSubsession* fNext; // used when we're part of a queue
  Boolean fHaveSetupStream;
};


////////// ProxyServerMediaSession implementation //////////

UsageEnvironment& operator<<(UsageEnvironment& env, const ProxyServerMediaSession& psms) { // used for debugging
  return env << "ProxyServerMediaSession[\"" << psms.url() << "\"]";
}

ProxyServerMediaSession* ProxyServerMediaSession
::createNew(UsageEnvironment& env, char const* inputStreamURL, char const* streamName,
	    char const* username, char const* password, portNumBits tunnelOverHTTPPortNum, int verbosityLevel) {
  return new ProxyServerMediaSession(env, inputStreamURL, streamName, username, password, tunnelOverHTTPPortNum, verbosityLevel);
}

ProxyServerMediaSession::ProxyServerMediaSession(UsageEnvironment& env, char const* inputStreamURL, char const* streamName,
						 char const* username, char const* password,
						 portNumBits tunnelOverHTTPPortNum, int verbosityLevel)
  : ServerMediaSession(env, streamName, NULL, NULL, False, NULL),
    describeCompletedFlag(0), fVerbosityLevel(verbosityLevel) {

  // Open a RTSP connection to the input stream, and send a "DESCRIBE" command.
  // We'll use the SDP description in the response to set ourselves up.
  fProxyRTSPClient = new ProxyRTSPClient(*this, inputStreamURL, username, password,
					 tunnelOverHTTPPortNum, verbosityLevel > 0 ? verbosityLevel-1 : verbosityLevel);
  ProxyRTSPClient::sendDESCRIBE(fProxyRTSPClient);
}

ProxyServerMediaSession::~ProxyServerMediaSession() {
  Medium::close(fProxyRTSPClient);
  Medium::close(fClientMediaSession);
}

char const* ProxyServerMediaSession::url() const {
  return fProxyRTSPClient == NULL ? NULL : fProxyRTSPClient->url();
}

void ProxyServerMediaSession::continueAfterDESCRIBE(char const* sdpDescription) {
  describeCompletedFlag = 1;

  // Create a (client) "MediaSession" object from the stream's SDP description ("resultString"), then iterate through its
  // "MediaSubsession" objects, to set up corresponding "ServerMediaSubsession" objects that we'll use to serve the stream's tracks.
  do {
    fClientMediaSession = MediaSession::createNew(envir(), sdpDescription);
    if (fClientMediaSession == NULL) break;

    MediaSubsessionIterator iter(*fClientMediaSession);
    for (MediaSubsession* mss = iter.next(); mss != NULL; mss = iter.next()) {
      ServerMediaSubsession* smss = new ProxyServerMediaSubsession(*mss);
      addSubsession(smss);
      if (fVerbosityLevel > 0) {
	envir() << *this << " added new \"ProxyServerMediaSubsession\" for "
		<< mss->protocolName() << "/" << mss->mediumName() << "/" << mss->codecName() << " track\n";
      }
    }
  } while (0);
}


///////// RTSP 'response handlers' //////////

static void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString) {
  char const* res;

  if (resultCode == 0) {
    // The "DESCRIBE" command succeeded, so "resultString" should be the stream's SDP description.
    res = resultString;
  } else {
    // The "DESCRIBE" command failed.
    res = NULL;
  }
  ((ProxyRTSPClient*)rtspClient)->continueAfterDESCRIBE(res);
  delete[] resultString;
}

static void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString) {
  if (resultCode == 0) {
    ((ProxyRTSPClient*)rtspClient)->continueAfterSETUP();
  }
  delete[] resultString;
}

static void continueAfterOPTIONS(RTSPClient* rtspClient, int resultCode, char* resultString) {
  ((ProxyRTSPClient*)rtspClient)->continueAfterOPTIONS(resultCode);
  delete[] resultString;
}


////////// "ProxyRTSPClient" implementation /////////

UsageEnvironment& operator<<(UsageEnvironment& env, const ProxyRTSPClient& proxyRTSPClient) { // used for debugging
  return env << "ProxyRTSPClient[\"" << proxyRTSPClient.url() << "\"]";
}

ProxyRTSPClient::ProxyRTSPClient(ProxyServerMediaSession& ourServerMediaSession, char const* rtspURL,
				 char const* username, char const* password, portNumBits tunnelOverHTTPPortNum, int verbosityLevel)
  : RTSPClient(ourServerMediaSession.envir(), rtspURL, verbosityLevel, "ProxyRTSPClient",
	       tunnelOverHTTPPortNum == (portNumBits)(~0) ? 0 : tunnelOverHTTPPortNum),
    fOurServerMediaSession(ourServerMediaSession), fStreamRTPOverTCP(tunnelOverHTTPPortNum != 0),
    fLivenessCommandTask(NULL), fDESCRIBECommandTask(NULL), fSubsessionTimerTask(NULL) {
  reset();

  if (username != NULL && password != NULL) {
    fOurAuthenticator = new Authenticator(username, password);
  } else {
    fOurAuthenticator = NULL;
  }
}

void ProxyRTSPClient::reset() {
  envir().taskScheduler().unscheduleDelayedTask(fLivenessCommandTask); fLivenessCommandTask = NULL;
  envir().taskScheduler().unscheduleDelayedTask(fDESCRIBECommandTask); fDESCRIBECommandTask = NULL;
  envir().taskScheduler().unscheduleDelayedTask(fSubsessionTimerTask); fSubsessionTimerTask = NULL;

  fSetupQueueHead = fSetupQueueTail = NULL;
  fNumSetupsDone = 0;
  fNextDESCRIBEDelay = 1;
  fLastCommandWasPLAY = False;
}

ProxyRTSPClient::~ProxyRTSPClient() {
  reset();

  delete fOurAuthenticator;
}

void ProxyRTSPClient::continueAfterDESCRIBE(char const* sdpDescription) {
  if (sdpDescription != NULL) {
    fOurServerMediaSession.continueAfterDESCRIBE(sdpDescription);

    // Unlike most RTSP streams, there might be a long delay between this "DESCRIBE" command (to the downstream server) and the
    // subsequent "SETUP"/"PLAY" - which doesn't occur until the first time that a client requests the stream.
    // To prevent the proxied connection (between us and the downstream server) from timing out, we send periodic
    // "OPTIONS" commands.  (The usual RTCP liveness mechanism wouldn't work here, because RTCP packets don't get sent
    // until after the "PLAY" command.)
    scheduleLivenessCommand();
  } else {
    // The "DESCRIBE" command failed, most likely because the server or the stream is not yet running.
    // Reschedule another "DESCRIBE" command to take place later:
    scheduleDESCRIBECommand();
  }
}

void ProxyRTSPClient::continueAfterOPTIONS(int resultCode) {
  if (resultCode < 0) {
    // The "OPTIONS" command failed without getting a response from the server (otherwise "resultCode" would have been >= 0).
    // From this, we infer that the server (which had previously been running) has now failed - perhaps temporarily.
    // We handle this by resetting our connection state with this server.  Any current clients will have to time out, but
    // subsequent clients will cause new RTSP "SETUP"s and "PLAY"s to get done, restarting the stream.
    if (fVerbosityLevel > 0) {
      envir() << *this << ": lost connection to server ('errno': " << -resultCode << ").  Resetting...\n";
    }
    reset();

    // Also "reset()" each of our "ProxyServerMediaSubsession"s:
    ServerMediaSubsessionIterator iter(fOurServerMediaSession);
    ProxyServerMediaSubsession* psmss;
    while ((psmss = (ProxyServerMediaSubsession*)(iter.next())) != NULL) {
      psmss->reset();
    }
  }

  // Schedule the next RTSP "OPTIONS" command (to show client liveness):
  scheduleLivenessCommand();
}

#define SUBSESSION_TIMEOUT_SECONDS 10 // how many seconds to wait for the last track's "SETUP" to be done (note below)

void ProxyRTSPClient::continueAfterSETUP() {
  if (fVerbosityLevel > 0) {
    envir() << *this << "::continueAfterSETUP(): head codec: " << fSetupQueueHead->fClientMediaSubsession.codecName()
	    << "; numSubsessions " << fSetupQueueHead->fParentSession->numSubsessions() << "\n\tqueue:";
    for (ProxyServerMediaSubsession* p = fSetupQueueHead; p != NULL; p = p->fNext) {
      envir() << "\t" << p->fClientMediaSubsession.codecName();
    }
    envir() << "\n";
  }
  envir().taskScheduler().unscheduleDelayedTask(fSubsessionTimerTask); // in case it had been set

  // Dequeue the first "ProxyServerMediaSubsession" from our 'SETUP queue'.  It will be the one for which this "SETUP" was done:
  ProxyServerMediaSubsession* smss = fSetupQueueHead; // Assert: != NULL
  fSetupQueueHead = fSetupQueueHead->fNext;
  if (fSetupQueueHead == NULL) fSetupQueueTail = NULL;

  if (fSetupQueueHead != NULL) {
    // There are still entries in the queue, for tracks for which we have still to do a "SETUP".
    // "SETUP" the first of these now:
    sendSetupCommand(fSetupQueueHead->fClientMediaSubsession, ::continueAfterSETUP,
		     False, fStreamRTPOverTCP, False, fOurAuthenticator);
    ++fNumSetupsDone;
    fSetupQueueHead->fHaveSetupStream = True;
  } else {
    if (fNumSetupsDone >= smss->fParentSession->numSubsessions()) {
      // We've now finished setting up each of our subsessions (i.e., 'tracks').
      // Continue by sending a "PLAY" command (an 'aggregate' "PLAY" command, on the whole session):
      sendPlayCommand(smss->fClientMediaSubsession.parentSession(), NULL, -1.0f, -1.0f, 1.0f, fOurAuthenticator);
          // the "-1.0f" "start" parameter causes the "PLAY" to be sent without a "Range:" header, in case we'd already done
          // a "PLAY" before (as a result of a 'subsession timeout' (note below))
      fLastCommandWasPLAY = True;
    } else {
      // Some of this session's subsessions (i.e., 'tracks') remain to be "SETUP".  They might get "SETUP" very soon, but it's
      // also possible - if the remote client chose to play only some of the session's tracks - that they might not.
      // To allow for this possibility, we set a timer.  If the timer expires without the remaining subsessions getting "SETUP",
      // then we send a "PLAY" command anyway:
      fSubsessionTimerTask
	= envir().taskScheduler().scheduleDelayedTask(SUBSESSION_TIMEOUT_SECONDS*1000000, (TaskFunc*)subsessionTimeout, this);
    }
  }
}

void ProxyRTSPClient::scheduleLivenessCommand() {
  // Delay a random time before sending "GET_PARAMETER":
  unsigned secondsToDelay = 30 + (our_random()&0x1F); // [30..61] seconds
  envir().taskScheduler().scheduleDelayedTask(secondsToDelay*1000000, sendLivenessCommand, this);
}

void ProxyRTSPClient::sendLivenessCommand(void* clientData) {
  ProxyRTSPClient* rtspClient = (ProxyRTSPClient*)clientData;
  rtspClient->sendOptionsCommand(::continueAfterOPTIONS, rtspClient->auth());
}

void ProxyRTSPClient::scheduleDESCRIBECommand() {
  // Delay 1s, 2s, 4s, 8s ... 256s until sending the next "DESCRIBE".  Then, keep delaying a random time from [256..511] seconds:
  unsigned secondsToDelay;
  if (fNextDESCRIBEDelay <= 256) {
    secondsToDelay = fNextDESCRIBEDelay;
    fNextDESCRIBEDelay *= 2;
  } else {
    secondsToDelay = 256 + (our_random()&0xFF); // [256..511] seconds
  }

  if (fVerbosityLevel > 0) {
    envir() << *this << ": RTSP \"DESCRIBE\" command failed; trying again in " << secondsToDelay << " seconds\n";
  }
  envir().taskScheduler().scheduleDelayedTask(secondsToDelay*1000000, sendDESCRIBE, this);
}

void ProxyRTSPClient::sendDESCRIBE(void* clientData) {
  ProxyRTSPClient* rtspClient = (ProxyRTSPClient*)clientData;
  if (rtspClient != NULL) rtspClient->sendDescribeCommand(::continueAfterDESCRIBE, rtspClient->auth());
}

void ProxyRTSPClient::subsessionTimeout(void* clientData) {
  ((ProxyRTSPClient*)clientData)->handleSubsessionTimeout();
}

void ProxyRTSPClient::handleSubsessionTimeout() {
  // We still have one or more subsessions ('tracks') left to "SETUP".  But we can't wait any longer for them.  Send a "PLAY" now:
  MediaSession* sess = fOurServerMediaSession.fClientMediaSession;
  if (sess != NULL) sendPlayCommand(*sess, NULL, -1.0f, -1.0f, 1.0f, fOurAuthenticator);
  fLastCommandWasPLAY = True;
}


//////// "ProxyServerMediaSubsession" implementation //////////

ProxyServerMediaSubsession::ProxyServerMediaSubsession(MediaSubsession& mediaSubsession)
  : OnDemandServerMediaSubsession(mediaSubsession.parentSession().envir(), True/*reuseFirstSource*/),
    fClientMediaSubsession(mediaSubsession), fNext(NULL), fHaveSetupStream(False) {
}

UsageEnvironment& operator<<(UsageEnvironment& env, const ProxyServerMediaSubsession& psmss) { // used for debugging
  return env << "ProxyServerMediaSubsession[\"" << psmss.codecName() << "\"]";
}

FramedSource* ProxyServerMediaSubsession::createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate) {
  if (verbosityLevel() > 0) {
    envir() << *this << "::createNewStreamSource(session id " << clientSessionId << ")\n";
  }
  // If we haven't yet created a data source from our 'media subsession' object, initiate() it to do so:
  if (fClientMediaSubsession.readSource() == NULL) {
    fClientMediaSubsession.receiveRawMP3ADUs(); // hack for MPA-ROBUST streams
    fClientMediaSubsession.initiate();
    if (verbosityLevel() > 0) {
      envir() << "\tInitiated: " << *this << "\n";
    }

    if (fClientMediaSubsession.readSource() != NULL) {
      // Some data sources require a 'framer' object to be added, before they can be fed into a "RTPSink".  Adjust for this now:
      char const* const codecName = fClientMediaSubsession.codecName();
      if (strcmp(codecName, "DV") == 0) {
	fClientMediaSubsession.addFilter(DVVideoStreamFramer::createNew(envir(), fClientMediaSubsession.readSource()));
      } else if (strcmp(codecName, "H264") == 0) {
	fClientMediaSubsession.addFilter(H264VideoStreamDiscreteFramer::createNew(envir(), fClientMediaSubsession.readSource()));
      } else if (strcmp(codecName, "MP4V-ES") == 0) {
	fClientMediaSubsession.addFilter(MPEG4VideoStreamDiscreteFramer::createNew(envir(), fClientMediaSubsession.readSource()));
      } else if (strcmp(codecName, "MPV") == 0) {
	fClientMediaSubsession.addFilter(MPEG1or2VideoStreamDiscreteFramer::createNew(envir(), fClientMediaSubsession.readSource()));
      }
    }

    if (fClientMediaSubsession.rtcpInstance() != NULL) {
      fClientMediaSubsession.rtcpInstance()->setByeHandler(subsessionByeHandler, this);
    }
  }

  ProxyServerMediaSession* const sms = (ProxyServerMediaSession*)fParentSession;
  ProxyRTSPClient* const proxyRTSPClient = sms->fProxyRTSPClient;
  if (clientSessionId != 0) {
    // We're being called as a result of implementing a RTSP "SETUP".
    if (!fHaveSetupStream) {
      // This is our first "SETUP".  Send RTSP "SETUP" and later "PLAY" commands to the proxied server, to start streaming:
      // (Before sending "SETUP", enqueue ourselves on the "RTSPClient"s 'SETUP queue', so we'll be able to get the correct
      //  "ProxyServerMediaSubsession" to handle the response.  (Note that responses come back in the same order as requests.))
      Boolean queueWasEmpty = proxyRTSPClient->fSetupQueueHead == NULL;
      if (queueWasEmpty) {
	proxyRTSPClient->fSetupQueueHead = this;
      } else {
	proxyRTSPClient->fSetupQueueTail->fNext = this;
      }
      proxyRTSPClient->fSetupQueueTail = this;

      // Hack: If there's already a pending "SETUP" request (for another track), don't send this track's "SETUP" right away, because
      // the server might not properly handle 'pipelined' requests.  Instead, wait until after previous "SETUP" responses come back.
      if (queueWasEmpty) {
	proxyRTSPClient->sendSetupCommand(fClientMediaSubsession, ::continueAfterSETUP,
					  False, proxyRTSPClient->fStreamRTPOverTCP, False, proxyRTSPClient->auth());
	++proxyRTSPClient->fNumSetupsDone;
	fHaveSetupStream = True;
      }
    } else {
      // This is a "SETUP" from a new client.  We know that there are no other currently active clients (otherwise we wouldn't
      // have been called here), so we know that the substream was previously "PAUSE"d.  Send "PLAY" downstream once again,
      // to resume the stream:
      if (!proxyRTSPClient->fLastCommandWasPLAY) { // so that we send only one "PLAY"; not one for each subsession
	proxyRTSPClient->sendPlayCommand(fClientMediaSubsession.parentSession(), NULL, -1.0f/*resume from previous point*/,
					 -1.0f, 1.0f, proxyRTSPClient->auth());
	proxyRTSPClient->fLastCommandWasPLAY = True;
      }
    }
  }

  estBitrate = fClientMediaSubsession.bandwidth();
  if (estBitrate == 0) estBitrate = 50; // kbps, estimate
  return fClientMediaSubsession.readSource();
}

void ProxyServerMediaSubsession::closeStreamSource(FramedSource* inputSource) {
  if (verbosityLevel() > 0) {
    envir() << *this << "::closeStreamSource()\n";
  }
  // Because there's only one input source for this 'subsession' (regardless of how many downstream clients are proxying it),
  // we don't close the input source here.  (Instead, we wait until *this* object gets deleted.)
  // However, because (as evidenced by this function having been called) we no longer have any clients accessing the stream,
  // then we "PAUSE" the downstream proxied stream, until a new client arrives:
  if (fHaveSetupStream) {
    ProxyServerMediaSession* const sms = (ProxyServerMediaSession*)fParentSession;
    ProxyRTSPClient* const proxyRTSPClient = sms->fProxyRTSPClient;
    if (proxyRTSPClient->fLastCommandWasPLAY) { // so that we send only one "PAUSE"; not one for each subsession
      proxyRTSPClient->sendPauseCommand(fClientMediaSubsession.parentSession(), NULL, proxyRTSPClient->auth());
      proxyRTSPClient->fLastCommandWasPLAY = False;
    }
  }
}

RTPSink* ProxyServerMediaSubsession
::createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource* inputSource) {
  if (verbosityLevel() > 0) {
    envir() << *this << "::createNewRTPSink()\n";
  }

  // Create (and return) the appropriate "RTPSink" object for our codec:
  char const* const codecName = fClientMediaSubsession.codecName();
  if (strcmp(codecName, "AC3") == 0 || strcmp(codecName, "EAC3") == 0) {
    return AC3AudioRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic,
				      fClientMediaSubsession.rtpTimestampFrequency()); 
  } else if (strcmp(codecName, "AMR") == 0 || strcmp(codecName, "AMR-WB") == 0) {
    Boolean isWideband = strcmp(codecName, "AMR-WB") == 0;
    return AMRAudioRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic,
				      isWideband, fClientMediaSubsession.numChannels());
  } else if (strcmp(codecName, "DV") == 0) {
    return DVVideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
  } else if (strcmp(codecName, "GSM") == 0) {
    return GSMAudioRTPSink::createNew(envir(), rtpGroupsock);
  } else if (strcmp(codecName, "H263-1998") == 0 || strcmp(codecName, "H263-2000") == 0) {
    return H263plusVideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic,
				      fClientMediaSubsession.rtpTimestampFrequency()); 
  } else if (strcmp(codecName, "H264") == 0) {
    return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic,
				       fClientMediaSubsession.fmtp_spropparametersets());
  } else if (strcmp(codecName, "MP4A-LATM") == 0) {
    return MPEG4LATMAudioRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic,
					    fClientMediaSubsession.rtpTimestampFrequency(),
					    fClientMediaSubsession.fmtp_config(),
					    fClientMediaSubsession.numChannels());
  } else if (strcmp(codecName, "MP4V-ES") == 0) {
    return MPEG4ESVideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic,
					  fClientMediaSubsession.rtpTimestampFrequency(),
					  fClientMediaSubsession.fmtp_profile_level_id(), fClientMediaSubsession.fmtp_config()); 
  } else if (strcmp(codecName, "MPA") == 0) {
    return MPEG1or2AudioRTPSink::createNew(envir(), rtpGroupsock);
  } else if (strcmp(codecName, "MPA-ROBUST") == 0) {
    return MP3ADURTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
  } else if (strcmp(codecName, "MPEG4-GENERIC") == 0) {
    return MPEG4GenericRTPSink::createNew(envir(), rtpGroupsock,
					  rtpPayloadTypeIfDynamic, fClientMediaSubsession.rtpTimestampFrequency(),
					  fClientMediaSubsession.mediumName(), fClientMediaSubsession.fmtp_mode(),
					  fClientMediaSubsession.fmtp_config(), fClientMediaSubsession.numChannels());
  } else if (strcmp(codecName, "MPV") == 0) {
    return MPEG1or2VideoRTPSink::createNew(envir(), rtpGroupsock);
  } else if (strcmp(codecName, "T140") == 0) {
    return T140TextRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
  } else if (strcmp(codecName, "VORBIS") == 0) {
    return VorbisAudioRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic,
					 fClientMediaSubsession.rtpTimestampFrequency(), fClientMediaSubsession.numChannels(),
					 fClientMediaSubsession.fmtp_config()); 
  } else if (strcmp(codecName, "VP8") == 0) {
    return VP8VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
  } else if (strcmp(codecName, "QCELP") == 0 ||
	     strcmp(codecName, "H261") == 0 ||
	     strcmp(codecName, "H263-1998") == 0 || strcmp(codecName, "H263-2000") == 0 ||
	     strcmp(codecName, "X-QT") == 0 || strcmp(codecName, "X-QUICKTIME") == 0) {
    // This codec requires a specialized RTP payload format; however, we don't yet have an appropriate "RTPSink" subclass for it:
    if (verbosityLevel() > 0) {
      envir() << "\treturns NULL (because we don't have a \"RTPSink\" subclass for this RTP payload format)\n";
    }
    return NULL;
  } else {
    // This codec is assumed to have a simple RTP paylaod format that can be implemented just with a "SimpleRTPSink":
    Boolean allowMultipleFramesPerPacket = True; // by default
    Boolean doNormalMBitRule = True; // by default
    // Some codecs change the above default parameters:
    if (strcmp(codecName, "MP2T") == 0) {
      doNormalMBitRule = False; // no RTP 'M' bit
    }
    return SimpleRTPSink::createNew(envir(), rtpGroupsock,
				    rtpPayloadTypeIfDynamic, fClientMediaSubsession.rtpTimestampFrequency(),
				    fClientMediaSubsession.mediumName(), fClientMediaSubsession.codecName(),
				    fClientMediaSubsession.numChannels(), allowMultipleFramesPerPacket, doNormalMBitRule);
  }
}

void ProxyServerMediaSubsession::subsessionByeHandler(void* clientData) {
  ((ProxyServerMediaSubsession*)clientData)->subsessionByeHandler();
}

void ProxyServerMediaSubsession::subsessionByeHandler() {
  if (verbosityLevel() > 0) {
    envir() << *this << ": received RTCP \"BYE\"\n";
  }

  // This "BYE" signals that our input source has (effectively) closed, so handle this accordingly:
  FramedSource::handleClosure(fClientMediaSubsession.readSource());
}

void ProxyServerMediaSubsession::reset() {
  fNext = NULL;
  fHaveSetupStream = False;
}
