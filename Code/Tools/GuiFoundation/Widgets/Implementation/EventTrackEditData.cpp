#include <PCH.h>
#include <GuiFoundation/Widgets/EventTrackEditData.h>
#include <Foundation/Tracks/EventTrack.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OpenDdlReader.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEventTrackControlPointData, 1, ezRTTIDefaultAllocator<ezEventTrackControlPointData>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Tick", m_iTick),
    EZ_ACCESSOR_PROPERTY("Event", GetEventName, SetEventName),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEventTrackData, 3, ezRTTIDefaultAllocator<ezEventTrackData>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_MEMBER_PROPERTY("ControlPoints", m_ControlPoints),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

void ezEventTrackControlPointData::SetTickFromTime(double time, ezInt64 fps)
{
  const ezUInt32 uiTicksPerStep = 4800 / fps;
  m_iTick = (ezInt64)ezMath::Round(time * 4800.0, (double)uiTicksPerStep);
}

ezInt64 ezEventTrackData::TickFromTime(double time) const
{
  const ezUInt32 uiTicksPerStep = 4800 / m_uiFramesPerSecond;
  return (ezInt64)ezMath::Round(time * 4800.0, (double)uiTicksPerStep);
}

void ezEventTrackData::ConvertToRuntimeData(ezEventTrack& out_Result) const
{
  out_Result.Clear();

  for (const auto& cp : m_ControlPoints)
  {
    out_Result.AddControlPoint(ezTime::Seconds(cp.GetTickAsTime()), cp.m_sEvent);
  }
}

void ezEventSet::AddAvailableEvent(const char* szEvent)
{
  if (ezStringUtils::IsNullOrEmpty(szEvent))
    return;

  if (m_AvailableEvents.Contains(szEvent))
    return;

  m_bModified = true;
  m_AvailableEvents.Insert(szEvent);
}

ezResult ezEventSet::WriteToDDL(const char* szFile)
{
  ezDeferredFileWriter file;
  file.SetOutput(szFile);

  ezOpenDdlWriter ddl;
  ddl.SetOutputStream(&file);

  for (const auto& s : m_AvailableEvents)
  {
    ddl.BeginObject("Event", s.GetData());
    ddl.EndObject();
  }

  if (file.Close().Succeeded())
  {
    m_bModified = false;
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
}

ezResult ezEventSet::ReadFromDDL(const char* szFile)
{
  m_AvailableEvents.Clear();

  ezFileReader file;
  if (file.Open(szFile).Failed())
    return EZ_FAILURE;

  ezOpenDdlReader ddl;
  if (ddl.ParseDocument(file).Failed())
    return EZ_FAILURE;

  auto* pRoot = ddl.GetRootElement();

  for (auto* pChild = pRoot->GetFirstChild(); pChild != nullptr; pChild = pChild->GetSibling())
  {
    if (pChild->IsCustomType("Event"))
    {
      AddAvailableEvent(pChild->GetName());
    }
  }

  m_bModified = false;
  return EZ_SUCCESS;
}
