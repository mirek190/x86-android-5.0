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

#include "kxmppthread.h"

#include "talk/xmpp/xmppclientsettings.h"
#include "talk/xmpp/xmppauth.h"

//For Debugger
#ifdef LOGGING
#include <android/log.h>
#include "talk/base/logging.h"
#include <iostream>
#include <iomanip>
#endif
#include "callclient.h"
#include "conductor.h"

namespace {

const uint32 MSG_LOGIN = 1;
const uint32 MSG_DISCONNECT = 2;
const uint32 MSG_MAKE_CALL = 3;
const uint32 MSG_HANG_UP = 4;
const uint32 MSG_ANSWER_CALL = 5;
const uint32 MSG_REJECT_CALL = 6;
const uint32 MSG_TERMINATE = 7;
const uint32 MSG_SEND_OFFER = 8;
const uint32 MSG_SEND_ACCEPT = 9;
const uint32 MSG_SEND_CANDIDATE = 10;
const uint32 MSG_ADD_CANDIDATE = 11;
const uint32 MSG_RECEIVE_ACCEPT = 12;
const uint32 MSG_RECEIVE_REJECT = 13;
const uint32 MSG_UPDATE_ICE = 14;

struct LoginData: public talk_base::MessageData {
  LoginData(const buzz::XmppClientSettings& s) : xcs(s) {}
  virtual ~LoginData() {}

  buzz::XmppClientSettings xcs;
};

struct CallData: public talk_base::MessageData {
  CallData(const std::string& s) : user(s) {}
  virtual ~CallData() {}

  std::string user;
};

struct OfferData: public talk_base::MessageData {
  OfferData(const std::string &s, const cricket::SessionDescription *d) : user(s), desc(d){ }
  virtual ~OfferData() {}

  std::string user;
  const cricket::SessionDescription *desc;
};

struct AcceptData: public talk_base::MessageData {
  AcceptData(const cricket::SessionDescription *d) : desc(d){ }
  virtual ~AcceptData() {}

  const cricket::SessionDescription *desc;
};

struct AnswerData: public talk_base::MessageData {
  AnswerData(bool v, bool a) : video(v), audio(a){ }
  virtual ~AnswerData() {}

  bool video;
  bool audio;
};

struct CandidateData: public talk_base::MessageData {
  CandidateData(const webrtc::IceCandidateInterface* c) {
    candidate = webrtc::CreateIceCandidate(c->sdp_mid(), c->sdp_mline_index(), c->candidate());
  }
  CandidateData(const std::string& content_name, const cricket::Candidate& c) {
    candidate = webrtc::CreateIceCandidate(content_name, 0, c);
  }
  virtual ~CandidateData() {}

  const webrtc::IceCandidateInterface* candidate;
};

struct IceServerData: public talk_base::MessageData {
  IceServerData(const webrtc::PeerConnectionInterface::IceServers *iceservers)
      : iceservers_(iceservers) { }
  const webrtc::PeerConnectionInterface::IceServers *iceservers_;
};

} // namespace
#ifdef LOGGING
class DebugLog : public sigslot::has_slots<> {
public:
    DebugLog() :
    debug_input_buf_(NULL), debug_input_len_(0), debug_input_alloc_(0),
    debug_output_buf_(NULL), debug_output_len_(0), debug_output_alloc_(0),
    censor_password_(false)
    {}
    char * debug_input_buf_;
    int debug_input_len_;
    int debug_input_alloc_;
    char * debug_output_buf_;
    int debug_output_len_;
    int debug_output_alloc_;
    bool censor_password_;

    void Input(const char * data, int len) {
        if (debug_input_len_ + len > debug_input_alloc_) {
            char * old_buf = debug_input_buf_;
            debug_input_alloc_ = 4096;
            while (debug_input_alloc_ < debug_input_len_ + len) {
                debug_input_alloc_ *= 2;
            }
            debug_input_buf_ = new char[debug_input_alloc_];
            memcpy(debug_input_buf_, old_buf, debug_input_len_);
            delete[] old_buf;
        }
        memcpy(debug_input_buf_ + debug_input_len_, data, len);
        debug_input_len_ += len;
        DebugPrint(debug_input_buf_, &debug_input_len_, false);
    }

