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

#include "callclient.h"
#include "org_webrtc_videoP2P_VideoClient.h"
#include <jni.h>
#include <android/log.h>

#include <string>

#include "talk/base/helpers.h"
#include "talk/base/logging.h"
#include "talk/base/network.h"
#include "talk/base/socketaddress.h"
#include "talk/base/stringencode.h"
#include "talk/base/stringutils.h"
#include "talk/base/thread.h"
#include "presencepushtask.h"
#include "talk/xmpp/presenceouttask.h"
#include "friendinvitesendtask.h"
#include "gsessionmanagertask.h"
#include "gsessionmanager.h"
#include "talk/media/devices/devicemanager.h"
#include "talk/media/base/mediacommon.h"
#include "talk/media/base/mediaengine.h"
#include "talk/media/base/rtpdataengine.h"
#include "talk/session/media/mediamessages.h"
#include "talk/media/base/screencastid.h"
#include "talk/media/devices/videorendererfactory.h"
#include "talk/media/base/videorenderer.h"
#include "talk/xmpp/constants.h"
#include "talk/xmpp/jingleinfotask.h"
#include "talk/xmpp/pingtask.h"
#include "talk/p2p/base/constants.h"
#include "videoclient.h"
#include "third_party/webrtc/modules/video_render/include/video_render.h"
#include "gsession.h"
#include "conductor.h"
#include "kxmppthread.h"

using namespace webrtc;

namespace {

// Must be period >= timeout.
const uint32 kPingPeriodMillis = 10000;
const uint32 kPingTimeoutMillis = 10000;

#if LOGGING
const char* DescribeStatus(buzz::PresenceStatus::Show show, const std::string& desc) {
  switch (show) {
  case buzz::PresenceStatus::SHOW_XA:      return desc.c_str();
  case buzz::PresenceStatus::SHOW_ONLINE:  return "online";
  case buzz::PresenceStatus::SHOW_AWAY:    return "away";
  case buzz::PresenceStatus::SHOW_DND:     return "do not disturb";
  case buzz::PresenceStatus::SHOW_CHAT:    return "ready to chat";
  default:                         return "offline";
  }
}
#endif

}  // namespace

static void *remoteSurfaceView_ = NULL;
static void *remoteSurface_ = NULL;
static JNIEnv *gRenderEnv=NULL;
static JNIEnv *gSizeEnv=NULL;
extern JavaVM *gJavaVM;
static jobject gContext;

JNIEXPORT jint JNICALL Java_org_webrtc_videoP2P_VideoClient_SetRemoteSurfaceView(
    JNIEnv *env, jobject context, jobject remoteSurface)
{
    gContext = reinterpret_cast<jobject>(env->NewGlobalRef(context));
    if (remoteSurface == NULL) return 0;
    remoteSurfaceView_ = (void*)env->NewGlobalRef(remoteSurface);
    LOG(INFO) << "SetRemoteSurfaceView to " << remoteSurfaceView_;
    return 0;
}

void* GetRemoteSurfaceView() { return remoteSurfaceView_; }

JNIEXPORT jint JNICALL Java_org_webrtc_videoP2P_VideoClient_SetRemoteSurface(
    JNIEnv *env, jobject context, jobject remoteSurface)
{
    gContext = reinterpret_cast<jobject>(env->NewGlobalRef(context));
    if (remoteSurface == NULL) return 0;
    remoteSurface_ = (void*)env->NewGlobalRef(remoteSurface);
    LOG(INFO) << "SetRemoteSurface to " << remoteSurface_;
    return 0;
}
void* GetRemoteSurface() { return remoteSurface_; }

