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

#ifndef TALK_EXAMPLES_CALL_CALLCLIENT_H_
#define TALK_EXAMPLES_CALL_CALLCLIENT_H_

#include <map>
#include <string>
#include <vector>
#include <jni.h>

#include "gsession.h"
#include "talk/media/base/mediachannel.h"
#include "talk/session/media/mediamessages.h"
#include "talk/xmpp/xmppclient.h"
#include "talk/xmpp/presencestatus.h"
#include "talk/examples/call/console.h"

#include "third_party/webrtc/common_types.h"
#include "talk/app/webrtc/mediastreaminterface.h"

class KXmppThread;
class Conductor;

namespace buzz {
class PresencePushTask;
class PresenceOutTask;
class FriendInviteSendTask;
class DiscoInfoQueryTask;
class Muc;
class PresenceStatus;
class IqTask;
class XmlElement;
struct AvailableMediaEntry;
struct MucRoomInfo;
}

namespace talk_base {
class Thread;
class NetworkManager;
}

namespace cricket {
class PortAllocator;
class MediaEngineInterface;
class GSessionManagerTask;
struct MediaStreams;
struct StreamParams;
}

struct RosterItem {
  buzz::Jid jid;
  buzz::PresenceStatus::Show show;
  std::string status;
  bool pendingRemoval;
};

struct StaticRenderedView {
  StaticRenderedView(const cricket::StaticVideoView& view,
                     cricket::VideoRenderer* renderer) :
      view(view),
      renderer(renderer) {
  }

  cricket::StaticVideoView view;
  cricket::VideoRenderer* renderer;
};

typedef std::vector<StaticRenderedView> StaticRenderedViews;

void* GetRemoteSurfaceView();
void* GetRemoteSurface();

class GCallClient: public sigslot::has_slots<> {
 public:
  GCallClient(buzz::XmppClient* xmpp_client,
             KXmppThread* kxmpp_thread,
             const std::string& caps_node,
             const std::string& version);
  ~GCallClient();

  void StartLocalRenderer(webrtc::VideoTrackInterface* local_video);
  void StartRemoteRenderer(webrtc::VideoTrackInterface* remote_video);
  void StopLocalRenderer() {
    local_renderer_.reset();
  }
  void StopRemoteRenderer() {
    remote_renderer_.reset();
  }

  void SetAutoAccept(bool auto_accept) {
    auto_accept_ = auto_accept;
  }

  void SetRender(bool render) {
    render_ = render;
  }
  void SetDataChannelEnabled(bool data_channel_enabled) {
    data_channel_enabled_ = data_channel_enabled;
  }
  void SetConsole(Console *console) {
    console_ = console;
  }
  void SetPriority(int priority) {
    my_status_.set_priority(priority);
  }
  void SendStatus() {
    SendStatus(my_status_);
  }
  void SendStatus(const buzz::PresenceStatus& status);

  void ParseLine(const std::string &str);

  void SendChat(const std::string& to, const std::string msg);
  void SendData(const std::string& stream_name,
                const std::string& text);
  void InviteFriend(const std::string& user);
  void SetNick(const std::string& muc_nick);
  void SetPortAllocatorFlags(uint32 flags) { portallocator_flags_ = flags; }
  bool CheckCallee(const buzz::Jid callee);
  void MakeCallTo(const std::string& name, const cricket::SessionDescription* desc);
  void SendCandidate(const webrtc::IceCandidateInterface* candidate);
  void OnRemoteCandidate(const std::string& content_name, const cricket::Candidate& candidate);
  void HangUp();
  void Accept(const cricket::SessionDescription* desc);
  void Reject();
  void ReceiveReject();

  std::string GetCaller();
  void SetAllowLocalIps(bool allow_local_ips) {
    allow_local_ips_ = allow_local_ips;
  }

  void SetInitialProtocol(cricket::SignalingProtocol initial_protocol) {
    initial_protocol_ = initial_protocol;
  }

  typedef std::map<buzz::Jid, buzz::Muc*> MucMap;

  void OnSessionCreate(cricket::GSession* session, bool initiate);
  void OnSessionDestroy(cricket::GSession* session) { session_ = NULL; }
  cricket::GSession* GetSession() { return session_; }
  bool IsIncomingCall() { return incoming_call_; }
  bool IsIncomingCallSupportVideo() { return remote_cap_video_; }
  bool IsIncomingCallSupportAudio() { return remote_cap_audio_; }
  void SetIncomingCallCapability(bool video, bool audio) {
    remote_cap_video_ = video;
    remote_cap_audio_ = audio;
  }