    void Output(const char * data, int len) {
        if (debug_output_len_ + len > debug_output_alloc_) {
            char * old_buf = debug_output_buf_;
            debug_output_alloc_ = 4096;
            while (debug_output_alloc_ < debug_output_len_ + len) {
                debug_output_alloc_ *= 2;
            }
            debug_output_buf_ = new char[debug_output_alloc_];
            memcpy(debug_output_buf_, old_buf, debug_output_len_);
            delete[] old_buf;
        }
        memcpy(debug_output_buf_ + debug_output_len_, data, len);
        debug_output_len_ += len;
        DebugPrint(debug_output_buf_, &debug_output_len_, true);
    }

    static bool IsAuthTag(const char * str, size_t len) {
        if (str[0] == '<' && str[1] == 'a' &&
            str[2] == 'u' &&
            str[3] == 't' &&
            str[4] == 'h' &&
            str[5] <= ' ') {
            std::string tag(str, len);

            if (tag.find("mechanism") != std::string::npos)
                return true;
        }
        return false;
    }

    void DebugPrint(char * buf, int * plen, bool output) {
        int len = *plen;
        if (len > 0) {
            time_t tim = time(NULL);
            struct tm * now = localtime(&tim);
            if (now) {
                char *time_string = asctime(now);
                if (time_string) {
                    size_t time_len = strlen(time_string);
                    if (time_len > 0) {
                        time_string[time_len-1] = 0;    // trim off terminating \n
                    }
                    LOG(INFO) << (output ? "SEND >>>>>>>>>>>>>>>>" : "RECV <<<<<<<<<<<<<<<<") << " : " << time_string;
                }
            }

            bool indent;
            int start = 0, nest = 3;
            for (int i = 0; i < len; i += 1) {
                if (buf[i] == '>') {
                    if ((i > 0) && (buf[i-1] == '/')) {
                        indent = false;
                    } else if ((start + 1 < len) && (buf[start + 1] == '/')) {
                        indent = false;
                        nest -= 2;
                    } else {
                        indent = true;
                    }

                    // Output a tag
                    LOG(INFO) << std::setw(nest) << " " << std::string(buf + start, i + 1 - start);

                    if (indent)
                        nest += 2;

                    // Note if it's a PLAIN auth tag
                    if (IsAuthTag(buf + start, i + 1 - start)) {
                        censor_password_ = true;
                    }

                    // incr
                    start = i + 1;
                }

                if (buf[i] == '<' && start < i) {
                    if (censor_password_) {
                        LOG(INFO) << std::setw(nest) << " " << "## TEXT REMOVED ##";
                        censor_password_ = false;
                    } else {
                        LOG(INFO) << std::setw(nest) << " " << std::string(buf + start, i - start);
                    }
                    start = i;
                }
            }
            len = len - start;
            memcpy(buf, buf + start, len);
            *plen = len;
        }
    }
};

static DebugLog debug_log_;
#endif //LOGGING

bool terminated;
pthread_mutex_t KXmppThread::mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t KXmppThread::cond = PTHREAD_COND_INITIALIZER;

KXmppThread::KXmppThread() {
  LOG(INFO) << "KXmppThread::KXmppThread()";
  Init();
}

void KXmppThread::Init() {
  LOG(INFO) << "KXmppThread::Init()";
  terminated = false;
  pump_ = new buzz::XmppPump(this);
  //Call Client
  std::string caps_node = "http://www.google.com/xmpp/client/caps";
  std::string caps_ver = "1.0";

  int32 portallocator_flags = 0;
  cricket::SignalingProtocol initial_protocol = cricket::PROTOCOL_JINGLE;

  client_ = new GCallClient(pump_->client(), this, caps_node, caps_ver);
  client_->SetAllowLocalIps(false);
  client_->SetAutoAccept(false);
  client_->SetPortAllocatorFlags(portallocator_flags);
  client_->SetInitialProtocol(initial_protocol);
  client_->SetRender(false);
  client_->SetDataChannelEnabled(false);

  conductor_ = new talk_base::RefCountedObject<Conductor>(this, client_);
#ifdef LOGGING
  pump_->client()->SignalLogInput.connect(&debug_log_, &DebugLog::Input);
  pump_->client()->SignalLogOutput.connect(&debug_log_, &DebugLog::Output);
#endif
}