GCallClient::GCallClient(buzz::XmppClient* xmpp_client, KXmppThread* kxmpp_thread,
                       const std::string& caps_node, const std::string& version)
    : xmpp_client_(xmpp_client),
      kxmpp_thread_(kxmpp_thread),
      incoming_call_(false),
      auto_accept_(false),
      render_(true),
      data_channel_enabled_(false),
      roster_(new RosterMap),
      portallocator_flags_(0),
      allow_local_ips_(false),
      initial_protocol_(cricket::PROTOCOL_JINGLE),
      local_renderer_(NULL),
      remote_renderer_(NULL) {
  xmpp_client_->SignalStateChange.connect(this, &GCallClient::OnStateChange);
  my_status_.set_caps_node(caps_node);
  my_status_.set_version(version);

  y_plane_ = NULL;
  u_plane_ = NULL;
  v_plane_ = NULL;
  index_ = 0;
  count_ = 0;
  buffer_initialized_ = false;
  pthread_mutex_init(&mutex_, NULL);
}

GCallClient::~GCallClient() {
  pthread_mutex_destroy(&mutex_);
#if 0
  if (session_manager_) {
    session_manager_->RemoveClient(cricket::NS_JINGLE_RTP);
  }
#endif
  //TODO: Investigate which of these can actually
  //be deleted. I think tasks are automatically
  //cleaned up.
  //delete session_manager_task_;
  //delete session_manager_;
  //delete presence_push_;
  //delete presence_out_;
  //delete friend_invite_send_;
  delete roster_;
}

#ifdef LOGGING
const std::string GCallClient::strerror(buzz::XmppEngine::Error err) {
  switch (err) {
    case  buzz::XmppEngine::ERROR_NONE:
      return "";
    case  buzz::XmppEngine::ERROR_XML:
      return "Malformed XML or encoding error";
    case  buzz::XmppEngine::ERROR_STREAM:
      return "XMPP stream error";
    case  buzz::XmppEngine::ERROR_VERSION:
      return "XMPP version error";
    case  buzz::XmppEngine::ERROR_UNAUTHORIZED:
      return "User is not authorized (Check your username and password)";
    case  buzz::XmppEngine::ERROR_TLS:
      return "TLS could not be negotiated";
    case  buzz::XmppEngine::ERROR_AUTH:
      return "Authentication could not be negotiated";
    case  buzz::XmppEngine::ERROR_BIND:
      return "Resource or session binding could not be negotiated";
    case  buzz::XmppEngine::ERROR_CONNECTION_CLOSED:
      return "Connection closed by output handler.";
    case  buzz::XmppEngine::ERROR_DOCUMENT_CLOSED:
      return "Closed by </stream:stream>";
    case  buzz::XmppEngine::ERROR_SOCKET:
      return "Socket error";
    default:
      return "Unknown error";
  }
}
#endif

const short GCallClient::strerrorcode(buzz::XmppEngine::Error err) {
  switch (err) {
    case  buzz::XmppEngine::ERROR_NONE:
      return videoP2P::XMPPERROR_NONE;
    case  buzz::XmppEngine::ERROR_XML:
      return videoP2P::XMPPERROR_XML;
    case  buzz::XmppEngine::ERROR_STREAM:
      return videoP2P::XMPPERROR_STREAM;
    case  buzz::XmppEngine::ERROR_VERSION:
      return videoP2P::XMPPERROR_VERSION;
    case  buzz::XmppEngine::ERROR_UNAUTHORIZED:
      return videoP2P::XMPPERROR_UNAUTH;
    case  buzz::XmppEngine::ERROR_TLS:
      return videoP2P::XMPPERROR_TLS;
    case  buzz::XmppEngine::ERROR_AUTH:
      return videoP2P::XMPPERROR_AUTH;
    case  buzz::XmppEngine::ERROR_BIND:
      return videoP2P::XMPPERROR_BIND;
    case  buzz::XmppEngine::ERROR_CONNECTION_CLOSED:
      return videoP2P::XMPPERROR_CONN_CLOSED;
    case  buzz::XmppEngine::ERROR_DOCUMENT_CLOSED:
      return videoP2P::XMPPERROR_DOC_CLOSED;
    case  buzz::XmppEngine::ERROR_SOCKET:
      return videoP2P::XMPPERROR_SOCK_ERR;
    default:
      return videoP2P::XMPPERROR_UNKNOWN;
  }
}

