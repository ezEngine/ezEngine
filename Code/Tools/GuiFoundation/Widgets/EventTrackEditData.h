#pragma once

#include <GuiFoundation/Basics.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Strings/HashedString.h>

class ezEventTrack;

class EZ_GUIFOUNDATION_DLL ezEventTrackControlPointData : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEventTrackControlPointData, ezReflectedClass);
public:

  double GetTickAsTime() const { return m_iTick / 4800.0; }
  void SetTickFromTime(double time, ezInt64 fps);
  const char* GetEventName() const { return m_sEvent.GetData(); }
  void SetEventName(const char* sz) { m_sEvent.Assign(sz); }

  ezInt64 m_iTick; // 4800 ticks per second
  ezHashedString m_sEvent;
};

class EZ_GUIFOUNDATION_DLL ezEventTrackData : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEventTrackData, ezReflectedClass);
public:

  ezInt64 TickFromTime(double time) const;
  void ConvertToRuntimeData(ezEventTrack& out_Result) const;

  ezUInt16 m_uiFramesPerSecond = 60;
  ezDynamicArray<ezEventTrackControlPointData> m_ControlPoints;
};

