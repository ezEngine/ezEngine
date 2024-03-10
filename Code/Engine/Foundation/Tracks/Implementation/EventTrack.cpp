#include <Foundation/FoundationPCH.h>

#include <Foundation/Tracks/EventTrack.h>

ezEventTrack::ezEventTrack() = default;

ezEventTrack::~ezEventTrack() = default;

void ezEventTrack::Clear()
{
  m_Events.Clear();
  m_ControlPoints.Clear();
}

bool ezEventTrack::IsEmpty() const
{
  return m_ControlPoints.IsEmpty();
}

void ezEventTrack::AddControlPoint(ezTime time, ezStringView sEvent)
{
  m_bSort = true;

  const ezUInt32 uiNumEvents = m_Events.GetCount();

  auto& cp = m_ControlPoints.ExpandAndGetRef();
  cp.m_Time = time;

  // search for existing event
  {
    ezTempHashedString tmp(sEvent);

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
    hs.Assign(sEvent);

    m_Events.PushBack(hs);
  }
}

ezUInt32 ezEventTrack::FindControlPointAfter(ezTime x) const
{
  // searches for a control point after OR AT x

  EZ_ASSERT_DEBUG(!m_ControlPoints.IsEmpty(), "");

  ezUInt32 uiLowIdx = 0;
  ezUInt32 uiHighIdx = m_ControlPoints.GetCount() - 1;

  // do a binary search to reduce the search space
  while (uiHighIdx - uiLowIdx > 8)
  {
    const ezUInt32 uiMidIdx = uiLowIdx + ((uiHighIdx - uiLowIdx) >> 1); // lerp

    if (m_ControlPoints[uiMidIdx].m_Time >= x)
      uiHighIdx = uiMidIdx;
    else
      uiLowIdx = uiMidIdx;
  }

  // now do a linear search to find the final item
  for (ezUInt32 idx = uiLowIdx; idx <= uiHighIdx; ++idx)
  {
    if (m_ControlPoints[idx].m_Time >= x)
    {
      return idx;
    }
  }

  EZ_ASSERT_DEBUG(uiHighIdx + 1 == m_ControlPoints.GetCount(), "Unexpected event track entry index");
  return m_ControlPoints.GetCount();
}

ezInt32 ezEventTrack::FindControlPointBefore(ezTime x) const
{
  // searches for a control point before OR AT x

  EZ_ASSERT_DEBUG(!m_ControlPoints.IsEmpty(), "");

  ezInt32 iLowIdx = 0;
  ezInt32 iHighIdx = (ezInt32)m_ControlPoints.GetCount() - 1;

  // do a binary search to reduce the search space
  while (iHighIdx - iLowIdx > 8)
  {
    const ezInt32 uiMidIdx = iLowIdx + ((iHighIdx - iLowIdx) >> 1); // lerp

    if (m_ControlPoints[uiMidIdx].m_Time >= x)
      iHighIdx = uiMidIdx;
    else
      iLowIdx = uiMidIdx;
  }

  // now do a linear search to find the final item
  for (ezInt32 idx = iHighIdx; idx >= iLowIdx; --idx)
  {
    if (m_ControlPoints[idx].m_Time <= x)
    {
      return idx;
    }
  }

  EZ_ASSERT_DEBUG(iLowIdx == 0, "Unexpected event track entry index");
  return -1;
}

void ezEventTrack::Sample(ezTime rangeStart, ezTime rangeEnd, ezDynamicArray<ezHashedString>& out_events) const
{
  if (m_ControlPoints.IsEmpty())
    return;

  if (m_bSort)
  {
    m_bSort = false;
    m_ControlPoints.Sort();
  }

  if (rangeStart <= rangeEnd)
  {
    ezUInt32 curCpIdx = FindControlPointAfter(rangeStart);

    const ezUInt32 uiNumCPs = m_ControlPoints.GetCount();
    while (curCpIdx < uiNumCPs && m_ControlPoints[curCpIdx].m_Time < rangeEnd)
    {
      const ezHashedString& sEvent = m_Events[m_ControlPoints[curCpIdx].m_uiEvent];

      out_events.PushBack(sEvent);

      ++curCpIdx;
    }
  }
  else
  {
    ezInt32 curCpIdx = FindControlPointBefore(rangeStart);

    while (curCpIdx >= 0 && m_ControlPoints[curCpIdx].m_Time > rangeEnd)
    {
      const ezHashedString& sEvent = m_Events[m_ControlPoints[curCpIdx].m_uiEvent];

      out_events.PushBack(sEvent);

      --curCpIdx;
    }
  }
}

void ezEventTrack::Save(ezStreamWriter& inout_stream) const
{
  if (m_bSort)
  {
    m_bSort = false;
    m_ControlPoints.Sort();
  }

  ezUInt8 uiVersion = 1;
  inout_stream << uiVersion;

  inout_stream << m_Events.GetCount();
  for (const ezHashedString& name : m_Events)
  {
    inout_stream << name.GetString();
  }

  inout_stream << m_ControlPoints.GetCount();
  for (const ControlPoint& cp : m_ControlPoints)
  {
    inout_stream << cp.m_Time;
    inout_stream << cp.m_uiEvent;
  }
}

void ezEventTrack::Load(ezStreamReader& inout_stream)
{
  // don't rely on the data being sorted
  m_bSort = true;

  ezUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  EZ_ASSERT_DEV(uiVersion == 1, "Invalid event track version {0}", uiVersion);

  ezUInt32 count = 0;
  ezStringBuilder tmp;

  inout_stream >> count;
  m_Events.SetCount(count);
  for (ezHashedString& name : m_Events)
  {
    inout_stream >> tmp;
    name.Assign(tmp);
  }

  inout_stream >> count;
  m_ControlPoints.SetCount(count);
  for (ControlPoint& cp : m_ControlPoints)
  {
    inout_stream >> cp.m_Time;
    inout_stream >> cp.m_uiEvent;
  }
}
