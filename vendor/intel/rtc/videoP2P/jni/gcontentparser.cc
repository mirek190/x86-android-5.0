/*
 ** libjingle
 ** Copyright 2004 Google Inc.
 **
 ** Redistribution and use in source and binary forms, with or without
 ** modification, are permitted provided that the following conditions are met:
 **
 **  1. Redistributions of source code must retain the above copyright notice,
 **     this list of conditions and the following disclaimer.
 **  2. Redistributions in binary form must reproduce the above copyright notice,
 **     this list of conditions and the following disclaimer in the documentation
 **     and/or other materials provided with the distribution.
 **  3. The name of the author may not be used to endorse or promote products
 **     derived from this software without specific prior written permission.
 **
 ** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 ** WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 ** MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 ** EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 ** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 ** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 ** OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 ** WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 ** OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 ** ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **/

#include <string>

#include "gcontentparser.h"
#include "talk/base/helpers.h"
#include "talk/base/logging.h"
#include "talk/base/stringencode.h"
#include "talk/base/stringutils.h"
#include "talk/p2p/base/constants.h"
#include "talk/session/media/mediamessages.h"
#include "talk/xmllite/qname.h"
#include "talk/xmllite/xmlconstants.h"
#include "talk/xmpp/constants.h"
#include "talk/session/media/mediasession.h"
#include "talk/p2p/base/parsing.h"

