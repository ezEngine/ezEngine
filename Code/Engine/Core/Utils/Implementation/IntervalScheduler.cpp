#include <Core/CorePCH.h>

#include <Core/Utils/IntervalScheduler.h>
#include <Foundation/SimdMath/SimdRandom.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezUpdateRate, 1)
  EZ_ENUM_CONSTANTS(ezUpdateRate::EveryFrame)
  EZ_ENUM_CONSTANTS(ezUpdateRate::Max30fps, ezUpdateRate::Max20fps, ezUpdateRate::Max10fps)
  EZ_ENUM_CONSTANTS(ezUpdateRate::Max5fps, ezUpdateRate::Max2fps, ezUpdateRate::Max1fps)
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

static ezTime s_Intervals[] = {
  ezTime::Zero(),              // EveryFrame
  ezTime::Seconds(1.0 / 30.0), // Max30fps
  ezTime::Seconds(1.0 / 20.0), // Max20fps
  ezTime::Seconds(1.0 / 10.0), // Max10fps
  ezTime::Seconds(1.0 / 5.0),  // Max5fps
  ezTime::Seconds(1.0 / 2.0),  // Max2fps
  ezTime::Seconds(1.0 / 1.0),  // Max1fps
};

static_assert(EZ_ARRAY_SIZE(s_Intervals) == ezUpdateRate::Max1fps + 1);

ezTime ezUpdateRate::GetInterval(Enum updateRate)
{
  return s_Intervals[updateRate];
}

//////////////////////////////////////////////////////////////////////////

EZ_ALWAYS_INLINE float GetRandomZeroToOne(int pos, ezUInt32& seed)
{
  return ezSimdRandom::FloatZeroToOne(ezSimdVec4i(pos), ezSimdVec4u(seed++)).x();
}

constexpr ezTime s_JitterRange = ezTime::Microseconds(10);

EZ_ALWAYS_INLINE ezTime GetRandomTimeJitter(int pos, ezUInt32& seed)
{
  const float x = ezSimdRandom::FloatZeroToOne(ezSimdVec4i(pos), ezSimdVec4u(seed++)).x();
  return s_JitterRange * (x * 2.0f - 1.0f);
}

ezIntervalSchedulerBase::ezIntervalSchedulerBase(ezTime minInterval, ezTime maxInterval)
  : m_MinInterval(minInterval)
  , m_MaxInterval(maxInterval)
{
  m_fInvIntervalRange = 1.0 / (m_MaxInterval - m_MinInterval).GetSeconds();

  for (ezUInt32 i = 0; i < HistogramSize; ++i)
  {
    m_HistogramSlotValues[i] = GetHistogramSlotValue(i);
  }
}

ezIntervalSchedulerBase::~ezIntervalSchedulerBase() = default;

void ezIntervalSchedulerBase::AddOrUpdateWork(ezUInt64 workId, ezTime interval)
{
  DataMap::Iterator it;
  if (m_WorkIdToData.TryGetValue(workId, it))
  {
    ezTime oldInterval = it.Value().m_Interval;
    if (interval == oldInterval)
      return;

    m_Data.Remove(it);

    const ezUInt32 uiHistogramIndex = GetHistogramIndex(oldInterval);
    m_Histogram[uiHistogramIndex]--;
  }

  Data data;
  data.m_WorkId = workId;
  data.m_Interval = ezMath::Max(interval, ezTime::Zero());
  data.m_DueTime = m_CurrentTime + GetRandomZeroToOne(m_Data.GetCount(), m_uiSeed) * data.m_Interval;
  data.m_LastScheduledTime = m_CurrentTime;

  m_WorkIdToData[workId] = InsertData(data);

  const ezUInt32 uiHistogramIndex = GetHistogramIndex(data.m_Interval);
  m_Histogram[uiHistogramIndex]++;
}

void ezIntervalSchedulerBase::RemoveWork(ezUInt64 workId)
{
  DataMap::Iterator it;
  EZ_VERIFY(m_WorkIdToData.Remove(workId, &it), "Entry not found");

  ezTime oldInterval = it.Value().m_Interval;
  m_Data.Remove(it);

  const ezUInt32 uiHistogramIndex = GetHistogramIndex(oldInterval);
  m_Histogram[uiHistogramIndex]--;
}

