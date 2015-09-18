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

#include "gsessionmanager.h"

#include "talk/base/common.h"
#include "talk/base/helpers.h"
#include "talk/base/logging.h"
#include "talk/base/scoped_ptr.h"
#include "talk/base/stringencode.h"
#include "talk/p2p/base/constants.h"
#include "gsession.h"
#include "talk/p2p/base/sessionmessages.h"
#include "talk/xmpp/constants.h"
#include "talk/xmpp/jid.h"
#include "callclient.h"

namespace cricket {

GSessionManager::GSessionManager() {
  signaling_thread_ = talk_base::Thread::Current();
  timeout_ = 50;
}
GSessionManager::~GSessionManager() {
  // Note: GSession::Terminate occurs asynchronously, so it's too late to
  // delete them now.  They better be all gone.
  ASSERT(session_map_.empty());
  // TerminateAll();
  SignalDestroyed();
}

void GSessionManager::AddClient(const std::string& content_type,
                               GCallClient* client) {
  ASSERT(client_map_.find(content_type) == client_map_.end());
  client_map_[content_type] = client;
}

void GSessionManager::RemoveClient(const std::string& content_type) {
  ClientMap::iterator iter = client_map_.find(content_type);
  ASSERT(iter != client_map_.end());
  client_map_.erase(iter);
}

GCallClient* GSessionManager::GetClient(const std::string& content_type) {
  ClientMap::iterator iter = client_map_.find(content_type);
  return (iter != client_map_.end()) ? iter->second : NULL;
}

GSession* GSessionManager::CreateSession(const std::string& local_name,
                                       const std::string& content_type) {
  return CreateSession(local_name, local_name,
                       talk_base::ToString(talk_base::CreateRandomId()),
                       content_type, false);
}

GSession* GSessionManager::CreateSession(
    const std::string& local_name, const std::string& initiator_name,
    const std::string& sid, const std::string& content_type,
    bool received_initiate) {
  GCallClient* client = GetClient(content_type);
  ASSERT(client != NULL);

  GSession* session = new GSession(this, local_name, initiator_name,
                                 sid, content_type, client);
  session_map_[session->id()] = session;
  session->SignalRequestSignaling.connect(
      this, &GSessionManager::OnRequestSignaling);
  session->SignalOutgoingMessage.connect(
      this, &GSessionManager::OnOutgoingMessage);
  session->SignalErrorMessage.connect(this, &GSessionManager::OnErrorMessage);
  SignalSessionCreate(session, received_initiate);
  return session;
}

void GSessionManager::DestroySession(GSession* session) {
  if (session != NULL) {
    SessionMap::iterator it = session_map_.find(session->id());
    if (it != session_map_.end()) {
      SignalSessionDestroy(session);
      session->client()->OnSessionDestroy(session);
      session_map_.erase(it);
      delete session;
    }
  }
}

GSession* GSessionManager::GetSession(const std::string& sid) {
  SessionMap::iterator it = session_map_.find(sid);
  if (it != session_map_.end())
    return it->second;
  return NULL;
}

void GSessionManager::TerminateAll() {
  while (session_map_.begin() != session_map_.end()) {
    GSession* session = session_map_.begin()->second;
    session->Terminate();
  }
}

bool GSessionManager::IsSessionMessage(const buzz::XmlElement* stanza) {
  return cricket::IsSessionMessage(stanza);
}

GSession* GSessionManager::FindSession(const std::string& sid,
                                     const std::string& remote_name) {
  SessionMap::iterator iter = session_map_.find(sid);
  if (iter == session_map_.end())
    return NULL;

  GSession* session = iter->second;
  if (buzz::Jid(remote_name) != buzz::Jid(session->remote_name()))
    return NULL;

  return session;
}

void GSessionManager::OnIncomingMessage(const buzz::XmlElement* stanza) {
  SessionMessage msg;
  ParseError error;

  if (!ParseSessionMessage(stanza, &msg, &error)) {
    SendErrorMessage(stanza, buzz::QN_STANZA_BAD_REQUEST, "modify",
                     error.text, NULL);
    return;
  }

  GSession* session = FindSession(msg.sid, msg.from);
  if (session) {
    session->OnIncomingMessage(msg);
    return;
  }

  if (!session_map_.empty()) {
    LOG(INFO) << "GSessionManager::OnIncomingMessage already has call in progress";
    return;
  }

  if (msg.type != ACTION_SESSION_INITIATE) {
    SendErrorMessage(stanza, buzz::QN_STANZA_BAD_REQUEST, "modify",
                     "unknown session", NULL);
    return;
  }

  std::string content_type;
  if (!ParseContentType(msg.protocol, msg.action_elem,
                        &content_type, &error)) {
    SendErrorMessage(stanza, buzz::QN_STANZA_BAD_REQUEST, "modify",
                     error.text, NULL);
    return;
  }

  if (!GetClient(content_type)) {
    SendErrorMessage(stanza, buzz::QN_STANZA_BAD_REQUEST, "modify",
                     "unknown content type: " + content_type, NULL);
    return;
  }

  session = CreateSession(msg.to, msg.initiator, msg.sid,
                          content_type, true);
  session->OnIncomingMessage(msg);
}

void GSessionManager::OnIncomingResponse(const buzz::XmlElement* orig_stanza,
    const buzz::XmlElement* response_stanza) {
  if (orig_stanza == NULL || response_stanza == NULL) {
    return;
  }

  SessionMessage msg;
  ParseError error;
  if (!ParseSessionMessage(orig_stanza, &msg, &error)) {
    LOG(LS_WARNING) << "Error parsing incoming response: " << error.text
                    << ":" << orig_stanza;
    return;
  }

  GSession* session = FindSession(msg.sid, msg.to);
  if (session) {
    session->OnIncomingResponse(orig_stanza, response_stanza, msg);
  }
}

void GSessionManager::OnFailedSend(const buzz::XmlElement* orig_stanza,
                                  const buzz::XmlElement* error_stanza) {
  SessionMessage msg;
  ParseError error;
  if (!ParseSessionMessage(orig_stanza, &msg, &error)) {
    return;  // TODO: log somewhere?
  }

  GSession* session = FindSession(msg.sid, msg.to);
  if (session) {
    talk_base::scoped_ptr<buzz::XmlElement> synthetic_error;
    if (!error_stanza) {
      // A failed send is semantically equivalent to an error response, so we
      // can just turn the former into the latter.
      synthetic_error.reset(
        CreateErrorMessage(orig_stanza, buzz::QN_STANZA_ITEM_NOT_FOUND,
                           "cancel", "Recipient did not respond", NULL));
      error_stanza = synthetic_error.get();
    }

    session->OnFailedSend(orig_stanza, error_stanza);
  }
}

void GSessionManager::SendErrorMessage(const buzz::XmlElement* stanza,
                                      const buzz::QName& name,
                                      const std::string& type,
                                      const std::string& text,
                                      const buzz::XmlElement* extra_info) {
  talk_base::scoped_ptr<buzz::XmlElement> msg(
      CreateErrorMessage(stanza, name, type, text, extra_info));
  SignalOutgoingMessage(this, msg.get());
}

buzz::XmlElement* GSessionManager::CreateErrorMessage(
    const buzz::XmlElement* stanza,
    const buzz::QName& name,
    const std::string& type,
    const std::string& text,
    const buzz::XmlElement* extra_info) {
  buzz::XmlElement* iq = new buzz::XmlElement(buzz::QN_IQ);
  iq->SetAttr(buzz::QN_TO, stanza->Attr(buzz::QN_FROM));
  iq->SetAttr(buzz::QN_ID, stanza->Attr(buzz::QN_ID));
  iq->SetAttr(buzz::QN_TYPE, "error");

  CopyXmlChildren(stanza, iq);

  buzz::XmlElement* error = new buzz::XmlElement(buzz::QN_ERROR);
  error->SetAttr(buzz::QN_TYPE, type);
  iq->AddElement(error);

  // If the error name is not in the standard namespace, we have to first add
  // some error from that namespace.
  if (name.Namespace() != buzz::NS_STANZA) {
     error->AddElement(
         new buzz::XmlElement(buzz::QN_STANZA_UNDEFINED_CONDITION));
  }
  error->AddElement(new buzz::XmlElement(name));

  if (extra_info)
    error->AddElement(new buzz::XmlElement(*extra_info));

  if (text.size() > 0) {
    // It's okay to always use English here.  This text is for debugging
    // purposes only.
    buzz::XmlElement* text_elem = new buzz::XmlElement(buzz::QN_STANZA_TEXT);
    text_elem->SetAttr(buzz::QN_XML_LANG, "en");
    text_elem->SetBodyText(text);
    error->AddElement(text_elem);
  }

  // TODO: Should we include error codes as well for SIP compatibility?

  return iq;
}

void GSessionManager::OnOutgoingMessage(GSession* session,
                                       const buzz::XmlElement* stanza) {
  SignalOutgoingMessage(this, stanza);
}

void GSessionManager::OnErrorMessage(BaseGSession* session,
                                    const buzz::XmlElement* stanza,
                                    const buzz::QName& name,
                                    const std::string& type,
                                    const std::string& text,
                                    const buzz::XmlElement* extra_info) {
  SendErrorMessage(stanza, name, type, text, extra_info);
}

void GSessionManager::OnRequestSignaling(GSession* session) {
  SignalRequestSignaling();
}

}  // namespace cricket
