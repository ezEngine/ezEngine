
#include <Foundation/SimdMath/SimdRandom.h>

EZ_ALWAYS_INLINE ezUInt32 ezIntervalSchedulerBase::GetHistogramIndex(ezTime value)
{
  if (value.IsZero())
    return 0;

  constexpr ezUInt32 maxSlotIndex = HistogramSize - 1;
  const double x = ezMath::Max((value - m_MinInterval).GetSeconds() * m_fInvIntervalRange, 0.0);
  const double i = ezMath::Sqrt(x) * (maxSlotIndex - 1) + 1;
  return ezMath::Min(static_cast<ezUInt32>(i), maxSlotIndex);
}

EZ_ALWAYS_INLINE ezTime ezIntervalSchedulerBase::GetHistogramSlotValue(ezUInt32 uiIndex)
{
  if (uiIndex == 0)
    return ezTime::MakeZero();

  constexpr double norm = 1.0 / (HistogramSize - 2.0);
  const double x = (uiIndex - 1) * norm;
  return (x * x) * (m_MaxInterval - m_MinInterval) + m_MinInterval;
}

// static
EZ_ALWAYS_INLINE float ezIntervalSchedulerBase::GetRandomZeroToOne(int pos, ezUInt32& seed)
{
  return ezSimdRandom::FloatZeroToOne(ezSimdVec4i(pos), ezSimdVec4u(seed++)).x();
}

constexpr ezTime s_JitterRange = ezTime::MakeFromMicroseconds(10);

// static
EZ_ALWAYS_INLINE ezTime ezIntervalSchedulerBase::GetRandomTimeJitter(int pos, ezUInt32& seed)
{
  const float x = ezSimdRandom::FloatZeroToOne(ezSimdVec4i(pos), ezSimdVec4u(seed++)).x();
  return s_JitterRange * (x * 2.0f - 1.0f);
}

//////////////////////////////////////////////////////////////////////////

template <typename T>
bool ezIntervalScheduler<T>::Data::IsValid() const
{
  return m_Interval.IsZeroOrPositive();
}

template <typename T>
void ezIntervalScheduler<T>::Data::MarkAsInvalid()
{
  m_Interval = ezTime::MakeFromSeconds(-1);
}

//////////////////////////////////////////////////////////////////////////

template <typename T>
void ezIntervalScheduler<T>::AddOrUpdateWork(const T& work, ezTime interval)
{
  typename DataMap::Iterator it;
  if (m_WorkIdToData.TryGetValue(work, it))
  {
    auto& data = it.Value();
    ezTime oldInterval = data.m_Interval;
    if (interval == oldInterval)
      return;

    data.MarkAsInvalid();

    const ezUInt32 uiHistogramIndex = GetHistogramIndex(oldInterval);
    m_Histogram[uiHistogramIndex]--;
  }

  Data data;
  data.m_Work = work;
  data.m_Interval = ezMath::Max(interval, ezTime::MakeZero());
  data.m_DueTime = m_CurrentTime + GetRandomZeroToOne(m_Data.GetCount(), m_uiSeed) * data.m_Interval;
  data.m_LastScheduledTime = m_CurrentTime;

  m_WorkIdToData[work] = InsertData(data);

  const ezUInt32 uiHistogramIndex = GetHistogramIndex(data.m_Interval);
  m_Histogram[uiHistogramIndex]++;
}

template <typename T>
void ezIntervalScheduler<T>::RemoveWork(const T& work)
{
  typename DataMap::Iterator it;
  if (m_WorkIdToData.Remove(work, &it))
  {
    auto& data = it.Value();
    ezTime oldInterval = data.m_Interval;
    data.MarkAsInvalid();

    const ezUInt32 uiHistogramIndex = GetHistogramIndex(oldInterval);
    m_Histogram[uiHistogramIndex]--;
  }
}

template <typename T>
ezTime ezIntervalScheduler<T>::GetInterval(const T& work) const
{
  typename DataMap::Iterator it;
  EZ_VERIFY(m_WorkIdToData.TryGetValue(work, it), "Entry not found");
  return it.Value().m_Interval;
}

template <typename T>
void ezIntervalScheduler<T>::Update(ezTime deltaTime, RunWorkCallback runWorkCallback)
{
  if (deltaTime.IsZeroOrNegative())
    return;

  m_CurrentTime += deltaTime;

  if (m_Data.IsEmpty())
    return;

  {
    double fNumWork = 0;
    for (ezUInt32 i = 1; i < HistogramSize; ++i)
    {
      fNumWork += (1.0 / ezMath::Max(m_HistogramSlotValues[i], deltaTime).GetSeconds()) * m_Histogram[i];
    }
    fNumWork *= deltaTime.GetSeconds();

    const float fRemainder = static_cast<float>(ezMath::Fraction(fNumWork));
    const int pos = static_cast<int>(m_CurrentTime.GetNanoseconds());
    const ezUInt32 extra = GetRandomZeroToOne(pos, m_uiSeed) < fRemainder ? 1 : 0;
    const ezUInt32 uiScheduleCount = ezMath::Min(static_cast<ezUInt32>(fNumWork) + extra + m_Histogram[0], m_Data.GetCount());

    // schedule work
    {
      auto RunWork = [&](typename DataMap::Iterator it, ezUInt32 uiIndex)
      {
        auto& data = it.Value();
        if (data.IsValid())
        {
          if (runWorkCallback.IsValid())
          {
            runWorkCallback(data.m_Work, m_CurrentTime - data.m_LastScheduledTime);
          }

          // add a little bit of random jitter so we don't end up with perfect timings that might collide with other work
          data.m_DueTime = m_CurrentTime + data.m_Interval + GetRandomTimeJitter(uiIndex, m_uiSeed);
          data.m_LastScheduledTime = m_CurrentTime;
        }

        m_ScheduledWork.PushBack(it);
      };

      auto it = m_Data.GetIterator();
      for (ezUInt32 i = 0; i < uiScheduleCount; ++i, ++it)
      {
        RunWork(it, i);
      }

      // check if the next works have a zero interval if so execute them as well to fulfill the every frame guarantee
      ezUInt32 uiNumExtras = 0;
      while (it.IsValid())
      {
        auto& data = it.Value();
        if (data.m_Interval.IsPositive())
          break;

        RunWork(it, uiNumExtras);

        ++uiNumExtras;
        ++it;
      }
    }

    // re-sort
    for (auto& it : m_ScheduledWork)
    {
      if (it.Value().IsValid())
      {
        // make a copy of data and re-insert at new due time
        Data data = it.Value();
        m_WorkIdToData[data.m_Work] = InsertData(data);
      }

      m_Data.Remove(it);
    }
    m_ScheduledWork.Clear();
  }
}

template <typename T>
void ezIntervalScheduler<T>::Clear()
{
  m_CurrentTime = ezTime::MakeZero();
  m_uiSeed = 0;
  ezMemoryUtils::ZeroFill(m_Histogram, HistogramSize);

  m_Data.Clear();
  m_WorkIdToData.Clear();
}

template <typename T>
EZ_FORCE_INLINE typename ezIntervalScheduler<T>::DataMap::Iterator ezIntervalScheduler<T>::InsertData(Data& data)
{
  // make sure that we have a unique due time since the map can't store multiple keys with the same value
  int pos = 0;
  while (m_Data.Contains(data.m_DueTime))
  {
    data.m_DueTime += GetRandomTimeJitter(pos++, m_uiSeed);
  }

  return m_Data.Insert(data.m_DueTime, data);
}
