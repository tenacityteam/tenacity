/*  SPDX-License-Identifier: GPL-2.0-or-later */
/**********************************************************************
 
 Audacity: A Digital Audio Editor
 
 @file WaveformApperance.cpp
 
 Paul Licameli
 
 **********************************************************************/

#include "WaveformAppearance.h"
#include "WaveClip.h"


static const AttachedTrackObjects::RegisteredFactory keyWA{
   [](Track &track) -> std::shared_ptr<WaveformAppearance> {
      if (const auto pTrack = dynamic_cast<WaveTrack *>(&track))
         return std::make_shared<WaveformAppearance>(*pTrack);
      else
         return nullptr;
   }
};

WaveformAppearance &WaveformAppearance::Get(WaveTrack &track)
{
   return track.AttachedObjects::Get<WaveformAppearance>(keyWA);
}

const WaveformAppearance &WaveformAppearance::Get(const WaveTrack &track)
{
   return Get(const_cast<WaveTrack&>(track));
}

WaveformAppearance &WaveformAppearance::Get(WaveChannel &channel)
{
   return Get(channel.GetTrack());
}

const WaveformAppearance &WaveformAppearance::Get(const WaveChannel &channel)
{
   return Get(const_cast<WaveChannel &>(channel));
}

WaveformAppearance::WaveformAppearance(WaveTrack &track)
   : mwTrack{
      std::static_pointer_cast<WaveTrack>(track.shared_from_this()) }
{
   Subscribe(mwTrack.lock());
}

WaveformAppearance::~WaveformAppearance() = default;

void WaveformAppearance::Subscribe(const std::shared_ptr<WaveTrack> &pTrack)
{
   if (pTrack)
      mSubscription =
      pTrack->Subscribe([this](const WaveTrackMessage &message){
         switch (message.type) {
         case WaveTrackMessage::New:
         case WaveTrackMessage::Deserialized:
            WaveColorAttachment::Get(*message.pClip).SetColorValue(mColorValue);
         default:
            break;
         }
      });
   else
      mSubscription.Reset();
}

void WaveformAppearance::CopyTo(Track &track) const
{
   if (const auto pTrack = dynamic_cast<WaveTrack *>(&track)) {
      auto &other = Get(*pTrack);
      other.mColorValue = mColorValue;
   }
}

void WaveformAppearance::Reparent(const std::shared_ptr<Track> &parent)
{
   mwTrack = std::dynamic_pointer_cast<WaveTrack>(parent);
   Subscribe(mwTrack.lock());
}

static constexpr auto ColorValue_attr = "color";

void WaveformAppearance::WriteXMLAttributes(XMLWriter &writer) const
{
   writer.WriteAttr(ColorValue_attr, mColorValue);
}

bool WaveformAppearance::HandleXMLAttribute(
   const std::string_view& attr, const XMLAttributeValueView& valueView)
{
   long nValue;
   if (attr == ColorValue_attr && valueView.TryGet(nValue))
      mColorValue = nValue;
   return false;
}

void WaveformAppearance::SetColorValue(int colorIndex)
{
   mColorValue = colorIndex;
   const auto pTrack = mwTrack.lock();
   if (!pTrack)
      return;
   for (const auto &pInterval : pTrack->Intervals())
      for (const auto &pChannel : pInterval->Channels())
         WaveColorAttachment::Get(*pChannel)
            .SetColorValue(colorIndex);
}

WaveColorAttachment::WaveColorAttachment()
{
}

WaveColorAttachment::~WaveColorAttachment()
{
}

std::unique_ptr<WaveClipListener> WaveColorAttachment::Clone() const
{
   auto result = std::make_unique<WaveColorAttachment>(*this);
   return result;
}

static WaveClip::Attachments::RegisteredFactory keyWCA{ [](WaveClip &clip) {
   return std::make_unique<WaveColorAttachment>();
} };

WaveColorAttachment &WaveColorAttachment::Get(WaveClip &clip)
{
   return clip.Attachments::Get<WaveColorAttachment>(keyWCA);
}

const WaveColorAttachment &WaveColorAttachment::Get(const WaveClip &clip)
{
   return const_cast<WaveClip&>(clip)
      .Attachments::Get<WaveColorAttachment>(keyWCA);
}

WaveColorAttachment &WaveColorAttachment::Get(WaveChannelInterval &clip)
{
   return Get(clip.GetClip());
}

const WaveColorAttachment &WaveColorAttachment::Get(const WaveChannelInterval &clip)
{
   return Get(clip.GetClip());
}

void WaveColorAttachment::MarkChanged() noexcept {}

void WaveColorAttachment::Invalidate() {}

void WaveColorAttachment::WriteXMLAttributes(XMLWriter &writer) const
{
   writer.WriteAttr(ColorValue_attr, mColorValue);
}

bool WaveColorAttachment::HandleXMLAttribute(const std::string_view &attr,
   const XMLAttributeValueView &valueView)
{
   long longValue;
   if (attr == ColorValue_attr) {
      if (!valueView.TryGet(longValue))
         return false;
      SetColorValue(longValue);
      return true;
   }
   return false;
}