void GCallClient::StartLocalRenderer(webrtc::VideoTrackInterface* local_video) {
  LOG(INFO) << "GCallClient::StartLocalRenderer";
  VideoRenderer *render = new VideoRenderer(this, local_video, VideoRenderer::LOCAL);
  local_renderer_.reset(render);
}

void GCallClient::StartRemoteRenderer(webrtc::VideoTrackInterface* remote_video) {
  LOG(INFO) << "GCallClient::StartRemoteRender";
  VideoRenderer *render = new VideoRenderer(this, remote_video, VideoRenderer::REMOTE);
  remote_renderer_.reset(render);
}

void GCallClient::OnStateChange(buzz::XmppEngine::State state) {
  switch (state) {
  case buzz::XmppEngine::STATE_START:
    LOG(INFO) <<"connecting...";
    callback_handler(videoP2P::XMPPENGINE_CALLBACK, videoP2P::XMPPENGINE_START);
    break;

  case buzz::XmppEngine::STATE_OPENING:
    LOG(INFO) <<"logging in...";
    callback_handler(videoP2P::XMPPENGINE_CALLBACK, videoP2P::XMPPENGINE_OPENING);
    break;

  case buzz::XmppEngine::STATE_OPEN:
    LOG(INFO) <<"logged in...";
    InitMedia();
    InitPresence();
    InitIce();
    callback_handler(videoP2P::XMPPENGINE_CALLBACK, videoP2P::XMPPENGINE_OPEN);
    break;

  case buzz::XmppEngine::STATE_NONE:
    LOG(INFO) << "state set to none.";
    break;

  case buzz::XmppEngine::STATE_CLOSED:
    buzz::XmppEngine::Error error = xmpp_client_->GetError(NULL);
    LOG(INFO) <<"logged out... " << strerror(error).c_str();
    callback_handler(videoP2P::XMPPENGINE_CALLBACK, videoP2P::XMPPENGINE_CLOSED);
    callback_handler(videoP2P::XMPPERROR_CALLBACK, strerrorcode(error));
    break;
  }
}

void GCallClient::InitMedia() {
  LOG(INFO) <<"GCallClient::InitMedia() +";
  std::string client_unique = xmpp_client_->jid().Str();
  LOG(INFO) <<"GCallClient::InitMedia() user=" << client_unique;
  talk_base::InitRandom(client_unique.c_str(), client_unique.size());

  session_manager_ = new cricket::GSessionManager();
  session_manager_->SignalSessionCreate.connect(
      this, &GCallClient::OnSessionCreate);
  session_manager_->AddClient(cricket::NS_JINGLE_RTP, this);

  session_manager_task_ =
      new cricket::GSessionManagerTask(xmpp_client_, session_manager_);
  session_manager_task_->EnableOutgoingMessages();
  session_manager_task_->Start();

  LOG(INFO) << "GCallClient::InitMedia() -";
}

void GCallClient::OnSessionCreate(cricket::GSession* session, bool initiate) {
  //jrr session->set_allow_local_ips(allow_local_ips_);
  session->set_current_protocol(initial_protocol_);

  session->SignalState.connect(this, &GCallClient::OnSessionState);
  session->SignalOnRemoteCandidate.connect(this, &GCallClient::OnRemoteCandidate);
}

std::string GCallClient::GetCaller(){
  std::string toReturn;
  if(session_){
    buzz::Jid jid(session_->remote_name());
    toReturn = jid.Str();
  }
  return toReturn;
}

void GCallClient::SetVideoRendererRotation(int deg) {
  pthread_mutex_lock(&mutex_);
  if(remote_renderer_.get() != NULL) {
    remote_renderer_->SetVideoRendererRotation(deg);
  }
  pthread_mutex_unlock(&mutex_);
}

void GCallClient::SendCandidate(const webrtc::IceCandidateInterface* candidate) {
  LOG(INFO) <<"GCallClient::SendCandidate";
  if (session_) {
    session_->SendCandidate(candidate);
  }
}

