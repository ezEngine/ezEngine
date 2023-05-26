#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

struct EZ_CORE_DLL ezUpdateRate
{
  using StorageType = ezUInt8;

  enum Enum
  {
    EveryFrame,
    Max30fps,
    Max20fps,
    Max10fps,
    Max5fps,
    Max2fps,
    Max1fps,

    Default = Max30fps
  };

  static ezTime GetInterval(Enum updateRate);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezUpdateRate);

//////////////////////////////////////////////////////////////////////////

/// \brief Helper class to schedule work in intervals typically larger than the duration of one frame
///
/// Tries to maintain an even workload per frame and also keep the given interval for a work as best as possible.
/// A typical use case would be e.g. component update functions that don't need to be called every frame.
class EZ_CORE_DLL ezIntervalSchedulerBase
{
protected:
  ezIntervalSchedulerBase(ezTime minInterval, ezTime maxInterval);
  ~ezIntervalSchedulerBase();

  ezUInt32 GetHistogramIndex(ezTime value);
  ezTime GetHistogramSlotValue(ezUInt32 uiIndex);

  static float GetRandomZeroToOne(int pos, ezUInt32& seed);
  static ezTime GetRandomTimeJitter(int pos, ezUInt32& seed);

  ezTime m_MinInterval;
  ezTime m_MaxInterval;
  double m_fInvIntervalRange;

  ezTime m_CurrentTime;
  double m_fNumWorkToSchedule = 0.0;

  ezUInt32 m_uiSeed = 0;

  static constexpr ezUInt32 HistogramSize = 32;
  ezUInt32 m_Histogram[HistogramSize] = {};
  ezTime m_HistogramSlotValues[HistogramSize] = {};
};

//////////////////////////////////////////////////////////////////////////

/// \brief \see ezIntervalSchedulerBase
template <typename T>
class ezIntervalScheduler : public ezIntervalSchedulerBase
{
  using SUPER = ezIntervalSchedulerBase;

public:
  EZ_ALWAYS_INLINE ezIntervalScheduler(ezTime minInterval = ezTime::Milliseconds(1), ezTime maxInterval = ezTime::Seconds(1))
    : SUPER(minInterval, maxInterval)
  {
  }

  void AddOrUpdateWork(const T& work, ezTime interval);
  void RemoveWork(const T& work);

  ezTime GetInterval(const T& work) const;

  // reference to the work that should be run and time passed since this work has been last run.
  using RunWorkCallback = ezDelegate<void(const T&, ezTime)>;

  /// \brief Advances the scheduler by deltaTime and triggers runWorkCallback for each work that should be run during this update step.
  /// Since it is not possible to maintain the exact interval all the time the actual delta time for the work is also passed to runWorkCallback.
  void Update(ezTime deltaTime, RunWorkCallback runWorkCallback);

private:
  struct Data
  {
    T m_Work;
    ezTime m_Interval;
    ezTime m_DueTime;
    ezTime m_LastScheduledTime;

    bool IsValid() const;
    void MarkAsInvalid();
  };

  using DataMap = ezMap<ezTime, Data>;
  DataMap m_Data;
  ezHashTable<T, typename DataMap::Iterator> m_WorkIdToData;

  typename DataMap::Iterator InsertData(Data& data);
  ezDynamicArray<typename DataMap::Iterator> m_ScheduledWork;
};

#include <Core/Utils/Implementation/IntervalScheduler_inl.h>
