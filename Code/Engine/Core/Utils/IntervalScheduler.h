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

  void AddOrUpdateWork(ezUInt64 workId, ezTime interval);
  void RemoveWork(ezUInt64 workId);

  ezTime GetInterval(ezUInt64 workId) const;

  /// \brief Advances the scheduler by deltaTime and triggers runWorkCallback for each work that should be run during this update step.
  /// Since it is not possible to maintain the exact interval all the time the actual delta time for the work is also passed to runWorkCallback.
  void Update(ezTime deltaTime, ezDelegate<void(ezUInt64, ezTime)> runWorkCallback);

private:
  ezUInt32 GetHistogramIndex(ezTime value);
  ezTime GetHistogramSlotValue(ezUInt32 uiIndex);

  ezTime m_minInterval;
  ezTime m_maxInterval;
  double m_InvIntervalRange;

  ezTime m_CurrentTime;
  double m_fNumWorkToSchedule = 0.0;

  ezUInt32 m_uiSeed = 0;

  static constexpr ezUInt32 HistogramSize = 32;
  ezUInt32 m_Histogram[HistogramSize] = {};
  ezTime m_HistogramSlotValues[HistogramSize] = {};

  struct Data
  {
    ezUInt64 m_WorkId = 0;
    ezTime m_Interval;
    ezTime m_DueTime;
    ezTime m_LastScheduledTime;
  };

  using DataMap = ezMap<ezTime, Data>;
  DataMap m_Data;
  ezHashTable<ezUInt64, DataMap::Iterator> m_WorkIdToData;

  DataMap::Iterator InsertData(Data& data);
  ezDynamicArray<DataMap::Iterator> m_ScheduledWork;
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
    static_assert(sizeof(T) <= sizeof(ezUInt64), "sizeof T must be smaller or equal to 8 bytes");
  }

  EZ_ALWAYS_INLINE void AddOrUpdateWork(const T& work, ezTime interval)
  {
    SUPER::AddOrUpdateWork(*reinterpret_cast<const ezUInt64*>(&work), interval);
  }

  EZ_ALWAYS_INLINE void RemoveWork(const T& work)
  {
    SUPER::RemoveWork(*reinterpret_cast<const ezUInt64*>(&work));
  }

  EZ_ALWAYS_INLINE ezTime GetInterval(const T& work) const
  {
    return SUPER::GetInterval(*reinterpret_cast<const ezUInt64*>(&work));
  }

  // reference to the work that should be run and time passed since this work has been last run.
  using RunWorkCallback = ezDelegate<void(const T&, ezTime)>;

  EZ_ALWAYS_INLINE void Update(ezTime deltaTime, RunWorkCallback runWorkCallback)
  {
    SUPER::Update(deltaTime, [&](ezUInt64 workId, ezTime deltaTime)
      {
        if (runWorkCallback.IsValid())
        {
          runWorkCallback(*reinterpret_cast<T*>(&workId), deltaTime);
        }
      });
  }
};