void GCallClient::OnRemoteCandidate(const std::string& content_name, const cricket::Candidate& candidate) {
  LOG(INFO) << "GCallClient::OnRemoteCandidate";
  kxmpp_thread_->AddCandidate(content_name, candidate);
}

void GCallClient::OnSessionState(cricket::BaseGSession* base_session,
                                cricket::BaseGSession::State state) {
  cricket::GSession* session = static_cast<cricket::GSession*>(base_session);
  if (state == cricket::GSession::STATE_RECEIVEDINITIATE) {
    buzz::Jid jid(session->remote_name());
    LOG(INFO) <<"Incoming call from " << jid.Str().c_str();
    session_ = session;
    incoming_call_ = true;
    callback_handler(videoP2P::CALL_STATE_CALLBACK, videoP2P::CALL_INCOMING);
  } else if (state == cricket::GSession::STATE_SENTINITIATE) {
    LOG(INFO) <<"calling...";
    callback_handler(videoP2P::CALL_STATE_CALLBACK, videoP2P::CALL_CALLING);
  } else if (state == cricket::GSession::STATE_RECEIVEDACCEPT) {
    LOG(INFO) <<"call answered";
    kxmpp_thread_->ReceiveAccept(session->remote_description());
    callback_handler(videoP2P::CALL_STATE_CALLBACK, videoP2P::CALL_ANSWERED);
  } else if (state == cricket::GSession::STATE_RECEIVEDREJECT) {
    callback_handler(videoP2P::CALL_STATE_CALLBACK, videoP2P::CALL_REJECTED);
    LOG(INFO) <<"call not answered";
  } else if (state == cricket::GSession::STATE_INPROGRESS) {
    callback_handler(videoP2P::CALL_STATE_CALLBACK, videoP2P::CALL_INPROGRESS);
    LOG(INFO) <<"call in progress";
  } else if (state == cricket::GSession::STATE_RECEIVEDTERMINATE) {
    callback_handler(videoP2P::CALL_STATE_CALLBACK, videoP2P::CALL_RECEIVED_TERMINATE);
	kxmpp_thread_->HangUp();
    LOG(INFO) <<"other side hung up";
  }
}

void SetMediaCaps(int media_caps, buzz::PresenceStatus* status) {
  status->set_voice_capability((media_caps & cricket::AUDIO_RECV) != 0);
  status->set_video_capability((media_caps & cricket::VIDEO_RECV) != 0);
  status->set_camera_capability((media_caps & cricket::VIDEO_SEND) != 0);
}

void SetCaps(int media_caps, buzz::PresenceStatus* status) {
  status->set_know_capabilities(true);
  SetMediaCaps(media_caps, status);
}

void SetAvailable(const buzz::Jid& jid, buzz::PresenceStatus* status) {
  status->set_jid(jid);
  status->set_available(true);
  status->set_show(buzz::PresenceStatus::SHOW_ONLINE);
}

void GCallClient::InitPresence() {
  presence_push_ = new buzz::PresencePushTask(xmpp_client_, this);
  presence_push_->SignalStatusUpdate.connect(
    this, &GCallClient::OnStatusUpdate);
  presence_push_->Start();

  presence_out_ = new buzz::PresenceOutTask(xmpp_client_);
  SetAvailable(xmpp_client_->jid(), &my_status_);
  my_status_.set_know_capabilities(true);
  my_status_.set_voice_capability(true);
  my_status_.set_video_capability(true);
  my_status_.set_camera_capability(true);
  SendStatus(my_status_);
  presence_out_->Start();

  friend_invite_send_ = new buzz::FriendInviteSendTask(xmpp_client_);
  friend_invite_send_->Start();

  StartXmppPing();
}

void GCallClient::StartXmppPing() {
  buzz::PingTask* ping = new buzz::PingTask(
      xmpp_client_, talk_base::Thread::Current(),
      kPingPeriodMillis, kPingTimeoutMillis);
  ping->SignalTimeout.connect(this, &GCallClient::OnPingTimeout);
  ping->Start();
}

