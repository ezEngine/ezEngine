#pragma once

#include <GuiFoundation/Basics.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Containers/DynamicArray.h>

class ezEventTrack;

class EZ_GUIFOUNDATION_DLL ezEventTrackControlPointData : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEventTrackControlPointData, ezReflectedClass);
public:

  double GetTickAsTime() const { return m_iTick / 4800.0; }
  void SetTickFromTime(double time, ezInt64 fps);

  ezInt64 m_iTick; // 4800 ticks per second
  ezString m_sEvent;
};

class EZ_GUIFOUNDATION_DLL ezEventTrackData : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEventTrackData, ezReflectedClass);
public:

  ezInt64 TickFromTime(double time);
  void ConvertToRuntimeData(ezEventTrack& out_Result) const;

  ezUInt16 m_uiFramesPerSecond = 60;
  ezDynamicArray<ezEventTrackControlPointData> m_ControlPoints;
};

