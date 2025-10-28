#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <RmlUi/Include/RmlUi/Core.h>

struct EZ_RMLUIPLUGIN_DLL ezRmlUiInputState
{
  ezVec2 m_vCursorPos;
  ezUInt32 m_uiMouseButton0Pressed : 1;
  ezUInt32 m_uiMouseButton1Pressed : 1;
  ezUInt32 m_uiMouseButton2Pressed : 1;
  //ezUInt32 m_uiCtrlPressed : 1;
  //ezUInt32 m_uiShiftPressed : 1;
  //ezUInt32 m_uiAltPressed : 1;
  // TODO: the rest

  ezRmlUiInputState() 
    : m_vCursorPos(ezVec2::MakeZero())
    , m_uiMouseButton0Pressed(0)
  {
  }

  inline bool operator==(const ezRmlUiInputState& rhs) const
  {
    return m_vCursorPos.IsEqual(rhs.m_vCursorPos, 0.001f)
      && m_uiMouseButton0Pressed == rhs.m_uiMouseButton0Pressed
      && m_uiMouseButton1Pressed == rhs.m_uiMouseButton1Pressed
      && m_uiMouseButton2Pressed == rhs.m_uiMouseButton2Pressed;
      //&& m_uiCtrlPressed == rhs.m_uiCtrlPressed
      //&& m_uiShiftPressed == rhs.m_uiShiftPressed
      //&& m_uiAltPressed == rhs.m_uiAltPressed;
  }

  inline bool IsAnyButtonPressed() const
  {
    return m_uiMouseButton0Pressed == 1
      || m_uiMouseButton1Pressed == 1
      || m_uiMouseButton2Pressed == 1;
  }
};
