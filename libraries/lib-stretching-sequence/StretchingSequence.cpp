/*  SPDX-License-Identifier: GPL-2.0-or-later */
/*!********************************************************************

  Audacity: A Digital Audio Editor

  StretchingSequence.cpp

  Matthieu Hodgkinson

**********************************************************************/
#include "StretchingSequence.h"
#include "AudioSegment.h"
#include "AudioSegmentFactory.h"
#include "StaffPadTimeAndPitch.h"

#include <cassert>

namespace
{
std::vector<float*>
GetOffsetBuffer(float* const* buffer, size_t numChannels, size_t offset)
{
   std::vector<float*> offsetBuffer(numChannels);
   for (auto i = 0u; i < numChannels; ++i)
      offsetBuffer[i] = buffer[i] + offset;
   return offsetBuffer;
}
} // namespace

StretchingSequence::StretchingSequence(
   const PlayableSequence& sequence, int sampleRate, size_t numChannels,
   std::unique_ptr<AudioSegmentFactoryInterface> factory)
    : mSequence { sequence }
    , mAudioSegmentFactory { std::move(factory) }
{
}

void StretchingSequence::ResetCursor(double t, PlaybackDirection direction)
{
   mAudioSegments =
      mAudioSegmentFactory->CreateAudioSegmentSequence(t, direction);
   mActiveAudioSegmentIt = mAudioSegments.begin();
   mPlaybackDirection = direction;
   mExpectedStart = TimeToLongSamples(t);
}

bool StretchingSequence::GetNext(
   float* buffers[], size_t numChannels, size_t numSamples)
{
   if (!mExpectedStart.has_value())
      ResetCursor(0., PlaybackDirection::forward);
   auto numProcessedSamples = 0u;
   while (numProcessedSamples < numSamples &&
          mActiveAudioSegmentIt != mAudioSegments.end())
   {
      const auto& segment = *mActiveAudioSegmentIt;
      auto offsetBuffers =
         GetOffsetBuffer(buffers, mSequence.NChannels(), numProcessedSamples);
      numProcessedSamples += segment->GetFloats(
         offsetBuffers,
         numSamples - numProcessedSamples); // No need to reverse, we feed the
                                            // time-stretching algorithm with
                                            // reversed samples already.
      if (segment->Empty())
         ++mActiveAudioSegmentIt;
   }
   const auto remaining = numSamples - numProcessedSamples;
   if (remaining > 0u)
   {
      const auto offsetBuffers =
         GetOffsetBuffer(buffers, mSequence.NChannels(), numProcessedSamples);
      for (auto i = 0u; i < mSequence.NChannels(); ++i)
         std::fill(offsetBuffers[i], offsetBuffers[i] + remaining, 0.f);
   }
   mExpectedStart =
      mPlaybackDirection == PlaybackDirection::forward ?
         *mExpectedStart + numSamples :
         *mExpectedStart - static_cast<sampleCount::type>(numSamples);
   return true;
}

size_t StretchingSequence::NChannels() const
{
   return mSequence.NChannels();
}

float StretchingSequence::GetChannelGain(int channel) const
{
   return mSequence.GetChannelGain(channel);
}

bool StretchingSequence::Get(
   size_t iChannel, size_t nBuffers, samplePtr buffers[], sampleFormat format,
   sampleCount start, size_t len, bool backwards, fillFormat fill,
   bool mayThrow, sampleCount* pNumWithinClips) const
{
   return const_cast<StretchingSequence&>(*this).MutableGet(
      iChannel, nBuffers, buffers, format, start, len, backwards);
}

bool StretchingSequence::IsLeader() const
{
   return mSequence.IsLeader();
}

bool StretchingSequence::GetSolo() const
{
   return mSequence.GetSolo();
}

bool StretchingSequence::GetMute() const
{
   return mSequence.GetMute();
}

double StretchingSequence::GetStartTime() const
{
   return mSequence.GetStartTime();
}

double StretchingSequence::GetEndTime() const
{
   return mSequence.GetEndTime();
}

double StretchingSequence::GetRate() const
{
   return mSequence.GetRate();
}

sampleFormat StretchingSequence::WidestEffectiveFormat() const
{
   return mSequence.WidestEffectiveFormat();
}

bool StretchingSequence::HasTrivialEnvelope() const
{
   return mSequence.HasTrivialEnvelope();
}

void StretchingSequence::GetEnvelopeValues(
   double* buffer, size_t bufferLen, double t0, bool backwards) const
{
   mSequence.GetEnvelopeValues(buffer, bufferLen, t0, backwards);
}

AudioGraph::ChannelType StretchingSequence::GetChannelType() const
{
   return mSequence.GetChannelType();
}

bool StretchingSequence::GetFloats(
   float* buffers[], sampleCount start, size_t len, bool backwards) const
{
   std::vector<samplePtr> charBuffers;
   const auto nChannels = NChannels();
   charBuffers.reserve(nChannels);
   for (auto i = 0u; i < nChannels; ++i)
      charBuffers.push_back(reinterpret_cast<samplePtr>(buffers[i]));
   constexpr auto iChannel = 0u;
   return Get(
      iChannel, nChannels, charBuffers.data(), sampleFormat::floatSample, start,
      len, backwards);
}

const WideSampleSequence* StretchingSequence::DoGetDecorated() const
{
   return &mSequence;
}

bool StretchingSequence::MutableGet(
   size_t iChannel, size_t nBuffers, samplePtr buffers[], sampleFormat format,
   sampleCount start, size_t len, bool backwards)
{
   // StretchingSequence is not expected to be used for any other case.
   assert(iChannel == 0u);
   if (
      !mExpectedStart.has_value() || *mExpectedStart != start ||
      (mPlaybackDirection == PlaybackDirection::backward != backwards))
   {
      const auto t = start.as_double() / mSequence.GetRate();
      ResetCursor(
         t,
         backwards ? PlaybackDirection::backward : PlaybackDirection::forward);
   }
   return GetNext(reinterpret_cast<float**>(buffers), nBuffers, len);
}

std::shared_ptr<StretchingSequence> StretchingSequence::Create(
   const PlayableSequence& sequence, const ClipConstHolders& clips)
{
   return std::make_shared<StretchingSequence>(
      sequence, sequence.GetRate(), sequence.NChannels(),
      std::make_unique<AudioSegmentFactory>(
         sequence.GetRate(), sequence.NChannels(), clips));
}

std::shared_ptr<StretchingSequence> StretchingSequence::Create(
   const PlayableSequence& sequence, const ClipHolders& clips)
{
   return Create(sequence, ClipConstHolders { clips.begin(), clips.end() });
}
