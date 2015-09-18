/*
 * libjingle
 * Copyright 2004--2005, Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TALK_EXAMPLES_LOGIN_XMPPTHREAD_H_
#define TALK_EXAMPLES_LOGIN_XMPPTHREAD_H_

#include "talk/base/thread.h"
#include "talk/xmpp/xmpppump.h"
#include "talk/xmpp/xmppsocket.h"
#include "talk/xmpp/xmppclientsettings.h"
#include "talk/xmpp/xmppengine.h"
#include "callclient.h"
#include "talk/p2p/base/sessiondescription.h"
#include "talk/app/webrtc/jsep.h"
#include "talk/app/webrtc/peerconnectioninterface.h"

class Conductor;

class KXmppThread:
    public talk_base::Thread, buzz::XmppPumpNotify, talk_base::MessageHandler {
public:
  KXmppThread();
  ~KXmppThread();

  buzz::XmppClient* client() { return pump_->client(); }

  void ProcessMessages(int cms);

  void Login(const buzz::XmppClientSettings & xcs);
  void Disconnect();
  void MakeCall(std::string user, bool video, bool audio);
  void SendOffer(std::string user, const cricket::SessionDescription* desc);
  void SendAccept(const cricket::SessionDescription* desc);
  void ReceiveAccept(const cricket::SessionDescription* desc);
  void HangUp();
  void RejectCall();
  void AnswerCall(bool video, bool audio);
  void SendIceCandidate(const webrtc::IceCandidateInterface* c);
  void AddCandidate(const std::string& content_name, const cricket::Candidate& candidate);
  void ReceiveReject(const cricket::SessionDescription* desc);
  bool Terminate();
  void SetCamera(int deviceId, std::string &deviceUniqueName);
  void SetImageOrientation(int degrees);
  bool SetVideo(bool enable);
  bool SetVoice(bool enable);
  std::string GetCaller();
  bool IsCalling();

  GCallClient* client_;
  void UpdateIceServers(const webrtc::PeerConnectionInterface::IceServers *iceservers);
private:
  buzz::XmppPump* pump_;
  talk_base::scoped_refptr<Conductor> conductor_;
  static pthread_mutex_t mtx;
  static pthread_cond_t cond;

  void Init();
  void OnStateChange(buzz::XmppEngine::State state);
  void OnMessage(talk_base::Message* pmsg);
};
#endif  // TALK_EXAMPLES_LOGIN_XMPPTHREAD_H_
