/**********************************************************************

  Audacity: A Digital Audio Editor

  WaveTrack.h

  Dominic Mazzoni

**********************************************************************/

#ifndef __AUDACITY_WAVETRACK__
#define __AUDACITY_WAVETRACK__

#include "Prefs.h"
#include "SampleCount.h"
#include "SampleFormat.h"
#include "SampleTrack.h"

#include <vector>
#include <functional>
#include <wx/thread.h>
#include <wx/longlong.h>

namespace BasicUI{ class ProgressDialog; }

class SampleBlockFactory;
using SampleBlockFactoryPtr = std::shared_ptr<SampleBlockFactory>;

class TimeWarper;

class ClipInterface;
class Sequence;
class WaveClip;
class AudioSegmentSampleView;

//! Clips are held by shared_ptr, not for sharing, but to allow weak_ptr
using WaveClipHolder = std::shared_ptr<WaveClip>;
using WaveClipHolders = std::vector<WaveClipHolder>;
using WaveClipConstHolders = std::vector < std::shared_ptr< const WaveClip > >;

using ClipConstHolders = std::vector<std::shared_ptr<const ClipInterface>>;

// Temporary arrays of mere pointers
using WaveClipPointers = std::vector < WaveClip* >;
using WaveClipConstPointers = std::vector < const WaveClip* >;

using ChannelSampleView = std::vector<AudioSegmentSampleView>;

//
// Tolerance for merging wave tracks (in seconds)
//
#define WAVETRACK_MERGE_POINT_TOLERANCE 0.01

class Envelope;