ezTime ezIntervalSchedulerBase::GetInterval(ezUInt64 workId) const
{
  DataMap::Iterator it;
  EZ_VERIFY(m_WorkIdToData.TryGetValue(workId, it), "Entry not found");
  return it.Value().m_Interval;
}

void ezIntervalSchedulerBase::Update(ezTime deltaTime, ezDelegate<void(ezUInt64, ezTime)> runWorkCallback)
{
  if (deltaTime <= ezTime::Zero())
    return;

  if (m_Data.IsEmpty())
  {
    m_fNumWorkToSchedule = 0.0;
  }
  else
  {
    double fNumWork = 0;
    for (ezUInt32 i = 0; i < HistogramSize; ++i)
    {
      fNumWork += (1.0 / ezMath::Max(m_HistogramSlotValues[i], deltaTime).GetSeconds()) * m_Histogram[i];
    }
    fNumWork *= deltaTime.GetSeconds();

    if (m_fNumWorkToSchedule == 0.0)
    {
      m_fNumWorkToSchedule = fNumWork;
    }
    else
    {
      // running average of num work per update to prevent huge spikes
      m_fNumWorkToSchedule = ezMath::Lerp<double>(m_fNumWorkToSchedule, fNumWork, 0.05);
    }

    const float fRemainder = static_cast<float>(ezMath::Fraction(m_fNumWorkToSchedule));
    const int pos = static_cast<int>(m_CurrentTime.GetNanoseconds());
    const ezUInt32 extra = GetRandomZeroToOne(pos, m_uiSeed) < fRemainder ? 1 : 0;
    const ezUInt32 uiScheduleCount = ezMath::Min(static_cast<ezUInt32>(m_fNumWorkToSchedule) + extra, m_Data.GetCount());

    // schedule work
    {
      auto it = m_Data.GetIterator();
      for (ezUInt32 i = 0; i < uiScheduleCount; ++i, ++it)
      {
        auto& data = it.Value();
        runWorkCallback(data.m_WorkId, m_CurrentTime - data.m_LastScheduledTime);

        // add a little bit of random jitter so we don't end up with perfect timings that might collide with other work
        data.m_DueTime = m_CurrentTime + ezMath::Max(data.m_Interval, deltaTime) + GetRandomTimeJitter(i, m_uiSeed);
        data.m_LastScheduledTime = m_CurrentTime;

        m_ScheduledWork.PushBack(it);
      }
    }

    // re-sort
    for (auto& it : m_ScheduledWork)
    {
      Data data = it.Value();
      m_WorkIdToData[data.m_WorkId] = InsertData(data);
      m_Data.Remove(it);
    }
    m_ScheduledWork.Clear();
  }

  m_CurrentTime += deltaTime;
}

ezUInt32 ezIntervalSchedulerBase::GetHistogramIndex(ezTime value)
{
  constexpr ezUInt32 maxSlotIndex = HistogramSize - 1;
  const double x = ezMath::Max((value - m_MinInterval).GetSeconds() * m_fInvIntervalRange, 0.0);
  const double i = ezMath::Sqrt(x) * maxSlotIndex;
  return ezMath::Min(static_cast<ezUInt32>(i), maxSlotIndex);
}

ezTime ezIntervalSchedulerBase::GetHistogramSlotValue(ezUInt32 uiIndex)
{
  constexpr double norm = 1.0 / (HistogramSize - 1.0);
  const double x = uiIndex * norm;
  return (x * x) * (m_MaxInterval - m_MinInterval) + m_MinInterval;
}

ezIntervalSchedulerBase::DataMap::Iterator ezIntervalSchedulerBase::InsertData(Data& data)
{
  // make sure that we have a unique due time since the map can't store multiple keys with the same value
  int pos = 0;
  while (m_Data.Contains(data.m_DueTime))
  {
    data.m_DueTime += GetRandomTimeJitter(pos++, m_uiSeed);
  }

  return m_Data.Insert(data.m_DueTime, data);
}


EZ_STATICLINK_FILE(Core, Core_Utils_Implementation_IntervalScheduler);

