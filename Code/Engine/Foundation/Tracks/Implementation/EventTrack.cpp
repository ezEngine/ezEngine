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

ezInt32 ezEventTrack::FindControlPointBefore(ezTime x) const
{
  ezInt32 uiLowIdx = 0;
  ezInt32 uiHighIdx = (ezInt32)m_ControlPoints.GetCount();

  // do a binary search to reduce the search space
  while (uiHighIdx - uiLowIdx > 8)
  {
    const ezInt32 uiMidIdx = uiLowIdx + ((uiHighIdx - uiLowIdx) >> 1); // lerp

                                                                        // doesn't matter whether to use > or >=
    if (m_ControlPoints[uiMidIdx].m_Time >= x)
      uiHighIdx = uiMidIdx;
    else
      uiLowIdx = uiMidIdx;
  }

  // now do a linear search to find the final item
  for (ezInt32 idx = uiHighIdx; idx > uiLowIdx; --idx)
  {
    if (m_ControlPoints[idx - 1].m_Time <= x)
    {
      return idx - 1;
    }
  }

  return -1;
}

void ezEventTrack::Sample(ezTime rangeStart, ezTime rangeEnd, ezHybridArray<ezHashedString, 8>& out_Events) const
{
  out_Events.Clear();

  if (m_bSort)
  {
    m_bSort = false;
    m_ControlPoints.Sort();
  }

  if (rangeStart <= rangeEnd)
  {
    ezUInt32 curCpIdx = FindControlPointAfter(rangeStart);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    if (curCpIdx > 0)
    {
      // make sure we didn't accidentally skip an important control point
      EZ_ASSERT_DEBUG(m_ControlPoints[curCpIdx - 1].m_Time < rangeStart, "Invalid control point computation");
    }
#endif

    const ezUInt32 uiNumCPs = m_ControlPoints.GetCount();
    while (curCpIdx < uiNumCPs && m_ControlPoints[curCpIdx].m_Time < rangeEnd)
    {
      const ezHashedString& sEvent = m_Events[m_ControlPoints[curCpIdx].m_uiEvent];

      out_Events.PushBack(sEvent);

      ++curCpIdx;
    }
  }
  else
  {
    ezInt32 curCpIdx = FindControlPointBefore(rangeStart);

    while (curCpIdx >= 0 && m_ControlPoints[curCpIdx].m_Time > rangeEnd)
    {
      const ezHashedString& sEvent = m_Events[m_ControlPoints[curCpIdx].m_uiEvent];

      out_Events.PushBack(sEvent);

      --curCpIdx;
    }
  }
}

void ezEventTrack::Save(ezStreamWriter& stream) const
{
  if (m_bSort)
  {
    m_bSort = false;
    m_ControlPoints.Sort();
  }

  ezUInt8 uiVersion = 1;
  stream << uiVersion;

  stream << m_Events.GetCount();
  for (const ezHashedString& name : m_Events)
  {
    stream << name.GetString();
  }

  stream << m_ControlPoints.GetCount();
  for (const ControlPoint& cp : m_ControlPoints)
  {
    stream << cp.m_Time;
    stream << cp.m_uiEvent;
  }
}

void ezEventTrack::Load(ezStreamReader& stream)
{
  // don't rely on the data being sorted
  m_bSort = true;

  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion == 1, "Invalid event track version {0}", uiVersion);

  ezUInt32 count = 0;
  ezStringBuilder tmp;

  stream >> count;
  m_Events.SetCount(count);
  for (ezHashedString& name : m_Events)
  {
    stream >> tmp;
    name.Assign(tmp.GetData());
  }

  stream >> count;
  m_ControlPoints.SetCount(count);
  for (ControlPoint& cp : m_ControlPoints)
  {
    stream >> cp.m_Time;
    stream >> cp.m_uiEvent;
  }
}



EZ_STATICLINK_FILE(Foundation, Foundation_Tracks_Implementation_EventTrack);