KXmppThread::~KXmppThread() {
  LOG(INFO) << "KXmppThread::~KXmppThread()";
}

void KXmppThread::ProcessMessages(int cms) {
  talk_base::Thread::ProcessMessages(cms);
}

void KXmppThread::Login(const buzz::XmppClientSettings& xcs) {
  LOG(INFO) << "KXmppThread::Login()";
  Post(this, MSG_LOGIN, new LoginData(xcs));
}

void KXmppThread::Disconnect() {
  LOG(INFO) << "KXmppThread::Disconnect()";
  Post(this, MSG_DISCONNECT);
}

void KXmppThread::HangUp() {
  LOG(INFO) << "KXmppThread::HangUp()";
  Post(this, MSG_HANG_UP);
}

void KXmppThread::MakeCall(std::string username, bool video, bool audio) {
  LOG(INFO) << "KXmppThread::MakeCall()";
  buzz::Jid jid(username);
  if(!client_->CheckCallee(jid)) return;
  if (pump_ == NULL) {
    Init();
  }
  conductor_->ConnectToPeer(username, video, audio);
  //Post(this, MSG_MAKE_CALL, new CallData(username));
}

void KXmppThread::AnswerCall(bool video, bool audio) {
  LOG(INFO) << "KXmppThread::AnswerCall()";
  Post(this, MSG_ANSWER_CALL, new AnswerData(video, audio));
}

void KXmppThread::SendIceCandidate(const webrtc::IceCandidateInterface *c) {
  if(client_->GetSession()==NULL) return;
  LOG(INFO) << "KXmppThread::SendIceCandidate()";
  Post(this, MSG_SEND_CANDIDATE, new CandidateData(c));
}

void KXmppThread::AddCandidate(const std::string& content_name,
                            const cricket::Candidate& candidate)
{
  LOG(INFO) << "KXmppThread::AddCandidate";
  Post(this, MSG_ADD_CANDIDATE, new CandidateData(content_name, candidate));
}

void KXmppThread::RejectCall() {
  LOG(INFO) << "KXmppThread::RejectCall()";
  Post(this, MSG_REJECT_CALL);
}

void KXmppThread::SendOffer(std::string user, const cricket::SessionDescription *desc) {
  LOG(INFO) << "KXmppThread::SendOffer()";
  Post(this, MSG_SEND_OFFER, new OfferData(user, desc));
}

void KXmppThread::SendAccept(const cricket::SessionDescription *desc) {
  LOG(INFO) << "KXmppThread::SendAccept()";
  Post(this, MSG_SEND_ACCEPT, new AcceptData(desc));
}

void KXmppThread::ReceiveAccept(const cricket::SessionDescription* desc) {
  LOG(INFO) << "KXmppThread::ReceiveAnswer";
  Post(this, MSG_RECEIVE_ACCEPT, new AcceptData(desc));
}

void KXmppThread::ReceiveReject(const cricket::SessionDescription* desc) {
  LOG(INFO) << "KXmppThread::ReceiveReject()";
  Post(this, MSG_RECEIVE_REJECT);
}

bool KXmppThread::Terminate() {
  LOG(INFO) << "KXmppThread::Terminate()";
  Post(this, MSG_TERMINATE);
  pthread_mutex_lock(&mtx);
  while (!terminated) {
    pthread_cond_wait(&cond, &mtx);
  }
  pthread_mutex_unlock(&mtx);
  return terminated;
}

void KXmppThread::SetCamera(int deviceId, std::string &deviceUniqueName) {
  conductor_->SetCamera(deviceId, deviceUniqueName);
}

void KXmppThread::SetImageOrientation(int degrees) {
  LOG(INFO) << "KXmppThread::SetImageOrientation(" << degrees << ")";
  conductor_->SetImageOrientation(degrees);
}

