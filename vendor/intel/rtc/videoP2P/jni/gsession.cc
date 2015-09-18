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

#include "gsession.h"
#include "callclient.h"
#include "talk/base/common.h"
#include "talk/base/logging.h"
#include "talk/base/helpers.h"
#include "talk/base/scoped_ptr.h"
#include "talk/base/sslstreamadapter.h"
#include "talk/session/media/mediasession.h"
#include "talk/xmpp/constants.h"
#include "talk/xmpp/jid.h"
#include "talk/p2p/base/p2ptransport.h"

#include "talk/p2p/base/constants.h"
#include "gcontentparser.h"
#include "talk/app/webrtc/jsep.h"

namespace cricket {

// BadMessage is already defined in session.cc
bool BadMessage(const buzz::QName type,
                const std::string& text,
                MessageError* err);

std::string BaseGSession::StateToString(State state) {
  switch (state) {
    case GSession::STATE_INIT:
      return "STATE_INIT";
    case GSession::STATE_SENTINITIATE:
      return "STATE_SENTINITIATE";
    case GSession::STATE_RECEIVEDINITIATE:
      return "STATE_RECEIVEDINITIATE";
    case GSession::STATE_SENTACCEPT:
      return "STATE_SENTACCEPT";
    case GSession::STATE_RECEIVEDACCEPT:
      return "STATE_RECEIVEDACCEPT";
    case GSession::STATE_SENTMODIFY:
      return "STATE_SENTMODIFY";
    case GSession::STATE_RECEIVEDMODIFY:
      return "STATE_RECEIVEDMODIFY";
    case GSession::STATE_SENTREJECT:
      return "STATE_SENTREJECT";
    case GSession::STATE_RECEIVEDREJECT:
      return "STATE_RECEIVEDREJECT";
    case GSession::STATE_SENTREDIRECT:
      return "STATE_SENTREDIRECT";
    case GSession::STATE_SENTTERMINATE:
      return "STATE_SENTTERMINATE";
    case GSession::STATE_RECEIVEDTERMINATE:
      return "STATE_RECEIVEDTERMINATE";
    case GSession::STATE_INPROGRESS:
      return "STATE_INPROGRESS";
    case GSession::STATE_DEINIT:
      return "STATE_DEINIT";
    default:
      break;
  }
  return "STATE_" + talk_base::ToString(state);
}

BaseGSession::BaseGSession(talk_base::Thread* signaling_thread,
                         const std::string& sid,
                         const std::string& content_type,
                         bool initiator)
    : state_(STATE_INIT),
      error_(ERROR_NONE),
      signaling_thread_(signaling_thread),
      sid_(sid),
      content_type_(content_type),
      transport_type_(NS_JINGLE_ICE_UDP),
      initiator_(initiator),
      identity_(NULL),
      local_description_(NULL),
      remote_description_(NULL),
      role_switch_(false) {
  LOG(INFO) << __FUNCTION__;
  ASSERT(signaling_thread->IsCurrent());
}

BaseGSession::~BaseGSession() {
  LOG(INFO) << __FUNCTION__;
  ASSERT(signaling_thread()->IsCurrent());

  ASSERT(state_ != STATE_DEINIT);
  LogState(state_, STATE_DEINIT);
  state_ = STATE_DEINIT;
  SignalState(this, state_);

  delete remote_description_;
  delete local_description_;
}

void BaseGSession::SetState(State state) {
  LOG(INFO) << __FUNCTION__;
  ASSERT(signaling_thread_->IsCurrent());
  if (state != state_) {
    LogState(state_, state);
    state_ = state;
    SignalState(this, state_);
    signaling_thread_->Post(this, MSG_STATE);
  }
}

void BaseGSession::SetError(Error error) {
  LOG(INFO) << __FUNCTION__;
  ASSERT(signaling_thread_->IsCurrent());
  if (error != error_) {
    error_ = error;
    SignalError(this, error);
  }
}

void BaseGSession::LogState(State old_state, State new_state) {
  LOG(LS_INFO) << "GSession:" << id()
               << " Old state:" << StateToString(old_state)
               << " New state:" << StateToString(new_state)
               << " Type:" << content_type()
               << " Transport:" << transport_type();
}

void BaseGSession::OnMessage(talk_base::Message *pmsg) {
  LOG(INFO) << __FUNCTION__;
  switch (pmsg->message_id) {
  case MSG_TIMEOUT:
    // Session timeout has occured.
    SetError(ERROR_TIME);
    break;

  case MSG_STATE:
    switch (state_) {
    case STATE_SENTACCEPT:
    case STATE_RECEIVEDACCEPT:
      SetState(STATE_INPROGRESS);
      break;

    default:
      // Explicitly ignoring some states here.
      break;
    }
    break;
  }
}

GSession::GSession(GSessionManager* session_manager,
                 const std::string& local_name,
                 const std::string& initiator_name,
                 const std::string& sid,
                 const std::string& content_type,
                 GCallClient* client)
    : BaseGSession(session_manager->signaling_thread(),
                  sid, content_type, initiator_name == local_name) {
  LOG(INFO) << __FUNCTION__;
  ASSERT(client != NULL);
  session_manager_ = session_manager;
  local_name_ = local_name;
  initiator_name_ = initiator_name;
  transport_parser_ = new P2PTransportParser();
  client_ = client;
  content_parser_ = new GContentParser();
  initiate_acked_ = false;
  current_protocol_ = PROTOCOL_JINGLE;
}

GSession::~GSession() {
  LOG(INFO) << __FUNCTION__;
  delete transport_parser_;
  delete content_parser_;
}

bool GSession::Initiate(const std::string &to,
                       const SessionDescription* sdesc) {
  LOG(INFO) << __FUNCTION__;
  ASSERT(signaling_thread()->IsCurrent());
  SessionError error;

  // Only from STATE_INIT
  if (state() != STATE_INIT)
    return false;

  // Setup for signaling.
  set_remote_name(to);
  set_local_description(sdesc);

  if (!SendInitiateMessage(sdesc, &error)) {
    LOG(LS_ERROR) << "Could not send initiate message: " << error.text;
    return false;
  }

  SetState(GSession::STATE_SENTINITIATE);
  return true;
}

bool GSession::SendCandidate(const webrtc::IceCandidateInterface* candidate) {
  if (!candidate) return false;

  std::string content_name(NS_JINGLE_ICE_UDP);
  Candidates candidates;
  candidates.push_back(candidate->candidate());

  TransportDescription tdesc;
  tdesc.candidates = candidates;
  tdesc.transport_type = content_name;

  TransportInfo tinfo(candidate->sdp_mid(), tdesc);

  SessionError error;
  if (!SendTransportInfoMessage(tinfo, &error)) {
    LOG(LS_ERROR) << "Could not send transport info message: "
                  << error.text;
    return false;
  }

  return true;
}

bool GSession::Accept(const SessionDescription* sdesc) {
  LOG(INFO) << __FUNCTION__;
  ASSERT(signaling_thread()->IsCurrent());

  // Only if just received initiate
  if (state() != STATE_RECEIVEDINITIATE) {
    return false;
    LOG(LS_ERROR) << "GSession::Accept is not in STATE_RECEIVEDINITIATE";
  }

#if 0
  // Setup for signaling.
  set_local_description(sdesc);
#endif

  SessionError error;
  if (!SendAcceptMessage(sdesc, &error)) {
    LOG(LS_ERROR) << "Could not send accept message: " << error.text;
    return false;
  }

  SetState(GSession::STATE_SENTACCEPT);
  return true;
}

bool GSession::Reject(const std::string& reason) {
  LOG(INFO) << __FUNCTION__;
  ASSERT(signaling_thread()->IsCurrent());

  // Reject is sent in response to an initiate or modify, to reject the
  // request
  if (state() != STATE_RECEIVEDINITIATE && state() != STATE_RECEIVEDMODIFY)
    return false;

  SessionError error;
  if (!SendRejectMessage(reason, &error)) {
    LOG(LS_ERROR) << "Could not send reject message: " << error.text;
    return false;
  }

  SetState(STATE_SENTREJECT);
  return true;
}

bool GSession::TerminateWithReason(const std::string& reason) {
  LOG(INFO) << __FUNCTION__;
  ASSERT(signaling_thread()->IsCurrent());

  // Either side can terminate, at any time.
  switch (state()) {
    case STATE_SENTTERMINATE:
    case STATE_RECEIVEDTERMINATE:
      return false;

    case STATE_SENTREJECT:
    case STATE_RECEIVEDREJECT:
      // We don't need to send terminate if we sent or received a reject...
      // it's implicit.
      break;

    default:
      SessionError error;
      if (!SendTerminateMessage(reason, &error)) {
        LOG(LS_ERROR) << "Could not send terminate message: " << error.text;
        return false;
      }
      break;
  }

  SetState(STATE_SENTTERMINATE);
  return true;
}

bool GSession::SendInfoMessage(const XmlElements& elems) {
  LOG(INFO) << __FUNCTION__;
  ASSERT(signaling_thread()->IsCurrent());
  SessionError error;
  if (!SendMessage(ACTION_SESSION_INFO, elems, &error)) {
    LOG(LS_ERROR) << "Could not send info message " << error.text;
    return false;
  }
  return true;
}

bool GSession::SendDescriptionInfoMessage(const ContentInfos& contents) {
  LOG(INFO) << __FUNCTION__;
  XmlElements elems;
  WriteError write_error;
  if (!WriteDescriptionInfo(current_protocol_,
                            contents,
                            GetContentParsers(),
                            &elems, &write_error)) {
    LOG(LS_ERROR) << "Could not write description info message: "
                  << write_error.text;
    return false;
  }
  SessionError error;
  if (!SendMessage(ACTION_DESCRIPTION_INFO, elems, &error)) {
    LOG(LS_ERROR) << "Could not send description info message: "
                  << error.text;
    return false;
  }
  return true;
}

TransportInfos GSession::GetEmptyTransportInfos(
    const ContentInfos& contents) const {
  TransportInfos tinfos;
  for (ContentInfos::const_iterator content = contents.begin();
       content != contents.end(); ++content) {
    TransportDescription tdesc;
    tdesc.candidates = Candidates();
    tdesc.transport_type = transport_type();

    tinfos.push_back(
        TransportInfo(content->name, tdesc));
  }
  return tinfos;
}

bool GSession::OnRemoteCandidates(
    const TransportInfos& tinfos, ParseError* error) {
  for (TransportInfos::const_iterator tinfo = tinfos.begin();
       tinfo != tinfos.end(); ++tinfo) {
    LOG(INFO) << "number of Candidate=" << tinfo->description.candidates.size();
    if (tinfo->description.candidates.size() != 0) {
      SignalOnRemoteCandidate(tinfo->content_name, tinfo->description.candidates.front());
    }
  }
  return true;
}

TransportParserMap GSession::GetTransportParsers() {
  TransportParserMap parsers;
  parsers[transport_type()] = transport_parser_;
  return parsers;
}

CandidateTranslatorMap GSession::GetCandidateTranslators() {
  CandidateTranslatorMap translators;
  // NOTE: This technique makes it impossible to parse G-ICE
  // candidates in session-initiate messages because the channels
  // aren't yet created at that point.  Since we don't use candidates
  // in session-initiate messages, we should be OK.  Once we switch to
  // ICE, this translation shouldn't be necessary.
#if 0
  for (TransportMap::const_iterator iter = transport_proxies().begin();
       iter != transport_proxies().end(); ++iter) {
    translators[iter->first] = iter->second;
  }
#endif
  return translators; // NOT using the translator, but still require to return an empty one
}

ContentParserMap GSession::GetContentParsers() {
  ContentParserMap parsers;
  parsers[content_type()] = content_parser_;
  return parsers;
}


void GSession::OnIncomingMessage(const SessionMessage& msg) {
  ASSERT(signaling_thread()->IsCurrent());
  ASSERT(state() == STATE_INIT || msg.from == remote_name());

  if (current_protocol_== PROTOCOL_HYBRID) {
    if (msg.protocol == PROTOCOL_GINGLE) {
      current_protocol_ = PROTOCOL_GINGLE;
    } else {
      current_protocol_ = PROTOCOL_JINGLE;
    }
  }

  bool valid = false;
  MessageError error;
  switch (msg.type) {
    case ACTION_SESSION_INITIATE:
      valid = OnInitiateMessage(msg, &error);
      break;
    case ACTION_SESSION_INFO:
      valid = OnInfoMessage(msg);
      break;
    case ACTION_SESSION_ACCEPT:
      valid = OnAcceptMessage(msg, &error);
      break;
    case ACTION_SESSION_REJECT:
      valid = OnRejectMessage(msg, &error);
      break;
    case ACTION_SESSION_TERMINATE:
      valid = OnTerminateMessage(msg, &error);
      break;
    case ACTION_TRANSPORT_INFO:
      valid = OnTransportInfoMessage(msg, &error);
      break;
    case ACTION_TRANSPORT_ACCEPT:
      valid = OnTransportAcceptMessage(msg, &error);
      break;
    case ACTION_DESCRIPTION_INFO:
      valid = OnDescriptionInfoMessage(msg, &error);
      break;
    default:
      valid = BadMessage(buzz::QN_STANZA_BAD_REQUEST,
                         "unknown session message type",
                         &error);
  }

  if (valid) {
    SendAcknowledgementMessage(msg.stanza);
  } else {
    SignalErrorMessage(this, msg.stanza, error.type,
                       "modify", error.text, NULL);
  }
}

void GSession::OnIncomingResponse(const buzz::XmlElement* orig_stanza,
                                 const buzz::XmlElement* response_stanza,
                                 const SessionMessage& msg) {
  ASSERT(signaling_thread()->IsCurrent());

  if (msg.type == ACTION_SESSION_INITIATE) {
    OnInitiateAcked();
  }
}

void GSession::OnInitiateAcked() {
    // TODO: This is to work around server re-ordering
    // messages.  We send the candidates once the session-initiate
    // is acked.  Once we have fixed the server to guarantee message
    // order, we can remove this case.
  if (!initiate_acked_) {
    initiate_acked_ = true;
  }
}

void GSession::OnFailedSend(const buzz::XmlElement* orig_stanza,
                           const buzz::XmlElement* error_stanza) {
  ASSERT(signaling_thread()->IsCurrent());

  SessionMessage msg;
  ParseError parse_error;
  if (!ParseSessionMessage(orig_stanza, &msg, &parse_error)) {
    LOG(LS_ERROR) << "Error parsing failed send: " << parse_error.text
                  << ":" << orig_stanza;
    return;
  }

  // If the error is a session redirect, call OnRedirectError, which will
  // continue the session with a new remote JID.
  SessionRedirect redirect;
  if (FindSessionRedirect(error_stanza, &redirect)) {
    SessionError error;
    if (!OnRedirectError(redirect, &error)) {
      // TODO: Should we send a message back?  The standard
      // says nothing about it.
      LOG(LS_ERROR) << "Failed to redirect: " << error.text;
      SetError(ERROR_RESPONSE);
    }
    return;
  }

  std::string error_type = "cancel";

  const buzz::XmlElement* error = error_stanza->FirstNamed(buzz::QN_ERROR);
  if (error) {
    error_type = error->Attr(buzz::QN_TYPE);

    LOG(LS_ERROR) << "GSession error:\n" << error->Str() << "\n"
                  << "in response to:\n" << orig_stanza->Str();
  } else {
    // don't crash if <error> is missing
    LOG(LS_ERROR) << "GSession error without <error/> element, ignoring";
    return;
  }

  if (msg.type == ACTION_TRANSPORT_INFO) {
    // Transport messages frequently generate errors because they are sent right
    // when we detect a network failure.  For that reason, we ignore such
    // errors, because if we do not establish writability again, we will
    // terminate anyway.  The exceptions are transport-specific error tags,
    // which we pass on to the respective transport.
  } else if ((error_type != "continue") && (error_type != "wait")) {
    // We do not set an error if the other side said it is okay to continue
    // (possibly after waiting).  These errors can be ignored.
    SetError(ERROR_RESPONSE);
  }
}

bool GSession::OnInitiateMessage(const SessionMessage& msg,
                                MessageError* error) {
  if (!CheckState(STATE_INIT, error))
    return false;

  SessionInitiate init;
  if (!ParseSessionInitiate(msg.protocol, msg.action_elem,
                            GetContentParsers(), GetTransportParsers(),
                            GetCandidateTranslators(),
                            &init, error))
    return false;

  bool video = false;
  bool audio = false;
  for (ContentInfos::const_iterator content = init.contents.begin();
       content != init.contents.end(); ++content) {
    if(cricket::IsVideoContent(content)) video = true;
    else if(cricket::IsAudioContent(content)) audio = true;
  }
  client_->SetIncomingCallCapability(video, audio);
  set_remote_name(msg.from);
  set_initiator_name(msg.initiator);
  set_remote_description(new SessionDescription(init.ClearContents(),
                                                init.transports,
                                                init.groups));

  SetState(STATE_RECEIVEDINITIATE);

  return true;
}

bool GSession::OnAcceptMessage(const SessionMessage& msg, MessageError* error) {
  if (!CheckState(STATE_SENTINITIATE, error))
    return false;

  SessionAccept accept;
  if (!ParseSessionAccept(msg.protocol, msg.action_elem,
                          GetContentParsers(), GetTransportParsers(),
                          GetCandidateTranslators(),
                          &accept, error)) {
    return false;
  }

  // If we get an accept, we can assume the initiate has been
  // received, even if we haven't gotten an IQ response.
  OnInitiateAcked();

  set_remote_description(new SessionDescription(accept.ClearContents(),
                                                accept.transports,
                                                accept.groups));
  // Updating transport with TransportDescription.
  //kvnPushdownTransportDescription(CS_REMOTE, CA_ANSWER);
  //kvnMaybeEnableMuxingSupport();  // Enable transport channel mux if supported.
  SetState(STATE_RECEIVEDACCEPT);

  if (!OnRemoteCandidates(accept.transports, error))
    return false;

  return true;
}

bool GSession::OnRejectMessage(const SessionMessage& msg, MessageError* error) {
  if (!CheckState(STATE_SENTINITIATE, error))
    return false;

  SetState(STATE_RECEIVEDREJECT);
  return true;
}

bool GSession::OnInfoMessage(const SessionMessage& msg) {
  SignalInfoMessage(this, msg.action_elem);
  return true;
}

bool GSession::OnTerminateMessage(const SessionMessage& msg,
                                 MessageError* error) {
  SessionTerminate term;
  if (!ParseSessionTerminate(msg.protocol, msg.action_elem, &term, error))
    return false;

  SignalReceivedTerminateReason(this, term.reason);
  if (term.debug_reason != buzz::STR_EMPTY) {
    LOG(LS_VERBOSE) << "Received error on call: " << term.debug_reason;
  }

  SetState(STATE_RECEIVEDTERMINATE);
  return true;
}

bool GSession::OnTransportInfoMessage(const SessionMessage& msg,
                                     MessageError* error) {
  TransportInfos tinfos;
  if (!ParseTransportInfos(msg.protocol, msg.action_elem,
                           initiator_description()->contents(),
                           GetTransportParsers(), GetCandidateTranslators(),
                           &tinfos, error)) {
    return false;
  }

  if (!OnRemoteCandidates(tinfos, error))
    return false;

  return true;
}

bool GSession::OnTransportAcceptMessage(const SessionMessage& msg,
                                       MessageError* error) {
  // TODO: Currently here only for compatibility with
  // Gingle 1.1 clients (notably, Google Voice).
  return true;
}

bool GSession::OnDescriptionInfoMessage(const SessionMessage& msg,
                              MessageError* error) {
  if (!CheckState(STATE_INPROGRESS, error))
    return false;

  DescriptionInfo description_info;
  if (!ParseDescriptionInfo(msg.protocol, msg.action_elem,
                            GetContentParsers(), GetTransportParsers(),
                            GetCandidateTranslators(),
                            &description_info, error)) {
    return false;
  }

  ContentInfos updated_contents = description_info.ClearContents();

  // TODO: Currently, reflector sends back
  // video stream updates even for an audio-only call, which causes
  // this to fail.  Put this back once reflector is fixed.
  //
  // ContentInfos::iterator it;
  // First, ensure all updates are valid before modifying remote_description_.
  // for (it = updated_contents.begin(); it != updated_contents.end(); ++it) {
  //   if (remote_description()->GetContentByName(it->name) == NULL) {
  //     return false;
  //   }
  // }

  // TODO: We used to replace contents from an update, but
  // that no longer works with partial updates.  We need to figure out
  // a way to merge patial updates into contents.  For now, users of
  // Session should listen to SignalRemoteDescriptionUpdate and handle
  // updates.  They should not expect remote_description to be the
  // latest value.
  //
  // for (it = updated_contents.begin(); it != updated_contents.end(); ++it) {
  //     remote_description()->RemoveContentByName(it->name);
  //     remote_description()->AddContent(it->name, it->type, it->description);
  //   }
  // }

  SignalRemoteDescriptionUpdate(this, updated_contents);
  return true;
}

// this function is defined in session.cc
bool BareJidsEqual(const std::string& name1,
                   const std::string& name2);

bool GSession::OnRedirectError(const SessionRedirect& redirect,
                              SessionError* error) {
  MessageError message_error;
  if (!CheckState(STATE_SENTINITIATE, &message_error)) {
    return BadWrite(message_error.text, error);
  }

  if (!BareJidsEqual(remote_name(), redirect.target))
    return BadWrite("Redirection not allowed: must be the same bare jid.",
                    error);

  // When we receive a redirect, we point the session at the new JID
  // and resend the candidates.
  set_remote_name(redirect.target);
  return (SendInitiateMessage(local_description(), error));
}

bool GSession::CheckState(State expected, MessageError* error) {
  if (state() != expected) {
    return BadMessage(buzz::QN_STANZA_NOT_ALLOWED,
                      "message not allowed in current state",
                      error);
  }
  return true;
}

void GSession::SetError(Error error) {
  BaseGSession::SetError(error);
  if (error != ERROR_NONE)
    signaling_thread()->Post(this, MSG_ERROR);
}

void GSession::OnMessage(talk_base::Message* pmsg) {
  // preserve this because BaseGSession::OnMessage may modify it
  State orig_state = state();

  BaseGSession::OnMessage(pmsg);

  switch (pmsg->message_id) {
  case MSG_ERROR:
    // On session error, pretend we receive terminate to perform complete hangup
    SetState(STATE_RECEIVEDTERMINATE);
    break;

  case MSG_STATE:
    switch (orig_state) {
    case STATE_SENTREJECT:
    case STATE_RECEIVEDREJECT:
      // Assume clean termination.
      Terminate();
      break;

    case STATE_SENTTERMINATE:
    case STATE_RECEIVEDTERMINATE:
      session_manager_->DestroySession(this);
      break;

    default:
      // Explicitly ignoring some states here.
      break;
    }
    break;
  }
}

bool GSession::SendInitiateMessage(const SessionDescription* sdesc,
                                  SessionError* error) {
  SessionInitiate init;
  init.contents = sdesc->contents();
  init.transports = GetEmptyTransportInfos(init.contents);
  init.groups = sdesc->groups();
  return SendMessage(ACTION_SESSION_INITIATE, init, error);
}

bool GSession::WriteSessionAction(
    SignalingProtocol protocol, const SessionInitiate& init,
    XmlElements* elems, WriteError* error) {
  return WriteSessionInitiate(protocol, init.contents, init.transports,
                              GetContentParsers(), GetTransportParsers(),
                              GetCandidateTranslators(), init.groups,
                              elems, error);
}

bool GSession::SendAcceptMessage(const SessionDescription* sdesc,
                                SessionError* error) {
  XmlElements elems;
  if (!WriteSessionAccept(current_protocol_,
                          sdesc->contents(),
                          GetEmptyTransportInfos(sdesc->contents()),
                          GetContentParsers(), GetTransportParsers(),
                          GetCandidateTranslators(), sdesc->groups(),
                          &elems, error)) {
    return false;
  }
  return SendMessage(ACTION_SESSION_ACCEPT, elems, error);
}

bool GSession::SendRejectMessage(const std::string& reason,
                                SessionError* error) {
  SessionTerminate term(reason);
  return SendMessage(ACTION_SESSION_REJECT, term, error);
}

bool GSession::SendTerminateMessage(const std::string& reason,
                                   SessionError* error) {
  SessionTerminate term(reason);
  return SendMessage(ACTION_SESSION_TERMINATE, term, error);
}

bool GSession::WriteSessionAction(SignalingProtocol protocol,
                                 const SessionTerminate& term,
                                 XmlElements* elems, WriteError* error) {
  WriteSessionTerminate(protocol, term, elems);
  return true;
}

bool GSession::SendTransportInfoMessage(const TransportInfo& tinfo,
                                       SessionError* error) {
  return SendMessage(ACTION_TRANSPORT_INFO, tinfo, error);
}

bool GSession::WriteSessionAction(SignalingProtocol protocol,
                                 const TransportInfo& tinfo,
                                 XmlElements* elems, WriteError* error) {
  TransportInfos tinfos;
  tinfos.push_back(tinfo);
  return WriteTransportInfos(protocol, tinfos,
                             GetTransportParsers(), GetCandidateTranslators(),
                             elems, error);
}

bool GSession::SendMessage(ActionType type, const XmlElements& action_elems,
                          SessionError* error) {
  talk_base::scoped_ptr<buzz::XmlElement> stanza(
      new buzz::XmlElement(buzz::QN_IQ));

  SessionMessage msg(current_protocol_, type, id(), initiator_name());
  msg.to = remote_name();
  WriteSessionMessage(msg, action_elems, stanza.get());

  SignalOutgoingMessage(this, stanza.get());
  return true;
}

template <typename Action>
bool GSession::SendMessage(ActionType type, const Action& action,
                          SessionError* error) {
  talk_base::scoped_ptr<buzz::XmlElement> stanza(
      new buzz::XmlElement(buzz::QN_IQ));
  if (!WriteActionMessage(type, action, stanza.get(), error))
    return false;

  SignalOutgoingMessage(this, stanza.get());
  return true;
}

template <typename Action>
bool GSession::WriteActionMessage(ActionType type, const Action& action,
                                 buzz::XmlElement* stanza,
                                 WriteError* error) {
  if (current_protocol_ == PROTOCOL_HYBRID) {
    if (!WriteActionMessage(PROTOCOL_JINGLE, type, action, stanza, error))
      return false;
    if (!WriteActionMessage(PROTOCOL_GINGLE, type, action, stanza, error))
      return false;
  } else {
    if (!WriteActionMessage(current_protocol_, type, action, stanza, error))
      return false;
  }
  return true;
}

template <typename Action>
bool GSession::WriteActionMessage(SignalingProtocol protocol,
                                 ActionType type, const Action& action,
                                 buzz::XmlElement* stanza, WriteError* error) {
  XmlElements action_elems;
  if (!WriteSessionAction(protocol, action, &action_elems, error))
    return false;

  SessionMessage msg(protocol, type, id(), initiator_name());
  msg.to = remote_name();

  WriteSessionMessage(msg, action_elems, stanza);
  return true;
}

void GSession::SendAcknowledgementMessage(const buzz::XmlElement* stanza) {
  talk_base::scoped_ptr<buzz::XmlElement> ack(
      new buzz::XmlElement(buzz::QN_IQ));
  ack->SetAttr(buzz::QN_TO, remote_name());
  ack->SetAttr(buzz::QN_ID, stanza->Attr(buzz::QN_ID));
  ack->SetAttr(buzz::QN_TYPE, "result");

  SignalOutgoingMessage(this, ack.get());
}

}  // namespace cricket
