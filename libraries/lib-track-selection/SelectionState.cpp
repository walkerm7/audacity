/**********************************************************************

 Audacity: A Digital Audio Editor

 SelectionState.h

 **********************************************************************/

#include "SelectionState.h"

#include "ViewInfo.h"
#include "SyncLock.h"
#include "Track.h"
#include "Project.h"

#include <cassert>

static const AudacityProject::AttachedObjects::RegisteredFactory key{
  [](AudacityProject &){ return std::make_shared< SelectionState >(); }
};

SelectionState &SelectionState::Get( AudacityProject &project )
{
   return project.AttachedObjects::Get< SelectionState >( key );
}

const SelectionState &SelectionState::Get( const AudacityProject &project )
{
   return Get( const_cast< AudacityProject & >( project ) );
}

// Set selection length to the length of a track -- but if sync-lock is turned
// on, use the largest possible selection in the sync-lock group.
// If it's a stereo track, do the same for the stereo channels.
void SelectionState::SelectTrackLength
( ViewInfo &viewInfo, Track &track, bool syncLocked )
{
   auto trackRange = syncLocked
   // If we have a sync-lock group and sync-lock linking is on,
   // check the sync-lock group tracks.
   ? SyncLock::Group(&track)

   // Otherwise, check for a stereo pair
   : TrackList::Channels(&track);

   auto minOffset = trackRange.min( &Track::GetOffset );
   auto maxEnd = trackRange.max( &Track::GetEndTime );

   // PRL: double click or click on track control.
   // should this select all frequencies too?  I think not.
   viewInfo.selectedRegion.setTimes(minOffset, maxEnd);
}

void SelectionState::SelectTrack(
   Track &track, bool selected, bool updateLastPicked)
{
   assert(track.IsLeader());
   //bool wasCorrect = (selected == track.GetSelected());

   track.SetSelected(selected);

   if (updateLastPicked)
      mLastPickedTrack = track.SharedPointer();

//The older code below avoids an anchor on an unselected track.

   /*
   if (selected) {
      // This handles the case of linked tracks, selecting all channels
      mTracks->Select(pTrack, true);
      if (updateLastPicked)
         mLastPickedTrack = Track::Pointer( pTrack );
   }
   else {
      mTracks->Select(pTrack, false);
      if (updateLastPicked && pTrack == mLastPickedTrack.lock().get())
         mLastPickedTrack.reset();
   }
*/
}

void SelectionState::SelectRangeOfTracks(
   TrackList &tracks, Track &rsTrack, Track &reTrack)
{
   Track *sTrack = &rsTrack, *eTrack = &reTrack;
   // Swap the track pointers if needed
   auto begin = tracks.Leaders().begin(),
      iterS = tracks.FindLeader(sTrack),
      iterE = tracks.FindLeader(eTrack);
   // Be sure to substitute the leaders for given tracks
   sTrack = *iterS;
   eTrack = *iterE;
   auto indS = std::distance(begin, iterS),
      indE = std::distance(begin, iterE);
   if (indE < indS)
      std::swap(sTrack, eTrack);

   for (auto track :
        tracks.Leaders().StartingWith(sTrack).EndingAfter(eTrack))
      SelectTrack(*track, true, false);
}

void SelectionState::SelectNone(TrackList &tracks)
{
   for (auto t : tracks.Leaders())
      SelectTrack(*t, false, false);
}

void SelectionState::ChangeSelectionOnShiftClick(
   TrackList &tracks, Track &track)
{
   // We will either extend from the first or from the last.
   auto pExtendFrom = tracks.Lock(mLastPickedTrack);

   if (!pExtendFrom) {
      auto trackRange = tracks.SelectedLeaders();
      auto pFirst = *trackRange.begin();

      // If our track is at or after the first, extend from the first.
      if (pFirst) {
         auto begin = tracks.Leaders().begin(),
            iterT = tracks.FindLeader(&track),
            iterF = tracks.FindLeader(pFirst);
         auto indT = std::distance(begin, iterT),
            indF = std::distance(begin, iterF);
         if (indT >= indF)
            pExtendFrom = pFirst->SharedPointer();
      }

      // Our track was earlier than the first.  Extend from the last.
      if (!pExtendFrom)
         pExtendFrom = Track::SharedPointer(*trackRange.rbegin());
   }
   // Either it's null, or mLastPickedTrack, or the first or last of
   // SelectedLeaders()
   assert(!pExtendFrom || pExtendFrom->IsLeader());

   SelectNone(tracks);
   if (pExtendFrom)
      SelectRangeOfTracks(tracks, track, *pExtendFrom);
   else
      SelectTrack(track, true, true);
   mLastPickedTrack = pExtendFrom;
}

void SelectionState::HandleListSelection(TrackList &tracks, ViewInfo &viewInfo,
  Track &track, bool shift, bool ctrl, bool syncLocked)
{
   assert(track.IsLeader());
   // AS: If the shift button is being held down, invert
   //  the selection on this track.
   if (ctrl)
      SelectTrack(track, !track.GetSelected(), true);
   else {
      if (shift && mLastPickedTrack.lock())
         ChangeSelectionOnShiftClick(tracks, track);
      else {
         SelectNone(tracks);
         SelectTrack(track, true, true);
         SelectTrackLength(viewInfo, track, syncLocked);
      }
   }
}

SelectionStateChanger::SelectionStateChanger
( SelectionState &state, TrackList &tracks )
   : mpState{ &state }
   , mTracks{ tracks }
   , mInitialLastPickedTrack{ state.mLastPickedTrack }
{
   // Save initial state of track selections
   const auto range = tracks.Leaders();
   mInitialTrackSelection.clear();
   mInitialTrackSelection.reserve(range.size());
   for (const auto track : range) {
      const bool isSelected = track->GetSelected();
      mInitialTrackSelection.push_back(isSelected);
   }
}

SelectionStateChanger::~SelectionStateChanger()
{
   if ( mpState ) {
      // roll back changes
      mpState->mLastPickedTrack = mInitialLastPickedTrack;
      std::vector<bool>::const_iterator
         it = mInitialTrackSelection.begin(),
         end = mInitialTrackSelection.end();

      for (auto track : mTracks.Leaders()) {
         if (it == end)
            break;
         track->SetSelected( *it++ );
      }
   }
}

void SelectionStateChanger::Commit()
{
   mpState = nullptr;
}