bool KXmppThread::SetVideo(bool enable) {
  LOG(INFO) << "KXmppThread::SetVideo()";
  return(conductor_->SetVideo(enable));
}

bool KXmppThread::SetVoice(bool enable) {
  LOG(INFO) << "KXmppThread::SetVoice()";
  return(conductor_->SetVoice(enable));
}

std::string KXmppThread::GetCaller() {
  LOG(INFO) << "KXmppThread::GetCaller()";
  std::string toReturn;
  if(client_){
    toReturn = client_->GetCaller();
  }
  return toReturn;
}

bool KXmppThread::IsCalling() {
  return conductor_->IsCalling();
}

void KXmppThread::UpdateIceServers(const webrtc::PeerConnectionInterface::IceServers *iceservers)
{
  LOG(INFO) << "KXmppThread::UpdateIceServers()";
  Post(this, MSG_UPDATE_ICE, new IceServerData(iceservers));
}

void KXmppThread::OnStateChange(buzz::XmppEngine::State state) {
}

void KXmppThread::OnMessage(talk_base::Message* pmsg) {
  if (pmsg->message_id == MSG_LOGIN) {
    ASSERT(pmsg->pdata != NULL);
    LoginData* data = reinterpret_cast<LoginData*>(pmsg->pdata);
    pump_->DoLogin(data->xcs, new buzz::XmppSocket(data->xcs.use_tls()), NULL);
    delete data;
  } else if (pmsg->message_id == MSG_DISCONNECT) {
    pump_->DoDisconnect();
  } else if (pmsg->message_id == MSG_HANG_UP){
    if(client_){
      client_->HangUp();
    }
    if(conductor_) {
      conductor_->Close();
    }
  } else if (pmsg->message_id == MSG_ANSWER_CALL){
    AnswerData *data = reinterpret_cast<AnswerData*>(pmsg->pdata);
    if(client_){
      conductor_->OnInitiateMessage(data->video, data->audio);
    }
  } else if (pmsg->message_id == MSG_SEND_OFFER) {
    OfferData* data = reinterpret_cast<OfferData*>(pmsg->pdata);
    if (client_) {
      client_->MakeCallTo(data->user, data->desc);
    }
    delete data;
  } else if (pmsg->message_id == MSG_SEND_ACCEPT) {
    AcceptData* data = reinterpret_cast<AcceptData*>(pmsg->pdata);
    if (client_) {
      client_->Accept(data->desc);
    }
    delete data;
  } else if (pmsg->message_id == MSG_RECEIVE_ACCEPT) {
    AcceptData* data = reinterpret_cast<AcceptData*>(pmsg->pdata);
    if (conductor_) {
      conductor_->ReceiveAccept(data->desc);
    }
    delete data;
  } else if (pmsg->message_id == MSG_SEND_CANDIDATE) {
    CandidateData* data = reinterpret_cast<CandidateData*>(pmsg->pdata);
    if (client_) {
      client_->SendCandidate(data->candidate);
    }
    delete data;
  } else if (pmsg->message_id == MSG_ADD_CANDIDATE) {
    CandidateData* data = reinterpret_cast<CandidateData*>(pmsg->pdata);
    if (conductor_) {
      conductor_->AddIceCandidate(data->candidate);
    }
    delete data;
  } else if (pmsg->message_id == MSG_REJECT_CALL){
    if(client_){
      client_->Reject();
      conductor_->ClearCandidates();
    }
  } else if (pmsg->message_id == MSG_RECEIVE_REJECT){
    if(client_){
      client_->ReceiveReject();
    }
  } else if (pmsg->message_id == MSG_TERMINATE){
    if(client_){
      conductor_->Close();
      delete client_;
      delete pump_;
      client_ = NULL;
      pump_ = NULL;
    }
    terminated = true;
    pthread_cond_signal(&cond);
  } else if (pmsg->message_id == MSG_UPDATE_ICE){
    IceServerData* data = reinterpret_cast<IceServerData*>(pmsg->pdata);
    if (conductor_) {
      conductor_->UpdateIceServers(data->iceservers_);
    } else {
      delete data;
    }
  } else {
    ASSERT(false);
  }
}