void GCallClient::OnPingTimeout() {
  LOG(LS_WARNING) << "KXMPP Ping timeout. Will keep trying...";
  StartXmppPing();
}

void GCallClient::InitIce() {
  // TODO: Does JingleInfoTask need cleanup?
  buzz::JingleInfoTask* jit = new buzz::JingleInfoTask(xmpp_client_);
  jit->SignalJingleInfo.connect(this, &GCallClient::OnJingleInfo);
  jit->Start();
  jit->RefreshJingleInfoNow();
}

void GCallClient::SendStatus(const buzz::PresenceStatus& status) {
  presence_out_->Send(status);
}

void GCallClient::OnStatusUpdate(const buzz::PresenceStatus& status) {
  RosterItem item;
  item.jid = status.jid();
  item.show = status.show();
  item.status = status.status();
  item.pendingRemoval = false;

  if(item.jid==xmpp_client_->jid()) {
    LOG(INFO) << "Ignoring roster myself";
    return;
  }
  std::string key = item.jid.Str();

  if (status.available() && status.voice_capability() && status.video_capability()) {
    LOG(INFO) <<"Adding to roster: " << key.c_str();
    (*roster_)[key] = item;
    // TODO: Make some of these constants.
    callback_handler_ext(videoP2P::PRESENCE_STATE_CALLBACK, videoP2P::CALL_PRESENCE_DETECTED, key.c_str());

  } else {
    RosterMap::iterator iter = roster_->find(key);
    if (iter != roster_->end()) {
      if(kxmpp_thread_->IsCalling()) {
        LOG(INFO) <<"Calling... Skip removing roster";
        iter->second.pendingRemoval = true;
        return;
      } else {
        LOG(INFO) <<"Removing from roster: " << key.c_str();
        roster_->erase(iter);
        callback_handler_ext(videoP2P::PRESENCE_STATE_CALLBACK, videoP2P::CALL_PRESENCE_DROPPED, key.c_str());
      }
    }
  }
}

#if LOGGING
void GCallClient::PrintRoster() {
  LOG(INFO) <<"Roster contains " << roster_->size() << " callable";
  RosterMap::iterator iter = roster_->begin();
  while (iter != roster_->end()) {
    LOG(INFO) << iter->second.jid.BareJid().Str().c_str() << " " << DescribeStatus(iter->second.show, iter->second.status);
    iter++;
  }
}
#endif

void GCallClient::SendChat(const std::string& to, const std::string msg) {
  buzz::XmlElement* stanza = new buzz::XmlElement(buzz::QN_MESSAGE);
  stanza->AddAttr(buzz::QN_TO, to);
  stanza->AddAttr(buzz::QN_ID, talk_base::CreateRandomString(16));
  stanza->AddAttr(buzz::QN_TYPE, "chat");
  buzz::XmlElement* body = new buzz::XmlElement(buzz::QN_BODY);
  body->SetBodyText(msg);
  stanza->AddElement(body);

  xmpp_client_->SendStanza(stanza);
  delete stanza;
}

void GCallClient::InviteFriend(const std::string& name) {
  buzz::Jid jid(name);
  if (!jid.IsValid() || jid.node() == "") {
    LOG(INFO) <<"Invalid JID. JIDs should be in the form user@domain.";
    return;
  }
  // Note: for some reason the Buzz backend does not forward our presence
  // subscription requests to the end user when that user is another call
  // client as opposed to a Smurf user. Thus, in that scenario, you must
  // run the friend command as the other user too to create the linkage
  // (and you won't be notified to do so).
  friend_invite_send_->Send(jid);
  LOG(INFO) <<"Requesting to befriend " <<  name.c_str();
}

bool GCallClient::CheckCallee(const buzz::Jid callee) {
  LOG(INFO) << "GCallClient::CheckCallee";
  bool found = false;
  buzz::Jid found_jid;

  // otherwise, it's a friend
  for (RosterMap::iterator iter = roster_->begin();
    iter != roster_->end(); ++iter) {
    if (iter->second.jid==callee) {
      found = true;
      found_jid = iter->second.jid;
      break;
    }
  }
  return found;
}

