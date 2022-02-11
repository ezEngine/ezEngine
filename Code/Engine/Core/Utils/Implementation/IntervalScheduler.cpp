#include <Core/CorePCH.h>

#include <Core/Utils/IntervalScheduler.h>
#include <Foundation/SimdMath/SimdRandom.h>

EZ_ALWAYS_INLINE float GetRandomZeroToOne(int pos, ezUInt32& seed)
{
  return ezSimdRandom::FloatZeroToOne(ezSimdVec4i(pos), ezSimdVec4u(seed++)).x();
}

constexpr ezTime s_JitterRange = ezTime::Microseconds(10);
constexpr ezTime s_HalfJitterRange = s_JitterRange * 0.5;

EZ_ALWAYS_INLINE ezTime GetRandomTimeJitter(int pos, ezUInt32& seed)
{
  const float x = ezSimdRandom::FloatZeroToOne(ezSimdVec4i(pos), ezSimdVec4u(seed++)).x();
  return s_JitterRange * (x * 2.0f - 1.0f);
}

ezIntervalSchedulerBase::ezIntervalSchedulerBase(ezTime minInterval, ezTime maxInterval)
  : m_minInterval(minInterval)
  , m_maxInterval(maxInterval)
{
  m_InvIntervalRange = 1.0 / (m_maxInterval - m_minInterval).GetSeconds();

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
  const double x = ezMath::Max((value - m_minInterval).GetSeconds() * m_InvIntervalRange, 0.0);
  const double i = ezMath::Sqrt(x) * maxSlotIndex;
  return ezMath::Min(static_cast<ezUInt32>(i), maxSlotIndex);
}

ezTime ezIntervalSchedulerBase::GetHistogramSlotValue(ezUInt32 uiIndex)
{
  constexpr double norm = 1.0 / (HistogramSize - 1.0);
  const double x = uiIndex * norm;
  return (x * x) * (m_maxInterval - m_minInterval) + m_minInterval;
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