  void SetVideoRendererRotation(int deg);

  jbyte **y_plane_;
  jbyte **u_plane_;
  jbyte **v_plane_;
  int index_;
  int count_;
  bool buffer_initialized_;
  pthread_mutex_t mutex_;

 private:
  void AddStream(uint32 audio_src_id, uint32 video_src_id);
  void RemoveStream(uint32 audio_src_id, uint32 video_src_id);
  void OnStateChange(buzz::XmppEngine::State state);

  void InitMedia();
  void DestroyMedia();
  void InitPresence();
  void StartXmppPing();
  void OnPingTimeout();
  void InitIce();
  void OnRequestSignaling();
  void OnSessionState(cricket::BaseGSession* session,
                      cricket::BaseGSession::State state);
  void OnStatusUpdate(const buzz::PresenceStatus& status);
  void OnPresenterStateChange(const std::string& nick,
                              bool was_presenting, bool is_presenting);
  void OnAudioMuteStateChange(const std::string& nick,
                              bool was_muted, bool is_muted);
  void OnRecordingStateChange(const std::string& nick,
                              bool was_recording, bool is_recording);
  void OnRemoteMuted(const std::string& mutee_nick,
                     const std::string& muter_nick,
                     bool should_mute_locally);
  void OnMediaBlocked(const std::string& blockee_nick,
                      const std::string& blocker_nick);
  void OnDevicesChange();
  void OnJingleInfo(const std::string& token,
                    const std::vector<std::string>& relay_hosts,
                    const std::vector<talk_base::SocketAddress>& stun_hosts);
  buzz::Jid GenerateRandomMucJid();

  void AddStaticRenderedView(
      cricket::GSession* session,
      uint32 ssrc, int width, int height, int framerate,
      int x_offset, int y_offset);
  bool RemoveStaticRenderedView(uint32 ssrc);
  void RemoveAllStaticRenderedViews();
  void SendViewRequest(cricket::GSession* session);
  bool SelectFirstDesktopScreencastId(cricket::ScreencastId* screencastid);


  static const std::string strerror(buzz::XmppEngine::Error err);
  static const short strerrorcode(buzz::XmppEngine::Error err);

  void PrintRoster();
  void PlaceCall(const buzz::Jid& jid/*, const cricket::CallOptions& options*/);


  void GetDevices();
  void PrintDevices(const std::vector<std::string>& names);

  void SetVolume(const std::string& level);

  typedef std::map<std::string, RosterItem> RosterMap;

  Console *console_;
  buzz::XmppClient* xmpp_client_;
  KXmppThread* kxmpp_thread_;
  talk_base::Thread* worker_thread_;
  cricket::GSessionManager* session_manager_;
  cricket::GSessionManagerTask* session_manager_task_;
  cricket::GSession *session_;

  MucMap mucs_;

  bool incoming_call_;
  bool remote_cap_video_;
  bool remote_cap_audio_;
  bool auto_accept_;
  bool render_;
  bool data_channel_enabled_;
  buzz::PresenceStatus my_status_;
  buzz::PresencePushTask* presence_push_;
  buzz::PresenceOutTask* presence_out_;
  buzz::FriendInviteSendTask* friend_invite_send_;
  RosterMap* roster_;
  uint32 portallocator_flags_;

  bool allow_local_ips_;
  cricket::SignalingProtocol initial_protocol_;
  std::string last_sent_to_;
  talk_base::scoped_refptr<Conductor> conductor_;

protected:
  class VideoRenderer : public webrtc::VideoRendererInterface {
  public:
    enum RenderType {
        LOCAL=0,
        REMOTE
    };
    VideoRenderer(GCallClient* client, webrtc::VideoTrackInterface* track, VideoRenderer::RenderType type);
    virtual ~VideoRenderer();

    virtual void SetSize(int width, int height);
    virtual void RenderFrame(const cricket::VideoFrame* frame);
    virtual void SetDirectRenderConfig(webrtc::DirectRenderConfig *config);

    void SetVideoRendererRotation(int deg);

    int width() const {
      return width_;
    }

    int height() const {
      return height_;
    }

  private:
    int width_, height_;
    RenderType type_;
    GCallClient *client_;
    talk_base::scoped_refptr<webrtc::VideoTrackInterface> rendered_track_;
    webrtc::DirectRenderConfig *direct_render_config_;
    int rotation_deg_;
  };

  talk_base::scoped_ptr<VideoRenderer> local_renderer_;
  talk_base::scoped_ptr<VideoRenderer> remote_renderer_;
};
#endif  // TALK_EXAMPLES_CALL_CALLCLIENT_H_