void GCallClient::MakeCallTo(const std::string& name, const cricket::SessionDescription* desc) {
  buzz::Jid callto_jid(name);
  bool found = CheckCallee(callto_jid);

  if (found) {
    LOG(INFO) <<"Found " << name;
    LOG(INFO) << "GCallClient::MakeCallTo-ConnectToPeer";
    const std::string& type = cricket::NS_JINGLE_RTP;
    session_ = session_manager_->CreateSession(name, type);
    session_->set_initiator_name(xmpp_client_->jid().Str().c_str());
    const std::string& toStr = name;
    session_->Initiate(toStr, desc);
  } else {
    LOG(INFO) <<"Could not find online friend " << name.c_str();
    ASSERT(false);
  }
}

void GCallClient::Accept(const cricket::SessionDescription* desc) {
  LOG(INFO) << "GCallClient::Accept()";
  if (!incoming_call_) {
    LOG(INFO) << "There's no incoming call";
    return;
  }

  session_->Accept(desc);
  incoming_call_ = false;
}

void GCallClient::Reject() {
  LOG(INFO) << "GCallClient::Reject";
  session_->Reject(cricket::STR_TERMINATE_DECLINE);
  incoming_call_ = false;
}

void GCallClient::ReceiveReject() {
  LOG(INFO) << "GCallClient::ReceiveReject";
  incoming_call_ = false;
}

void GCallClient::HangUp() {
  LOG(INFO) << "GCallClient::HangUp";
  incoming_call_ = false;
  if(session_) {
    session_->TerminateWithReason(cricket::STR_TERMINATE_SUCCESS);
    return;
  }

  RosterMap::iterator iter = roster_->begin();
  while (iter != roster_->end()) {
    if(iter->second.pendingRemoval) {
      callback_handler_ext(videoP2P::PRESENCE_STATE_CALLBACK, videoP2P::CALL_PRESENCE_DROPPED, iter->second.jid.Str().c_str());
      roster_->erase(iter++);
    }
    else {
      iter++;
    }
  }
}

void GCallClient::SetNick(const std::string& muc_nick) {
  my_status_.set_nick(muc_nick);

  // TODO: We might want to re-send presence, but right
  // now, it appears to be ignored by the MUC.
  //
  // presence_out_->Send(my_status_); for (MucMap::const_iterator itr
  // = mucs_.begin(); itr != mucs_.end(); ++itr) {
  // presence_out_->SendDirected(itr->second->local_jid(),
  // my_status_); }

  LOG(INFO) << "Nick set to" << muc_nick;
}

void GCallClient::OnJingleInfo(const std::string& token,
                    const std::vector<std::string>& relay_hosts,
                    const std::vector<talk_base::SocketAddress>& stun_hosts)
{
   std::vector<talk_base::SocketAddress>::const_iterator iter;
   webrtc::PeerConnectionInterface::IceServers *iceservers = new webrtc::PeerConnectionInterface::IceServers();

   for(iter = stun_hosts.begin(); iter != stun_hosts.end(); iter++) {
      webrtc::PeerConnectionInterface::IceServer server;
      server.uri = "stun:" + iter->HostAsURIString() + ":" + iter->PortAsString();
      iceservers->push_back(server);
   }

   kxmpp_thread_->UpdateIceServers(iceservers);
}

GCallClient::VideoRenderer::VideoRenderer(GCallClient* client,
    webrtc::VideoTrackInterface* track, VideoRenderer::RenderType type) :
    width_(0),
    height_(0),
    type_(type),
    client_(client),
    rendered_track_(track),
    direct_render_config_(NULL),
    rotation_deg_(0) {
  LOG(INFO) << "VideoRenderer::" << __FUNCTION__ << " name=" << (type_?"remote":"local");
  rendered_track_->AddRenderer(this);
}