class WAVE_TRACK_API WaveTrack final
   : public WritableSampleTrack
   , public Channel
{
public:
   /// \brief Structure to hold region of a wavetrack and a comparison function
   /// for sortability.
   struct Region
   {
      Region() : start(0), end(0) {}
      Region(double start_, double end_) : start(start_), end(end_) {}

      double start, end;

      //used for sorting
      bool operator < (const Region &b) const
      {
         return this->start < b.start;
      }
   };

   using Regions = std::vector < Region >;

   static wxString GetDefaultAudioTrackNamePreference();

   //
   // Constructor / Destructor / Duplicator
   //

   // Construct and also build all attachments
   static WaveTrack *New( AudacityProject &project );

   WaveTrack(
      const SampleBlockFactoryPtr &pFactory, sampleFormat format, double rate);
   //! Copied only in WaveTrack::Clone() !
   WaveTrack(const WaveTrack &orig, ProtectedCreationArg&&);

   //! The width of every WaveClip in this track; for now always 1
   size_t GetWidth() const;

   //! May report more than one only when this is a leader track
   size_t NChannels() const override;

   AudioGraph::ChannelType GetChannelType() const override;

   // overwrite data excluding the sample sequence but including display
   // settings
   void Reinit(const WaveTrack &orig);
private:
   void Init(const WaveTrack &orig);

   Track::Holder Clone() const override;

   friend class WaveTrackFactory;

   wxString MakeClipCopyName(const wxString& originalName) const;
   wxString MakeNewClipName() const;
 public:

   using Holder = std::shared_ptr<WaveTrack>;

   virtual ~WaveTrack();

   double GetOffset() const override;
   void SetOffset(double o) override;

   bool LinkConsistencyFix(bool doFix, bool completeList) override;

   /** @brief Get the time at which the first clip in the track starts
    *
    * @return time in seconds, or zero if there are no clips in the track
    */
   double GetStartTime() const override;

   /** @brief Get the time at which the last clip in the track ends, plus
    * recorded stuff
    *
    * @return time in seconds, or zero if there are no clips in the track.
    */
   double GetEndTime() const override;

   //
   // Identifying the type of track
   //

   //
   // WaveTrack parameters
   //

   double GetRate() const override;
   void SetRate(double newRate);

   // Multiplicative factor.  Only converted to dB for display.
   float GetGain() const;
   void SetGain(float newGain);

   // -1.0 (left) -> 1.0 (right)
   float GetPan() const;
   void SetPan(float newPan);

   //! Takes gain and pan into account
   float GetChannelGain(int channel) const override;

   int GetWaveColorIndex() const { return mWaveColorIndex; };
   void SetWaveColorIndex(int colorIndex);

   sampleCount GetPlaySamplesCount() const;
   //! Returns the total number of samples in all underlying sequences
   //! of all clips (but not counting the cutlines)
   sampleCount GetSequenceSamplesCount() const;

   sampleFormat GetSampleFormat() const override { return mFormat; }

   void ConvertToSampleFormat(sampleFormat format,
      const std::function<void(size_t)> & progressReport = {});

   //
   // High-level editing
   //

   Track::Holder Cut(double t0, double t1) override;

   //! Make another track copying format, rate, color, etc. but containing no
   //! clips
   /*!
    It is important to pass the correct factory (that for the project
    which will own the copy) in the unusual case that a track is copied from
    another project or the clipboard.  For copies within one project, the
    default will do.

    @param keepLink if false, make the new track mono.  But always preserve
    any other track group data.
    */
   Holder EmptyCopy(const SampleBlockFactoryPtr &pFactory = {},
      bool keepLink = true) const;

   // If forClipboard is true,
   // and there is no clip at the end time of the selection, then the result
   // will contain a "placeholder" clip whose only purpose is to make
   // GetEndTime() correct.  This clip is not re-copied when pasting.
   Track::Holder Copy(double t0, double t1, bool forClipboard = true) const override;
   Track::Holder CopyNonconst(double t0, double t1) /* not override */;

   void Clear(double t0, double t1) override;
   void Paste(double t0, const Track *src) override;
   // May assume precondition: t0 <= t1
   void ClearAndPaste(double t0, double t1,
                              const Track *src,
                              bool preserve = true,
                              bool merge = true,
                              const TimeWarper *effectWarper = NULL) /* not override */;

   void Silence(double t0, double t1) override;
   void InsertSilence(double t, double len) override;

   void SplitAt(double t) /* not override */;
   void Split(double t0, double t1) /* not override */;
   // Track::Holder CutAndAddCutLine(double t0, double t1) /* not override */;
   // May assume precondition: t0 <= t1
   void ClearAndAddCutLine(double t0, double t1) /* not override */;

   Track::Holder SplitCut(double t0, double t1) /* not override */;
   // May assume precondition: t0 <= t1
   void SplitDelete(double t0, double t1) /* not override */;
   void Join(double t0, double t1) /* not override */;
   // May assume precondition: t0 <= t1
   void Disjoin(double t0, double t1) /* not override */;

   // May assume precondition: t0 <= t1
   void Trim(double t0, double t1) /* not override */;

   // May assume precondition: t0 <= t1
   void HandleClear(double t0, double t1, bool addCutLines, bool split);

   void SyncLockAdjust(double oldT1, double newT1) override;

   /** @brief Returns true if there are no WaveClips in the specified region
    *
    * @return true if no clips in the track overlap the specified time range,
    * false otherwise.
    */
   bool IsEmpty(double t0, double t1) const;

   /*!
    If there is an existing WaveClip in the WaveTrack then the data are
    appended to that clip. If there are no WaveClips in the track, then a new
    one is created.
    @return true if at least one complete block was created
    */
   bool Append(constSamplePtr buffer, sampleFormat format,
      size_t len, unsigned int stride = 1,
      sampleFormat effectiveFormat = widestSampleFormat) override;

   void Flush() override;

   //! @name PlayableSequence implementation
   //! @{
   bool IsLeader() const override;
   bool GetMute() const override;
   bool GetSolo() const override;
   //! @}

   /*!
    * @pre `iChannel + nBuffers <= NChannels()`
    * @return nBuffers `ChannelSampleView`s, one per channel.
    */
   std::vector<ChannelSampleView> GetSampleView(
      size_t iChannel, size_t nBuffers, sampleCount start, size_t len,
      bool backwards) const;

   ///
   /// MM: Now that each wave track can contain multiple clips, we don't
   /// have a continuous space of samples anymore, but we simulate it,
   /// because there are a lot of places (e.g. effects) using this interface.
   /// This interface makes much sense for modifying samples, but note that
   /// it is not time-accurate, because the "offset" is a double value and
   /// therefore can lie inbetween samples. But as long as you use the
   /// same value for "start" in both calls to "Set" and "Get" it is
   /// guaranteed that the same samples are affected.
   ///

   bool Get(
      size_t iChannel, size_t nBuffers, samplePtr buffers[],
      sampleFormat format, sampleCount start, size_t len, bool backwards,
      fillFormat fill = fillZero, bool mayThrow = true,
      // Report how many samples were copied from within clips, rather than
      // filled according to fillFormat; but these were not necessarily one
      // contiguous range.
      sampleCount* pNumWithinClips = nullptr) const override;
   /*!
    Set samples in the unique channel
    TODO wide wave tracks -- overloads to set one or all channels
    */
   void Set(constSamplePtr buffer, sampleFormat format,
      sampleCount start, size_t len,
      sampleFormat effectiveFormat = widestSampleFormat /*!<
         Make the effective format of the data at least the minumum of this
         value and `format`.  (Maybe wider, if merging with preexistent data.)
         If the data are later narrowed from stored format, but not narrower
         than the effective, then no dithering will occur.
      */
   );

   sampleFormat WidestEffectiveFormat() const override;

   bool HasTrivialEnvelope() const override;

   void GetEnvelopeValues(
      double* buffer, size_t bufferLen, double t0,
      bool backwards) const override;

   // Get min and max from the unique channel
   /*!
    @pre `t0 <= t1`
    TODO wide wave tracks -- require a channel number
    */
   std::pair<float, float> GetMinMax(
      double t0, double t1, bool mayThrow = true) const;

   // Get RMS from the unique channel
   /*!
    @pre `t0 <= t1`
    TODO wide wave tracks -- require a channel number
    */
   float GetRMS(double t0, double t1, bool mayThrow = true) const;

   //
   // MM: We now have more than one sequence and envelope per track, so
   // instead of GetEnvelope() we have the following function which gives the
   // envelope that contains the given time.
   //
   Envelope* GetEnvelopeAtTime(double time);

   WaveClip* GetClipAtTime(double time);

   //
   // Getting information about the track's internal block sizes
   // and alignment for efficiency
   //

   // These return a nonnegative number of samples meant to size a memory buffer
   size_t GetBestBlockSize(sampleCount t) const;
   size_t GetMaxBlockSize() const;
   size_t GetIdealBlockSize();

   //
   // XMLTagHandler callback methods for loading and saving
   //

   bool HandleXMLTag(const std::string_view& tag, const AttributesList& attrs) override;
   void HandleXMLEndTag(const std::string_view& tag) override;
   XMLTagHandler *HandleXMLChild(const std::string_view& tag) override;
   void WriteXML(XMLWriter &xmlFile) const override;

   // Returns true if an error occurred while reading from XML
   std::optional<TranslatableString> GetErrorOpening() const override;

   //
   // Lock and unlock the track: you must lock the track before
   // doing a copy and paste between projects.
   //

   //! Should be called upon project close.  Not balanced by unlocking calls.
   /*!
    @pre `IsLeader()`
    @excsafety{No-fail}
    */
   bool CloseLock() noexcept;

   //! Get access to the (visible) clips in the tracks, in unspecified order
   //! (not necessarily sequenced in time).
   /*!
    @post all pointers are non-null
    */
   WaveClipHolders &GetClips() { return mClips; }
   /*!
    @copydoc GetClips
    */
   const WaveClipConstHolders &GetClips() const
      { return reinterpret_cast< const WaveClipConstHolders& >( mClips ); }

   /**
    * @brief Get access to the (visible) clips in the tracks, in unspecified
    * order.
    * @pre `IsLeader()`
    */
   ClipConstHolders GetClipInterfaces() const;

   // Get mutative access to all clips (in some unspecified sequence),
   // including those hidden in cutlines.
   class AllClipsIterator
      : public ValueIterator< WaveClip * >
   {
   public:
      // Constructs an "end" iterator
      AllClipsIterator () {}

      // Construct a "begin" iterator
      explicit AllClipsIterator( WaveTrack &track )
      {
         push( track.mClips );
      }

      WaveClip *operator * () const
      {
         if (mStack.empty())
            return nullptr;
         else
            return mStack.back().first->get();
      }

      AllClipsIterator &operator ++ ();

      // Define == well enough to serve for loop termination test
      friend bool operator == (
         const AllClipsIterator &a, const AllClipsIterator &b)
      { return a.mStack.empty() == b.mStack.empty(); }

      friend bool operator != (
         const AllClipsIterator &a, const AllClipsIterator &b)
      { return !( a == b ); }

   private:

      void push( WaveClipHolders &clips );

      using Iterator = WaveClipHolders::iterator;
      using Pair = std::pair< Iterator, Iterator >;
      using Stack = std::vector< Pair >;

      Stack mStack;
   };

   // Get const access to all clips (in some unspecified sequence),
   // including those hidden in cutlines.
   class AllClipsConstIterator
      : public ValueIterator< const WaveClip * >
   {
   public:
      // Constructs an "end" iterator
      AllClipsConstIterator () {}

      // Construct a "begin" iterator
      explicit AllClipsConstIterator( const WaveTrack &track )
         : mIter{ const_cast< WaveTrack& >( track ) }
      {}

      const WaveClip *operator * () const
      { return *mIter; }

      AllClipsConstIterator &operator ++ ()
      { ++mIter; return *this; }

      // Define == well enough to serve for loop termination test
      friend bool operator == (
         const AllClipsConstIterator &a, const AllClipsConstIterator &b)
      { return a.mIter == b.mIter; }

      friend bool operator != (
         const AllClipsConstIterator &a, const AllClipsConstIterator &b)
      { return !( a == b ); }

   private:
      AllClipsIterator mIter;
   };

   IteratorRange< AllClipsIterator > GetAllClips()
   {
      return { AllClipsIterator{ *this }, AllClipsIterator{ } };
   }

   IteratorRange< AllClipsConstIterator > GetAllClips() const
   {
      return { AllClipsConstIterator{ *this }, AllClipsConstIterator{ } };
   }

   //! Create new clip and add it to this track.
   /*!
    Returns a pointer to the newly created clip. Optionally initial offset and
    clip name may be provided

    @post result: `result->GetWidth() == GetWidth()`
    */
   WaveClip* CreateClip(double offset = .0, const wxString& name = wxEmptyString);

   /** @brief Get access to the most recently added clip, or create a clip,
   *  if there is not already one.  THIS IS NOT NECESSARILY RIGHTMOST.
   *
   *  @return a pointer to the most recently added WaveClip
   */
   WaveClip* NewestOrNewClip();

   /** @brief Get access to the last (rightmost) clip, or create a clip,
   *  if there is not already one.
   *
   *  @return a pointer to a WaveClip at the end of the track
   */
   WaveClip* RightmostOrNewClip();

   // Get the linear index of a given clip (-1 if the clip is not found)
   int GetClipIndex(const WaveClip* clip) const;

   //! Get the nth clip in this WaveTrack (will return nullptr if not found).
   /*!
    Use this only in special cases (like getting the linked clip), because
    it is much slower than GetClipIterator().
    */
   WaveClip *GetClipByIndex(int index);
   /*!
    @copydoc GetClipByIndex
    */
   const WaveClip* GetClipByIndex(int index) const;

   // Get number of clips in this WaveTrack
   int GetNumClips() const;

   // Add all wave clips to the given array 'clips' and sort the array by
   // clip start time. The array is emptied prior to adding the clips.
   WaveClipPointers SortedClipArray();
   WaveClipConstPointers SortedClipArray() const;

   //! Decide whether the clips could be offset (and inserted) together without overlapping other clips
   /*!
   @return true if possible to offset by `(allowedAmount ? *allowedAmount : amount)`
    */
   bool CanOffsetClips(
      const std::vector<WaveClip*> &clips, //!< not necessarily in this track
      double amount, //!< signed
      double *allowedAmount = nullptr /*!<
         [out] if null, test exact amount only; else, largest (in magnitude) possible offset with same sign */
   );

   // Before moving a clip into a track (or inserting a clip), use this
   // function to see if the times are valid (i.e. don't overlap with
   // existing clips).
   bool CanInsertClip(WaveClip* clip, double &slideBy, double &tolerance) const;

   // Remove the clip from the track and return a SMART pointer to it.
   // You assume responsibility for its memory!
   std::shared_ptr<WaveClip> RemoveAndReturnClip(WaveClip* clip);

   //! Append a clip to the track; to succeed, must have the same block factory
   //! as this track, and `this->GetWidth() == clip->GetWidth()`; return success
   /*!
    @pre `clip != nullptr`
    @pre `this->GetWidth() == clip->GetWidth()`
    */
   bool AddClip(const std::shared_ptr<WaveClip> &clip);

   // Merge two clips, that is append data from clip2 to clip1,
   // then remove clip2 from track.
   // clipidx1 and clipidx2 are indices into the clip list.
   void MergeClips(int clipidx1, int clipidx2);

   // Expand cut line (that is, re-insert audio, then DELETE audio saved in cut line)
   void ExpandCutLine(double cutLinePosition, double* cutlineStart = NULL, double* cutlineEnd = NULL);

   // Remove cut line, without expanding the audio in it
   bool RemoveCutLine(double cutLinePosition);

   // This track has been merged into a stereo track.  Copy shared parameters
   // from the NEW partner.
   void Merge(const Track &orig);

   // Resample track (i.e. all clips in the track)
   void Resample(int rate, BasicUI::ProgressDialog *progress = NULL);

   const TypeInfo &GetTypeInfo() const override;
   static const TypeInfo &ClassTypeInfo();

   class WAVE_TRACK_API Interval final : public WideChannelGroupInterval {
   public:
      /*!
       @pre `pClip != nullptr`
       */
      Interval(const ChannelGroup &group,
         const std::shared_ptr<WaveClip> &pClip,
         const std::shared_ptr<WaveClip> &pClip1);

      ~Interval() override;

      std::shared_ptr<const WaveClip> GetClip(size_t iChannel) const
      { return iChannel == 0 ? mpClip : mpClip1; }
      const std::shared_ptr<WaveClip> &GetClip(size_t iChannel)
      { return iChannel == 0 ? mpClip : mpClip1; }
   private:
      std::shared_ptr<ChannelInterval> DoGetChannel(size_t iChannel) override;
      const std::shared_ptr<WaveClip> mpClip;
      //! TODO wide wave tracks: eliminate this
      const std::shared_ptr<WaveClip> mpClip1;
   };

   Track::Holder PasteInto( AudacityProject & ) const override;

   //! Returns nullptr if clip with such name was not found
   const WaveClip* FindClipByName(const wxString& name) const;

   size_t NIntervals() const override;

protected:
   std::shared_ptr<WideChannelGroupInterval> DoGetInterval(size_t iInterval)
      override;
   std::shared_ptr<::Channel> DoGetChannel(size_t iChannel) override;

   ChannelGroup &DoGetChannelGroup() const override;

   //
   // Protected variables
   //

   /*!
    * Do not call `mClips.push_back` directly. Use `InsertClip` instead.
    * @invariant all are non-null and match `this->GetWidth()`
    */
   WaveClipHolders mClips;

   sampleFormat  mFormat;
   mutable int   mLegacyRate{ 0 }; //!< used only during deserialization
   int           mWaveColorIndex;

private:
   void SetClipRates(double newRate);
   void DoOnProjectTempoChange(
      const std::optional<double>& oldTempo, double newTempo) override;

   bool GetOne(
      samplePtr buffer, sampleFormat format, sampleCount start, size_t len,
      bool backwards, fillFormat fill, bool mayThrow,
      sampleCount* pNumWithinClips) const;
   ChannelSampleView
   GetOneSampleView(sampleCount start, size_t len, bool backwards) const;

   void DoSetPan(float value);
   void DoSetGain(float value);

   void PasteWaveTrack(double t0, const WaveTrack* other);

   //! Whether all clips have a common rate
   bool RateConsistencyCheck() const;

   //! Sets project tempo on clip upon push. Use this instead of
   //! `mClips.push_back`.
   void InsertClip(WaveClipHolder clip);

   SampleBlockFactoryPtr mpFactory;

   wxCriticalSection mFlushCriticalSection;
   wxCriticalSection mAppendCriticalSection;
   double mLegacyProjectFileOffset;
};

