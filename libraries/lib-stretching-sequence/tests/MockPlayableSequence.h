/*  SPDX-License-Identifier: GPL-2.0-or-later */
/*!********************************************************************

  Audacity: A Digital Audio Editor

  MockPlayableSequence.h

  Matthieu Hodgkinson

**********************************************************************/
#pragma once

#include "AudioIOSequences.h"

class MockPlayableSequence final : public PlayableSequence
{
public:
   MockPlayableSequence(int sampleRate, size_t numChannels)
       : sampleRate(sampleRate)
       , numChannels(numChannels)
   {
   }

   // WideSampleSequence
   bool Get(
      size_t iChannel, size_t nBuffers, samplePtr buffers[],
      sampleFormat format, sampleCount start, size_t len, bool backwards,
      fillFormat fill = fillZero, bool mayThrow = true,
      sampleCount* pNumWithinClips = nullptr) const override
   {
      return true;
   }

   size_t NChannels() const override
   {
      return numChannels;
   }

   float GetChannelGain(int channel) const override
   {
      return 1.f;
   }

   double GetStartTime() const override
   {
      return 0.;
   }

   double GetEndTime() const override
   {
      return 0.;
   }

   double GetRate() const override
   {
      return sampleRate;
   }

   sampleFormat WidestEffectiveFormat() const override
   {
      return floatSample;
   }

   bool HasTrivialEnvelope() const override
   {
      return true;
   }

   void GetEnvelopeValues(
      double* buffer, size_t bufferLen, double t0,
      bool backwards) const override
   {
   }

   // AudioGraph::Channel
   AudioGraph::ChannelType GetChannelType() const override
   {
      return AudioGraph::MonoChannel;
   }

   // PlayableSequence
   bool IsLeader() const override
   {
      return false;
   }

   bool GetSolo() const override
   {
      return false;
   }

   bool GetMute() const override
   {
      return false;
   }

   const int sampleRate;
   const size_t numChannels;
};