GCallClient::VideoRenderer::~VideoRenderer() {
  LOG(INFO) << "VideoRenderer::" << __FUNCTION__ << " name=" << (type_?"remote":"local");
  rendered_track_->RemoveRenderer(this);
}

void GCallClient::VideoRenderer::SetSize(int width, int height) {
  //LOG(INFO) << "VideoRenderer::" << __FUNCTION__ << " name=" << (type_?"remote":"local");
  bool isAttached = false;
  jclass clazz;
  jmethodID method;

  if( type_ == 0 )
    return;

  pthread_mutex_lock(&client_->mutex_);

  if ( isAttached == false ) {
    if( gJavaVM->AttachCurrentThread(&gSizeEnv, NULL) < 0 ) {
      goto done;
    }
    isAttached = true;
  }

  width_ = width;
  height_ = height;

  clazz = gSizeEnv->GetObjectClass(gContext);
  if ( clazz == 0) goto done;

  method = gSizeEnv->GetMethodID(clazz, "renderSize", "(II)V");
  if ( method == 0 ) goto done;

  gSizeEnv->CallVoidMethod(gContext, method, width_, height_);

  if( isAttached ) {
    gJavaVM->DetachCurrentThread();
    gSizeEnv = NULL;
  }

done:
  pthread_mutex_unlock(&client_->mutex_);
}

void GCallClient::VideoRenderer::RenderFrame(const cricket::VideoFrame* frame) {
//  LOG(INFO) << "VideoRenderer::" << __FUNCTION__ << " name=" << (type_?"remote":"local");

  jclass clazz;
  jmethodID method_pop;
  jmethodID method_push;

  if( type_ == 0 || frame == NULL || client_->buffer_initialized_ == false)
    return;

  pthread_mutex_lock(&client_->mutex_);

  int size = width_ * height_;
  bool isAttached = false;
  int i = 0;

  VideoRenderer* remote_renderer = client_->remote_renderer_.get();
  if (remote_renderer && client_->y_plane_ != NULL && client_->u_plane_ != NULL && client_->v_plane_ != NULL)
  {
    if( isAttached == false ) {
      if( gJavaVM->AttachCurrentThread(&gRenderEnv, NULL) < 0 ) {
        goto render_done;
      }
      isAttached = true;
    }

    clazz = gRenderEnv->GetObjectClass(gContext);
    if ( clazz == 0)  goto render_done;

    method_pop = gRenderEnv->GetMethodID (clazz, "popFromArrivalQueue", "()I");
    if ( method_pop == 0 ) goto render_done;

    jint index = gRenderEnv->CallIntMethod(gContext, method_pop);

    if( index < 0 || index >= client_->count_ ) goto render_done;

    memcpy(client_->y_plane_[index], frame->GetYPlane(), width_ * height_);
    memcpy(client_->u_plane_[index], frame->GetUPlane(), width_ * height_ / 4);
    memcpy(client_->v_plane_[index], frame->GetVPlane(), width_ * height_ / 4);

    method_push = gRenderEnv->GetMethodID (clazz, "pushIntoDepartureQueue", "(I)V");
    if ( method_push == 0 ) goto render_done;
    gRenderEnv->CallVoidMethod(gContext, method_push, index);

    gRenderEnv->DeleteLocalRef(clazz);

    if( isAttached ) {
      gJavaVM->DetachCurrentThread();
      gRenderEnv = NULL;
    }
  }

render_done:
  pthread_mutex_unlock(&client_->mutex_);
}

void GCallClient::VideoRenderer::SetDirectRenderConfig(webrtc::DirectRenderConfig *config)
{
  pthread_mutex_lock(&client_->mutex_);
  direct_render_config_ = config;
  SetVideoRendererRotation(rotation_deg_);
  pthread_mutex_unlock(&client_->mutex_);
}

void GCallClient::VideoRenderer::SetVideoRendererRotation(int deg)
{
  if(direct_render_config_ != NULL) {
    direct_render_config_->changeOrientation(deg, true);
  }
  rotation_deg_ = deg;
}