ENUMERATE_TRACK_TYPE(WaveTrack);

#include <unordered_set>
class SampleBlock;
using SampleBlockID = long long;
using SampleBlockIDSet = std::unordered_set<SampleBlockID>;
class TrackList;
using BlockVisitor = std::function< void(SampleBlock&) >;
using BlockInspector = std::function< void(const SampleBlock&) >;

// Function to visit all sample blocks from a list of tracks.
// If a set is supplied, then only visit once each unique block ID not already
// in that set, and accumulate those into the set as a side-effect.
// The visitor function may be null.
void VisitBlocks(TrackList &tracks, BlockVisitor visitor,
   SampleBlockIDSet *pIDs = nullptr);

// Non-mutating version of the above
WAVE_TRACK_API void InspectBlocks(const TrackList &tracks,
   BlockInspector inspector, SampleBlockIDSet *pIDs = nullptr);

class ProjectRate;

class WAVE_TRACK_API WaveTrackFactory final
   : public ClientData::Base
{
 public:
   static WaveTrackFactory &Get( AudacityProject &project );
   static const WaveTrackFactory &Get( const AudacityProject &project );
   static WaveTrackFactory &Reset( AudacityProject &project );
   static void Destroy( AudacityProject &project );

   WaveTrackFactory(
      const ProjectRate& rate,
      const SampleBlockFactoryPtr &pFactory)
       : mRate{ rate }
       , mpFactory(pFactory)
   {
   }
   WaveTrackFactory( const WaveTrackFactory & ) PROHIBITED;
   WaveTrackFactory &operator=( const WaveTrackFactory & ) PROHIBITED;

   const SampleBlockFactoryPtr &GetSampleBlockFactory() const
   { return mpFactory; }

   /**
    * \brief Creates an unnamed empty WaveTrack with default sample format and default rate
    * \return Orphaned WaveTrack
    */
   std::shared_ptr<WaveTrack> Create();

   /**
    * \brief Creates an unnamed empty WaveTrack with custom sample format and custom rate
    * \param format Desired sample format
    * \param rate Desired sample rate
    * \return Orphaned WaveTrack
    */
   std::shared_ptr<WaveTrack> Create(sampleFormat format, double rate);

 private:
   const ProjectRate &mRate;
   SampleBlockFactoryPtr mpFactory;
};

extern WAVE_TRACK_API BoolSetting
     EditClipsCanMove
;

extern WAVE_TRACK_API StringSetting AudioTrackNameSetting;

WAVE_TRACK_API bool GetEditClipsCanMove();

// Generate a registry for serialized data
#include "XMLMethodRegistry.h"
using WaveTrackIORegistry = XMLMethodRegistry<WaveTrack>;
DECLARE_XML_METHOD_REGISTRY( WAVE_TRACK_API, WaveTrackIORegistry );

#endif // __AUDACITY_WAVETRACK__