namespace cricket {
// TODO(pthatcher): Move all of the parsing and writing functions into
// mediamessages.cc, with unit tests.
bool ParseGingleAudioCodec(const buzz::XmlElement* element, AudioCodec* out) {
  int id = GetXmlAttr(element, QN_ID, -1);
  if (id < 0)
    return false;

  std::string name = GetXmlAttr(element, QN_NAME, buzz::STR_EMPTY);
  int clockrate = GetXmlAttr(element, QN_CLOCKRATE, 0);
  int bitrate = GetXmlAttr(element, QN_BITRATE, 0);
  int channels = GetXmlAttr(element, QN_CHANNELS, 1);
  *out = AudioCodec(id, name, clockrate, bitrate, channels, 0);
  return true;
}

bool ParseGingleVideoCodec(const buzz::XmlElement* element, VideoCodec* out) {
  int id = GetXmlAttr(element, QN_ID, -1);
  if (id < 0)
    return false;

  std::string name = GetXmlAttr(element, QN_NAME, buzz::STR_EMPTY);
  int width = GetXmlAttr(element, QN_WIDTH, 0);
  int height = GetXmlAttr(element, QN_HEIGHT, 0);
  int framerate = GetXmlAttr(element, QN_FRAMERATE, 0);

  *out = VideoCodec(id, name, width, height, framerate, 0);
  return true;
}

// Parses an ssrc string as a legacy stream.  If it fails, returns
// false and fills an error message.
bool ParseSsrcAsLegacyStream(const std::string& ssrc_str,
                             std::vector<StreamParams>* streams,
                             ParseError* error) {
  if (!ssrc_str.empty()) {
    uint32 ssrc;
    if (!talk_base::FromString(ssrc_str, &ssrc)) {
      return BadParse("Missing or invalid ssrc.", error);
    }

    streams->push_back(StreamParams::CreateLegacy(ssrc));
  }
  return true;
}

void ParseGingleSsrc(const buzz::XmlElement* parent_elem,
                     const buzz::QName& name,
                     MediaContentDescription* media) {
  const buzz::XmlElement* ssrc_elem = parent_elem->FirstNamed(name);
  if (ssrc_elem) {
    ParseError error;
    ParseSsrcAsLegacyStream(
        ssrc_elem->BodyText(), &(media->mutable_streams()), &error);
  }
}

bool ParseCryptoParams(const buzz::XmlElement* element,
                       CryptoParams* out,
                       ParseError* error) {
  if (!element->HasAttr(QN_CRYPTO_SUITE)) {
    return BadParse("crypto: crypto-suite attribute missing ", error);
  } else if (!element->HasAttr(QN_CRYPTO_KEY_PARAMS)) {
    return BadParse("crypto: key-params attribute missing ", error);
  } else if (!element->HasAttr(QN_CRYPTO_TAG)) {
    return BadParse("crypto: tag attribute missing ", error);
  }

  const std::string& crypto_suite = element->Attr(QN_CRYPTO_SUITE);
  const std::string& key_params = element->Attr(QN_CRYPTO_KEY_PARAMS);
  const int tag = GetXmlAttr(element, QN_CRYPTO_TAG, 0);
  const std::string& session_params =
      element->Attr(QN_CRYPTO_SESSION_PARAMS);  // Optional.

  *out = CryptoParams(tag, crypto_suite, key_params, session_params);
  return true;
}


void ParseFeedbackParams(const buzz::XmlElement* element,
                         FeedbackParams* params) {
  for (const buzz::XmlElement* param = element->FirstNamed(QN_JINGLE_RTCP_FB);
       param != NULL; param = param->NextNamed(QN_JINGLE_RTCP_FB)) {
    std::string type = GetXmlAttr(param, QN_TYPE, buzz::STR_EMPTY);
    std::string subtype = GetXmlAttr(param, QN_SUBTYPE, buzz::STR_EMPTY);
    if (!type.empty()) {
      params->Add(FeedbackParam(type, subtype));
    }
  }
}

void AddFeedbackParams(const FeedbackParams& additional_params,
                       FeedbackParams* params) {
  for (size_t i = 0; i < additional_params.params().size(); ++i) {
    params->Add(additional_params.params()[i]);
  }
}

// Parse the first encryption element found with a matching 'usage'
// element.
// <usage/> is specific to Gingle. In Jingle, <crypto/> is already
// scoped to a content.
// Return false if there was an encryption element and it could not be
// parsed.
bool ParseGingleEncryption(const buzz::XmlElement* desc,
                           const buzz::QName& usage,
                           MediaContentDescription* media,
                           ParseError* error) {
  for (const buzz::XmlElement* encryption = desc->FirstNamed(QN_ENCRYPTION);
       encryption != NULL;
       encryption = encryption->NextNamed(QN_ENCRYPTION)) {
    if (encryption->FirstNamed(usage) != NULL) {
      media->set_crypto_required(
          GetXmlAttr(encryption, QN_ENCRYPTION_REQUIRED, false));
      for (const buzz::XmlElement* crypto = encryption->FirstNamed(QN_CRYPTO);
           crypto != NULL;
           crypto = crypto->NextNamed(QN_CRYPTO)) {
        CryptoParams params;
        if (!ParseCryptoParams(crypto, &params, error)) {
          return false;
        }
        media->AddCrypto(params);
      }
      break;
    }
  }
  return true;
}

void ParseBandwidth(const buzz::XmlElement* parent_elem,
                    MediaContentDescription* media) {
  const buzz::XmlElement* bw_elem = GetXmlChild(parent_elem, LN_BANDWIDTH);
  int bandwidth_kbps = -1;
  if (bw_elem && talk_base::FromString(bw_elem->BodyText(), &bandwidth_kbps)) {
    if (bandwidth_kbps >= 0) {
      media->set_bandwidth(bandwidth_kbps * 1000);
    }
  }
}

bool ParseGingleAudioContent(const buzz::XmlElement* content_elem,
                             ContentDescription** content,
                             ParseError* error) {
  AudioContentDescription* audio = new AudioContentDescription();

  if (content_elem->FirstElement()) {
    for (const buzz::XmlElement* codec_elem =
             content_elem->FirstNamed(QN_GINGLE_AUDIO_PAYLOADTYPE);
         codec_elem != NULL;
         codec_elem = codec_elem->NextNamed(QN_GINGLE_AUDIO_PAYLOADTYPE)) {
      AudioCodec codec;
      if (ParseGingleAudioCodec(codec_elem, &codec)) {
        audio->AddCodec(codec);
      }
    }
  } else {
    // For backward compatibility, we can assume the other client is
    // an old version of Talk if it has no audio payload types at all.
    audio->AddCodec(AudioCodec(103, "ISAC", 16000, -1, 1, 1));
    audio->AddCodec(AudioCodec(0, "PCMU", 8000, 64000, 1, 0));
  }

  ParseGingleSsrc(content_elem, QN_GINGLE_AUDIO_SRCID, audio);

  if (!ParseGingleEncryption(content_elem, QN_GINGLE_AUDIO_CRYPTO_USAGE,
                             audio, error)) {
    return false;
  }

  *content = audio;
  return true;
}

bool ParseGingleVideoContent(const buzz::XmlElement* content_elem,
                             ContentDescription** content,
                             ParseError* error) {
  VideoContentDescription* video = new VideoContentDescription();

  for (const buzz::XmlElement* codec_elem =
           content_elem->FirstNamed(QN_GINGLE_VIDEO_PAYLOADTYPE);
       codec_elem != NULL;
       codec_elem = codec_elem->NextNamed(QN_GINGLE_VIDEO_PAYLOADTYPE)) {
    VideoCodec codec;
    if (ParseGingleVideoCodec(codec_elem, &codec)) {
      video->AddCodec(codec);
    }
  }

  ParseGingleSsrc(content_elem, QN_GINGLE_VIDEO_SRCID, video);
  ParseBandwidth(content_elem, video);

  if (!ParseGingleEncryption(content_elem, QN_GINGLE_VIDEO_CRYPTO_USAGE,
                             video, error)) {
    return false;
  }

  *content = video;
  return true;
}

void ParsePayloadTypeParameters(const buzz::XmlElement* element,
                                std::map<std::string, std::string>* paramap) {
  for (const buzz::XmlElement* param = element->FirstNamed(QN_PARAMETER);
       param != NULL; param = param->NextNamed(QN_PARAMETER)) {
    std::string name  = GetXmlAttr(param, QN_PAYLOADTYPE_PARAMETER_NAME,
                                   buzz::STR_EMPTY);
    std::string value = GetXmlAttr(param, QN_PAYLOADTYPE_PARAMETER_VALUE,
                                   buzz::STR_EMPTY);
    if (!name.empty() && !value.empty()) {
      paramap->insert(make_pair(name, value));
    }
  }
}

int FindWithDefault(const std::map<std::string, std::string>& map,
                    const std::string& key, const int def) {
  std::map<std::string, std::string>::const_iterator iter = map.find(key);
  return (iter == map.end()) ? def : atoi(iter->second.c_str());
}


// Parse the first encryption element found.
// Return false if there was an encryption element and it could not be
// parsed.
bool ParseJingleEncryption(const buzz::XmlElement* content_elem,
                           MediaContentDescription* media,
                           ParseError* error) {
  const buzz::XmlElement* encryption =
          content_elem->FirstNamed(QN_ENCRYPTION);
  if (encryption == NULL) {
      return true;
  }

  media->set_crypto_required(
      GetXmlAttr(encryption, QN_ENCRYPTION_REQUIRED, false));

  for (const buzz::XmlElement* crypto = encryption->FirstNamed(QN_CRYPTO);
       crypto != NULL;
       crypto = crypto->NextNamed(QN_CRYPTO)) {
    CryptoParams params;
    if (!ParseCryptoParams(crypto, &params, error)) {
      return false;
    }
    media->AddCrypto(params);
  }
  return true;
}

bool ParseJingleAudioCodec(const buzz::XmlElement* elem, AudioCodec* codec) {
  int id = GetXmlAttr(elem, QN_ID, -1);
  if (id < 0)
    return false;

  std::string name = GetXmlAttr(elem, QN_NAME, buzz::STR_EMPTY);
  int clockrate = GetXmlAttr(elem, QN_CLOCKRATE, 0);
  int channels = GetXmlAttr(elem, QN_CHANNELS, 1);

  std::map<std::string, std::string> paramap;
  ParsePayloadTypeParameters(elem, &paramap);
  int bitrate = FindWithDefault(paramap, PAYLOADTYPE_PARAMETER_BITRATE, 0);

  *codec = AudioCodec(id, name, clockrate, bitrate, channels, 0);
  ParseFeedbackParams(elem, &codec->feedback_params);
  return true;
}

bool ParseJingleVideoCodec(const buzz::XmlElement* elem, VideoCodec* codec) {
  int id = GetXmlAttr(elem, QN_ID, -1);
  if (id < 0)
    return false;

  std::string name = GetXmlAttr(elem, QN_NAME, buzz::STR_EMPTY);

  std::map<std::string, std::string> paramap;
  ParsePayloadTypeParameters(elem, &paramap);
  int width = FindWithDefault(paramap, PAYLOADTYPE_PARAMETER_WIDTH, 0);
  int height = FindWithDefault(paramap, PAYLOADTYPE_PARAMETER_HEIGHT, 0);
  int framerate = FindWithDefault(paramap, PAYLOADTYPE_PARAMETER_FRAMERATE, 0);

  *codec = VideoCodec(id, name, width, height, framerate, 0);
  codec->params = paramap;
  ParseFeedbackParams(elem, &codec->feedback_params);
  return true;
}

bool ParseJingleDataCodec(const buzz::XmlElement* elem, DataCodec* codec) {
  int id = GetXmlAttr(elem, QN_ID, -1);
  if (id < 0)
    return false;

  std::string name = GetXmlAttr(elem, QN_NAME, buzz::STR_EMPTY);

  *codec = DataCodec(id, name, 0);
  ParseFeedbackParams(elem, &codec->feedback_params);
  return true;
}

bool ParseJingleStreamsOrLegacySsrc(const buzz::XmlElement* desc_elem,
                                    MediaContentDescription* media,
                                    ParseError* error) {
  if (HasJingleStreams(desc_elem)) {
    if (!ParseJingleStreams(desc_elem, &(media->mutable_streams()), error)) {
      return false;
    }
  } else {
    const std::string ssrc_str = desc_elem->Attr(QN_SSRC);
    if (!ParseSsrcAsLegacyStream(
            ssrc_str, &(media->mutable_streams()), error)) {
      return false;
    }
  }
  return true;
}

bool ParseJingleAudioContent(const buzz::XmlElement* content_elem,
                             ContentDescription** content,
                             ParseError* error) {
  AudioContentDescription* audio = new AudioContentDescription();

  FeedbackParams content_feedback_params;
  ParseFeedbackParams(content_elem, &content_feedback_params);

  for (const buzz::XmlElement* payload_elem =
           content_elem->FirstNamed(QN_JINGLE_RTP_PAYLOADTYPE);
      payload_elem != NULL;
      payload_elem = payload_elem->NextNamed(QN_JINGLE_RTP_PAYLOADTYPE)) {
    AudioCodec codec;
    if (ParseJingleAudioCodec(payload_elem, &codec)) {
      AddFeedbackParams(content_feedback_params, &codec.feedback_params);
      audio->AddCodec(codec);
    }
  }

  if (!ParseJingleStreamsOrLegacySsrc(content_elem, audio, error)) {
    return false;
  }

  if (!ParseJingleEncryption(content_elem, audio, error)) {
    return false;
  }

  audio->set_rtcp_mux(content_elem->FirstNamed(QN_JINGLE_RTCP_MUX) != NULL);

  RtpHeaderExtensions hdrexts;
  if (!ParseJingleRtpHeaderExtensions(content_elem, &hdrexts, error)) {
    return false;
  }
  audio->set_rtp_header_extensions(hdrexts);

  *content = audio;
  return true;
}

bool ParseJingleVideoContent(const buzz::XmlElement* content_elem,
                             ContentDescription** content,
                             ParseError* error) {
  VideoContentDescription* video = new VideoContentDescription();

  FeedbackParams content_feedback_params;
  ParseFeedbackParams(content_elem, &content_feedback_params);

  for (const buzz::XmlElement* payload_elem =
           content_elem->FirstNamed(QN_JINGLE_RTP_PAYLOADTYPE);
      payload_elem != NULL;
      payload_elem = payload_elem->NextNamed(QN_JINGLE_RTP_PAYLOADTYPE)) {
    VideoCodec codec;
    if (ParseJingleVideoCodec(payload_elem, &codec)) {
      AddFeedbackParams(content_feedback_params, &codec.feedback_params);
      video->AddCodec(codec);
    }
  }

  if (!ParseJingleStreamsOrLegacySsrc(content_elem, video, error)) {
    return false;
  }
  ParseBandwidth(content_elem, video);

  if (!ParseJingleEncryption(content_elem, video, error)) {
    return false;
  }

  video->set_rtcp_mux(content_elem->FirstNamed(QN_JINGLE_RTCP_MUX) != NULL);

  RtpHeaderExtensions hdrexts;
  if (!ParseJingleRtpHeaderExtensions(content_elem, &hdrexts, error)) {
    return false;
  }
  video->set_rtp_header_extensions(hdrexts);

  *content = video;
  return true;
}

bool ParseJingleDataContent(const buzz::XmlElement* content_elem,
                            ContentDescription** content,
                            ParseError* error) {
  DataContentDescription* data = new DataContentDescription();

  FeedbackParams content_feedback_params;
  ParseFeedbackParams(content_elem, &content_feedback_params);

  for (const buzz::XmlElement* payload_elem =
           content_elem->FirstNamed(QN_JINGLE_RTP_PAYLOADTYPE);
      payload_elem != NULL;
      payload_elem = payload_elem->NextNamed(QN_JINGLE_RTP_PAYLOADTYPE)) {
    DataCodec codec;
    if (ParseJingleDataCodec(payload_elem, &codec)) {
      AddFeedbackParams(content_feedback_params, &codec.feedback_params);
      data->AddCodec(codec);
    }
  }

  if (!ParseJingleStreamsOrLegacySsrc(content_elem, data, error)) {
    return false;
  }
  ParseBandwidth(content_elem, data);

  if (!ParseJingleEncryption(content_elem, data, error)) {
    return false;
  }

  data->set_rtcp_mux(content_elem->FirstNamed(QN_JINGLE_RTCP_MUX) != NULL);

  *content = data;
  return true;
}

bool GContentParser::ParseContent(SignalingProtocol protocol,
                                      const buzz::XmlElement* content_elem,
                                      ContentDescription** content,
                                      ParseError* error) {
  if (protocol == PROTOCOL_GINGLE) {
    const std::string& content_type = content_elem->Name().Namespace();
    if (NS_GINGLE_AUDIO == content_type) {
      return ParseGingleAudioContent(content_elem, content, error);
    } else if (NS_GINGLE_VIDEO == content_type) {
      return ParseGingleVideoContent(content_elem, content, error);
    } else {
      return BadParse("Unknown content type: " + content_type, error);
    }
  } else {
    std::string media;
    if (!RequireXmlAttr(content_elem, QN_JINGLE_CONTENT_MEDIA, &media, error))
      return false;

    if (media == JINGLE_CONTENT_MEDIA_AUDIO) {
      return ParseJingleAudioContent(content_elem, content, error);
    } else if (media == JINGLE_CONTENT_MEDIA_VIDEO) {
      return ParseJingleVideoContent(content_elem, content, error);
    } else if (media == JINGLE_CONTENT_MEDIA_DATA) {
      return ParseJingleDataContent(content_elem, content, error);
    } else {
      return BadParse("Unknown media: " + media, error);
    }
  }
}

buzz::XmlElement* CreateGingleAudioCodecElem(const AudioCodec& codec) {
  buzz::XmlElement* payload_type =
      new buzz::XmlElement(QN_GINGLE_AUDIO_PAYLOADTYPE, true);
  AddXmlAttr(payload_type, QN_ID, codec.id);
  payload_type->AddAttr(QN_NAME, codec.name);
  if (codec.clockrate > 0)
    AddXmlAttr(payload_type, QN_CLOCKRATE, codec.clockrate);
  if (codec.bitrate > 0)
    AddXmlAttr(payload_type, QN_BITRATE, codec.bitrate);
  if (codec.channels > 1)
    AddXmlAttr(payload_type, QN_CHANNELS, codec.channels);
  return payload_type;
}

buzz::XmlElement* CreateGingleVideoCodecElem(const VideoCodec& codec) {
  buzz::XmlElement* payload_type =
      new buzz::XmlElement(QN_GINGLE_VIDEO_PAYLOADTYPE, true);
  AddXmlAttr(payload_type, QN_ID, codec.id);
  payload_type->AddAttr(QN_NAME, codec.name);
  AddXmlAttr(payload_type, QN_WIDTH, codec.width);
  AddXmlAttr(payload_type, QN_HEIGHT, codec.height);
  AddXmlAttr(payload_type, QN_FRAMERATE, codec.framerate);
  return payload_type;
}

buzz::XmlElement* CreateGingleSsrcElem(const buzz::QName& name, uint32 ssrc) {
  buzz::XmlElement* elem = new buzz::XmlElement(name, true);
  if (ssrc) {
    SetXmlBody(elem, ssrc);
  }
  return elem;
}

buzz::XmlElement* CreateBandwidthElem(const buzz::QName& name, int bps) {
  int kbps = bps / 1000;
  buzz::XmlElement* elem = new buzz::XmlElement(name);
  elem->AddAttr(buzz::QN_TYPE, "AS");
  SetXmlBody(elem, kbps);
  return elem;
}

// For Jingle, usage_qname is empty.
buzz::XmlElement* CreateJingleEncryptionElem(const CryptoParamsVec& cryptos,
                                             bool required) {
  buzz::XmlElement* encryption_elem = new buzz::XmlElement(QN_ENCRYPTION);

  if (required) {
    encryption_elem->SetAttr(QN_ENCRYPTION_REQUIRED, "true");
  }

  for (CryptoParamsVec::const_iterator i = cryptos.begin();
       i != cryptos.end();
       ++i) {
    buzz::XmlElement* crypto_elem = new buzz::XmlElement(QN_CRYPTO);

    AddXmlAttr(crypto_elem, QN_CRYPTO_TAG, i->tag);
    crypto_elem->AddAttr(QN_CRYPTO_SUITE, i->cipher_suite);
    crypto_elem->AddAttr(QN_CRYPTO_KEY_PARAMS, i->key_params);
    if (!i->session_params.empty()) {
      crypto_elem->AddAttr(QN_CRYPTO_SESSION_PARAMS, i->session_params);
    }
    encryption_elem->AddElement(crypto_elem);
  }
  return encryption_elem;
}

buzz::XmlElement* CreateGingleEncryptionElem(const CryptoParamsVec& cryptos,
                                             const buzz::QName& usage_qname,
                                             bool required) {
  buzz::XmlElement* encryption_elem =
      CreateJingleEncryptionElem(cryptos, required);

  if (required) {
    encryption_elem->SetAttr(QN_ENCRYPTION_REQUIRED, "true");
  }

  buzz::XmlElement* usage_elem = new buzz::XmlElement(usage_qname);
  encryption_elem->AddElement(usage_elem);

  return encryption_elem;
}

buzz::XmlElement* CreateGingleAudioContentElem(
    const AudioContentDescription* audio,
    bool crypto_required) {
  buzz::XmlElement* elem =
      new buzz::XmlElement(QN_GINGLE_AUDIO_CONTENT, true);

  for (AudioCodecs::const_iterator codec = audio->codecs().begin();
       codec != audio->codecs().end(); ++codec) {
    elem->AddElement(CreateGingleAudioCodecElem(*codec));
  }
  if (audio->has_ssrcs()) {
    elem->AddElement(CreateGingleSsrcElem(
        QN_GINGLE_AUDIO_SRCID, audio->first_ssrc()));
  }

  const CryptoParamsVec& cryptos = audio->cryptos();
  if (!cryptos.empty()) {
    elem->AddElement(CreateGingleEncryptionElem(cryptos,
                                                QN_GINGLE_AUDIO_CRYPTO_USAGE,
                                                crypto_required));
  }
  return elem;
}

buzz::XmlElement* CreateGingleVideoContentElem(
    const VideoContentDescription* video,
    bool crypto_required) {
  buzz::XmlElement* elem =
      new buzz::XmlElement(QN_GINGLE_VIDEO_CONTENT, true);

  for (VideoCodecs::const_iterator codec = video->codecs().begin();
       codec != video->codecs().end(); ++codec) {
    elem->AddElement(CreateGingleVideoCodecElem(*codec));
  }
  if (video->has_ssrcs()) {
    elem->AddElement(CreateGingleSsrcElem(
        QN_GINGLE_VIDEO_SRCID, video->first_ssrc()));
  }
  if (video->bandwidth() != kAutoBandwidth) {
    elem->AddElement(CreateBandwidthElem(QN_GINGLE_VIDEO_BANDWIDTH,
                                         video->bandwidth()));
  }

  const CryptoParamsVec& cryptos = video->cryptos();
  if (!cryptos.empty()) {
    elem->AddElement(CreateGingleEncryptionElem(cryptos,
                                                QN_GINGLE_VIDEO_CRYPTO_USAGE,
                                                crypto_required));
  }

  return elem;
}

template <class T>
buzz::XmlElement* CreatePayloadTypeParameterElem(
    const std::string& name, T value) {
  buzz::XmlElement* elem = new buzz::XmlElement(QN_PARAMETER);

  elem->AddAttr(QN_PAYLOADTYPE_PARAMETER_NAME, name);
  AddXmlAttr(elem, QN_PAYLOADTYPE_PARAMETER_VALUE, value);

  return elem;
}

void AddRtcpFeedbackElem(buzz::XmlElement* elem,
                      const FeedbackParams& feedback_params) {
  std::vector<FeedbackParam>::const_iterator it;
  for (it = feedback_params.params().begin();
       it != feedback_params.params().end(); ++it) {
    buzz::XmlElement* fb_elem = new buzz::XmlElement(QN_JINGLE_RTCP_FB);
    fb_elem->AddAttr(QN_TYPE, it->id());
    fb_elem->AddAttr(QN_SUBTYPE, it->param());
    elem->AddElement(fb_elem);
  }
}

buzz::XmlElement* CreateJingleAudioCodecElem(const AudioCodec& codec) {
  buzz::XmlElement* elem = new buzz::XmlElement(QN_JINGLE_RTP_PAYLOADTYPE);

  AddXmlAttr(elem, QN_ID, codec.id);
  elem->AddAttr(QN_NAME, codec.name);
  if (codec.clockrate > 0) {
    AddXmlAttr(elem, QN_CLOCKRATE, codec.clockrate);
  }
  if (codec.bitrate > 0) {
    elem->AddElement(CreatePayloadTypeParameterElem(
        PAYLOADTYPE_PARAMETER_BITRATE, codec.bitrate));
  }
  if (codec.channels > 1) {
    AddXmlAttr(elem, QN_CHANNELS, codec.channels);
  }

  AddRtcpFeedbackElem(elem, codec.feedback_params);

  return elem;
}

buzz::XmlElement* CreateJingleVideoCodecElem(const VideoCodec& codec) {
  buzz::XmlElement* elem = new buzz::XmlElement(QN_JINGLE_RTP_PAYLOADTYPE);

  AddXmlAttr(elem, QN_ID, codec.id);
  elem->AddAttr(QN_NAME, codec.name);
  elem->AddElement(CreatePayloadTypeParameterElem(
      PAYLOADTYPE_PARAMETER_WIDTH, codec.width));
  elem->AddElement(CreatePayloadTypeParameterElem(
      PAYLOADTYPE_PARAMETER_HEIGHT, codec.height));
  elem->AddElement(CreatePayloadTypeParameterElem(
      PAYLOADTYPE_PARAMETER_FRAMERATE, codec.framerate));

  AddRtcpFeedbackElem(elem, codec.feedback_params);

  CodecParameterMap::const_iterator param_iter;
  for (param_iter = codec.params.begin(); param_iter != codec.params.end();
       ++param_iter) {
    elem->AddElement(CreatePayloadTypeParameterElem(param_iter->first,
                                                    param_iter->second));
  }

  return elem;
}

buzz::XmlElement* CreateJingleDataCodecElem(const DataCodec& codec) {
  buzz::XmlElement* elem = new buzz::XmlElement(QN_JINGLE_RTP_PAYLOADTYPE);

  AddXmlAttr(elem, QN_ID, codec.id);
  elem->AddAttr(QN_NAME, codec.name);

  AddRtcpFeedbackElem(elem, codec.feedback_params);

  return elem;
}

void WriteLegacyJingleSsrc(const MediaContentDescription* media,
                           buzz::XmlElement* elem) {
  if (media->has_ssrcs()) {
    AddXmlAttr(elem, QN_SSRC, media->first_ssrc());
  }
}

void WriteJingleStreamsOrLegacySsrc(const MediaContentDescription* media,
                                    buzz::XmlElement* desc_elem) {
  if (!media->multistream()) {
    WriteLegacyJingleSsrc(media, desc_elem);
  } else {
    WriteJingleStreams(media->streams(), desc_elem);
  }
}

buzz::XmlElement* CreateJingleAudioContentElem(
    const AudioContentDescription* audio, bool crypto_required) {
  buzz::XmlElement* elem =
      new buzz::XmlElement(QN_JINGLE_RTP_CONTENT, true);

  elem->SetAttr(QN_JINGLE_CONTENT_MEDIA, JINGLE_CONTENT_MEDIA_AUDIO);
  WriteJingleStreamsOrLegacySsrc(audio, elem);

  for (AudioCodecs::const_iterator codec = audio->codecs().begin();
       codec != audio->codecs().end(); ++codec) {
    elem->AddElement(CreateJingleAudioCodecElem(*codec));
  }

  const CryptoParamsVec& cryptos = audio->cryptos();
  if (!cryptos.empty()) {
    elem->AddElement(CreateJingleEncryptionElem(cryptos, crypto_required));
  }

  if (audio->rtcp_mux()) {
    elem->AddElement(new buzz::XmlElement(QN_JINGLE_RTCP_MUX));
  }

  WriteJingleRtpHeaderExtensions(audio->rtp_header_extensions(), elem);

  return elem;
}

buzz::XmlElement* CreateJingleVideoContentElem(
    const VideoContentDescription* video, bool crypto_required) {
  buzz::XmlElement* elem =
      new buzz::XmlElement(QN_JINGLE_RTP_CONTENT, true);

  elem->SetAttr(QN_JINGLE_CONTENT_MEDIA, JINGLE_CONTENT_MEDIA_VIDEO);
  WriteJingleStreamsOrLegacySsrc(video, elem);

  for (VideoCodecs::const_iterator codec = video->codecs().begin();
       codec != video->codecs().end(); ++codec) {
    elem->AddElement(CreateJingleVideoCodecElem(*codec));
  }

  const CryptoParamsVec& cryptos = video->cryptos();
  if (!cryptos.empty()) {
    elem->AddElement(CreateJingleEncryptionElem(cryptos, crypto_required));
  }

  if (video->rtcp_mux()) {
    elem->AddElement(new buzz::XmlElement(QN_JINGLE_RTCP_MUX));
  }

  if (video->bandwidth() != kAutoBandwidth) {
    elem->AddElement(CreateBandwidthElem(QN_JINGLE_RTP_BANDWIDTH,
                                         video->bandwidth()));
  }

  WriteJingleRtpHeaderExtensions(video->rtp_header_extensions(), elem);

  return elem;
}

buzz::XmlElement* CreateJingleDataContentElem(
    const DataContentDescription* data, bool crypto_required) {
  buzz::XmlElement* elem =
      new buzz::XmlElement(QN_JINGLE_RTP_CONTENT, true);

  elem->SetAttr(QN_JINGLE_CONTENT_MEDIA, JINGLE_CONTENT_MEDIA_DATA);
  WriteJingleStreamsOrLegacySsrc(data, elem);

  for (DataCodecs::const_iterator codec = data->codecs().begin();
       codec != data->codecs().end(); ++codec) {
    elem->AddElement(CreateJingleDataCodecElem(*codec));
  }

  const CryptoParamsVec& cryptos = data->cryptos();
  if (!cryptos.empty()) {
    elem->AddElement(CreateJingleEncryptionElem(cryptos, crypto_required));
  }

  if (data->rtcp_mux()) {
    elem->AddElement(new buzz::XmlElement(QN_JINGLE_RTCP_MUX));
  }

  if (data->bandwidth() != kAutoBandwidth) {
    elem->AddElement(CreateBandwidthElem(QN_JINGLE_RTP_BANDWIDTH,
                                         data->bandwidth()));
  }

  return elem;
}

bool GContentParser::IsWritable(SignalingProtocol protocol,
                                    const ContentDescription* content) {
  const MediaContentDescription* media =
      static_cast<const MediaContentDescription*>(content);
  if (protocol == PROTOCOL_GINGLE &&
      media->type() == MEDIA_TYPE_DATA) {
    return false;
  }
  return true;
}

bool GContentParser::WriteContent(SignalingProtocol protocol,
                                      const ContentDescription* content,
                                      buzz::XmlElement** elem,
                                      WriteError* error) {
  const MediaContentDescription* media =
      static_cast<const MediaContentDescription*>(content);
  //bool crypto_required = secure() == SEC_REQUIRED;
  bool crypto_required = false;

  if (media->type() == MEDIA_TYPE_AUDIO) {
    const AudioContentDescription* audio =
        static_cast<const AudioContentDescription*>(media);
    if (protocol == PROTOCOL_GINGLE) {
      *elem = CreateGingleAudioContentElem(audio, crypto_required);
    } else {
      *elem = CreateJingleAudioContentElem(audio, crypto_required);
    }
  } else if (media->type() == MEDIA_TYPE_VIDEO) {
    const VideoContentDescription* video =
        static_cast<const VideoContentDescription*>(media);
    if (protocol == PROTOCOL_GINGLE) {
      *elem = CreateGingleVideoContentElem(video, crypto_required);
    } else {
      *elem = CreateJingleVideoContentElem(video, crypto_required);
    }
  } else if (media->type() == MEDIA_TYPE_DATA) {
    const DataContentDescription* data =
        static_cast<const DataContentDescription*>(media);
    if (protocol == PROTOCOL_GINGLE) {
      return BadWrite("Data channel not supported with Gingle.", error);
    } else {
      *elem = CreateJingleDataContentElem(data, crypto_required);
    }
  } else {
    return BadWrite("Unknown content type: " +
                    talk_base::ToString<int>(media->type()), error);
  }

  return true;
}

}
