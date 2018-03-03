#include <PCH.h>
#include <Foundation/Tracks/EventTrack.h>

ezEventTrack::ezEventTrack()
{
}

ezEventTrack::~ezEventTrack()
{
}

void ezEventTrack::Clear()
{
  m_Events.Clear();
  m_ControlPoints.Clear();
}

bool ezEventTrack::IsEmpty() const
{
  return m_ControlPoints.IsEmpty();
}

void ezEventTrack::AddControlPoint(ezTime time, const char* szEvent)
{
  m_bSort = true;

  const ezUInt32 uiNumEvents = m_Events.GetCount();

  auto& cp = m_ControlPoints.ExpandAndGetRef();
  cp.m_Time = time;

  // search for existing event
  {
    ezTempHashedString tmp(szEvent);

    for (ezUInt32 i = 0; i < uiNumEvents; ++i)
    {
      if (m_Events[i] == tmp)
      {
        cp.m_uiEvent = i;
        return;
      }
    }
  }

  // not found -> add event name
  {
    cp.m_uiEvent = uiNumEvents;

    ezHashedString hs;
    hs.Assign(szEvent);

    m_Events.PushBack(hs);
  }
}

ezUInt32 ezEventTrack::FindControlPointAfter(ezTime x) const
{
  ezUInt32 uiLowIdx = 0;
  ezUInt32 uiHighIdx = m_ControlPoints.GetCount();

  // do a binary search to reduce the search space
  while (uiHighIdx - uiLowIdx > 8)
  {
    const ezUInt32 uiMidIdx = uiLowIdx + ((uiHighIdx - uiLowIdx) >> 1); // lerp

    // doesn't matter whether to use > or >=
    if (m_ControlPoints[uiMidIdx].m_Time > x)
      uiHighIdx = uiMidIdx;
    else
      uiLowIdx = uiMidIdx;
  }

  // now do a linear search to find the final item
  for (ezUInt32 idx = uiLowIdx; idx < uiHighIdx; ++idx)
  {
    if (m_ControlPoints[idx].m_Time >= x)
    {
      return idx;
    }
  }

  return m_ControlPoints.GetCount();
}

void ezEventTrack::Sample(ezTime rangeStart, ezTime rangeEnd, ezHybridArray<ezHashedString, 8>& out_Events) const
{
  out_Events.Clear();

  if (m_bSort)
  {
    m_bSort = false;
    m_ControlPoints.Sort();
  }

  ezUInt32 uiCurCpIdx = FindControlPointAfter(rangeStart);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  if (uiCurCpIdx > 0)
  {
    // make sure we didn't accidentally skip an important control point
    EZ_ASSERT_DEBUG(m_ControlPoints[uiCurCpIdx - 1].m_Time < rangeStart, "Invalid control point computation");
  }
#endif

  const ezUInt32 uiNumCPs = m_ControlPoints.GetCount();
  while (uiCurCpIdx < uiNumCPs && m_ControlPoints[uiCurCpIdx].m_Time < rangeEnd)
  {
    const ezHashedString& sEvent = m_Events[m_ControlPoints[uiCurCpIdx].m_uiEvent];

    out_Events.PushBack(sEvent);

    ++uiCurCpIdx;
  }
}

