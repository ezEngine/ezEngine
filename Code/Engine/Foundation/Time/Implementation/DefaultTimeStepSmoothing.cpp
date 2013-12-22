#include <Foundation/PCH.h>
#include <Foundation/Time/DefaultTimeStepSmoothing.h>

ezDefaultTimeStepSmoothing::ezDefaultTimeStepSmoothing()
{
  m_fLerpFactor = 0.2f;
}

void ezDefaultTimeStepSmoothing::Reset(const ezClock* pClock)
{
  m_LastTimeSteps.Clear();
}

ezTime ezDefaultTimeStepSmoothing::GetSmoothedTimeStep(ezTime RawTimeStep, const ezClock* pClock)
{
  RawTimeStep = ezMath::Clamp(RawTimeStep * pClock->GetSpeed(), pClock->GetMinimumTimeStep(), pClock->GetMaximumTimeStep());

  if (m_LastTimeSteps.IsEmpty())
  {
    m_LastTimeSteps.PushBack(RawTimeStep);
    m_LastTimeStepTaken = RawTimeStep;
    return m_LastTimeStepTaken;
  }

  if (!m_LastTimeSteps.CanAppend(1))
    m_LastTimeSteps.PopFront(1);

  m_LastTimeSteps.PushBack(RawTimeStep);

  ezStaticArray<ezTime, 11> Sorted;
  Sorted.SetCount(m_LastTimeSteps.GetCount());

  for (ezUInt32 i = 0; i < m_LastTimeSteps.GetCount(); ++i)
    Sorted[i] = m_LastTimeSteps[i];

  Sorted.Sort();

  ezUInt32 uiFirstSample = 2;
  ezUInt32 uiLastSample = 8;

  if (Sorted.GetCount() < 7)
  {
    uiFirstSample = 0;
    uiLastSample = Sorted.GetCount() - 1;
  }
  else
  if (Sorted.GetCount() < 10)
  {
    uiFirstSample = 1;
    uiLastSample = Sorted.GetCount() - 2;
  }

  ezTime tAvg;

  for (ezUInt32 i = uiFirstSample; i <= uiLastSample; ++i)
  {
    tAvg = tAvg + Sorted[i];
  }

  tAvg = tAvg / (double) ((uiLastSample - uiFirstSample) + 1.0);


  m_LastTimeStepTaken = ezMath::Lerp(m_LastTimeStepTaken, tAvg, m_fLerpFactor);

  return m_LastTimeStepTaken;
}






EZ_STATICLINK_FILE(Foundation, Foundation_Time_Implementation_DefaultTimeStepSmoothing);

