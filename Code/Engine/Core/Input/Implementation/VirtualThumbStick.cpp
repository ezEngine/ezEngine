#include <Core/PCH.h>
#include <Core/Input/VirtualThumbStick.h>
#include <Core/Input/InputManager.h>
#include <Foundation/Strings/StringBuilder.h>

ezInt32 ezVirtualThumbStick::s_iThumbsticks = 0;

ezVirtualThumbStick::ezVirtualThumbStick()
{
  SetAreaFocusMode(ezInputManager::ezInputActionConfig::OnEnterArea::RequireKeyUp, ezInputManager::ezInputActionConfig::OnLeaveArea::KeepFocus);
  SetTriggerInputSlot(ezVirtualThumbStick::InputTrigger::Touchpoint);
  SetThumbstickOutput(ezVirtualThumbStick::OutputTrigger::Controller0_LeftStick);

  SetInputArea(ezVec2(0.0f), ezVec2(0.0f), 0.0f);

  ezStringBuilder s;
  s.Format("Thumbstick_%i", s_iThumbsticks);
  m_sName = s.GetData();

  ++s_iThumbsticks;

  m_bEnabled = false;
  m_bConfigChanged = false;
}

ezVirtualThumbStick::~ezVirtualThumbStick()
{
  ezInputManager::RemoveInputAction(GetDeviceName(), m_sName.GetData());
}

void ezVirtualThumbStick::SetTriggerInputSlot(ezVirtualThumbStick::InputTrigger::Enum Input, const char* szFilterAxisX, const char* szFilterAxisY, const char* szTrigger1, const char* szTrigger2, const char* szTrigger3)
{
  switch (Input)
  {
  case ezVirtualThumbStick::InputTrigger::Touchpoint:
    {
      m_szFilterAxisX = ezInputSlot_TouchPoint0_PositionX;
      m_szFilterAxisY = ezInputSlot_TouchPoint0_PositionY;
      m_szTrigger1    = ezInputSlot_TouchPoint0;
      m_szTrigger2    = ezInputSlot_TouchPoint1;
      m_szTrigger3    = ezInputSlot_TouchPoint2;
    }
    break;
  case ezVirtualThumbStick::InputTrigger::Custom:
    {
      m_szFilterAxisX = szFilterAxisX;
      m_szFilterAxisY = szFilterAxisY;
      m_szTrigger1    = szTrigger1;
      m_szTrigger2    = szTrigger2;
      m_szTrigger3    = szTrigger3;
    }
    break;
  }

  m_bConfigChanged = true;
}

void ezVirtualThumbStick::SetThumbstickOutput(ezVirtualThumbStick::OutputTrigger::Enum Output, const char* szOutputLeft, const char* szOutputRight, const char* szOutputUp, const char* szOutputDown)
{
  switch (Output)
  {
  case ezVirtualThumbStick::OutputTrigger::Controller0_LeftStick:
    {
      m_szOutputLeft  = ezInputSlot_Controller0_LeftStick_NegX;
      m_szOutputRight = ezInputSlot_Controller0_LeftStick_PosX;
      m_szOutputUp    = ezInputSlot_Controller0_LeftStick_PosY;
      m_szOutputDown  = ezInputSlot_Controller0_LeftStick_NegY;
    }
    break;
  case ezVirtualThumbStick::OutputTrigger::Controller0_RightStick:
    {
      m_szOutputLeft  = ezInputSlot_Controller0_RightStick_NegX;
      m_szOutputRight = ezInputSlot_Controller0_RightStick_PosX;
      m_szOutputUp    = ezInputSlot_Controller0_RightStick_PosY;
      m_szOutputDown  = ezInputSlot_Controller0_RightStick_NegY;
    }
    break;
  case ezVirtualThumbStick::OutputTrigger::Custom:
    {
      m_szOutputLeft  = szOutputLeft;
      m_szOutputRight = szOutputRight;
      m_szOutputUp    = szOutputUp;
      m_szOutputDown  = szOutputDown;
    }
    break;
  }

  m_bConfigChanged = true;
}

void ezVirtualThumbStick::SetAreaFocusMode(ezInputManager::ezInputActionConfig::OnEnterArea OnEnter, ezInputManager::ezInputActionConfig::OnLeaveArea OnLeave)
{
  m_bConfigChanged = true;

  m_OnEnter = OnEnter;
  m_OnLeave = OnLeave;
}

void ezVirtualThumbStick::SetInputArea(const ezVec2& vLowerLeft, const ezVec2& vUpperRight, float fPriority)
{
  m_bConfigChanged = true;

  m_vLowerLeft = vLowerLeft;
  m_vUpperRight = vUpperRight;
  m_fPriority = fPriority;
}

void ezVirtualThumbStick::GetInputArea(ezVec2& out_vLowerLeft, ezVec2& out_vUpperRight)
{
  out_vLowerLeft = m_vLowerLeft;
  out_vUpperRight = m_vUpperRight;
}

void ezVirtualThumbStick::UpdateActionMapping()
{
  if (!m_bConfigChanged)
    return;

  ezInputManager::ezInputActionConfig ac;

  ac.m_fFilterXMinValue = m_vLowerLeft.x;
  ac.m_fFilterXMaxValue = m_vUpperRight.x;
  ac.m_fFilterYMinValue = m_vLowerLeft.y;
  ac.m_fFilterYMaxValue = m_vUpperRight.y;
  ac.m_fFilteredPriority = m_fPriority;
  ac.m_OnEnterArea = m_OnEnter;
  ac.m_OnLeaveArea = m_OnLeave;
  ac.m_sFilterByInputSlotX  = m_szFilterAxisX;
  ac.m_sFilterByInputSlotY  = m_szFilterAxisY;
  ac.m_sInputSlotTrigger[0] = m_szTrigger1;
  ac.m_sInputSlotTrigger[1] = m_szTrigger2;
  ac.m_sInputSlotTrigger[2] = m_szTrigger3;

  ezInputManager::SetInputActionConfig(GetDeviceName(), m_sName.GetData(), ac, false);

  m_bConfigChanged = false;
}

void ezVirtualThumbStick::UpdateInputSlotValues()
{
  m_InputSlotValues[m_szOutputLeft ]  = 0.0f;
  m_InputSlotValues[m_szOutputRight]  = 0.0f;
  m_InputSlotValues[m_szOutputUp   ]  = 0.0f;
  m_InputSlotValues[m_szOutputDown ]  = 0.0f;

  if (!m_bEnabled)
  {
    ezInputManager::RemoveInputAction(GetDeviceName(), m_sName.GetData());
    return;
  }

  UpdateActionMapping();
  
  if (ezInputManager::GetInputActionState(GetDeviceName(), m_sName.GetData()) != ezKeyState::Up)
  {
    const ezVec2 vHalf = (m_vUpperRight - m_vLowerLeft) * 0.5f;
    const float fRadius = ezMath::Min(vHalf.x, vHalf.y);
    const ezVec2 vCenter = vHalf + m_vLowerLeft;

    ezVec2 vTouchPos(0.0f);

    ezInputManager::GetInputSlotState(m_szFilterAxisX, &vTouchPos.x);
    ezInputManager::GetInputSlotState(m_szFilterAxisY, &vTouchPos.y);

    ezVec2 vDir = vTouchPos - vCenter;
    vDir.y *= -1;

    const float fLength = ezMath::Min(vDir.GetLength(), fRadius) / fRadius;
    vDir.Normalize();

    m_InputSlotValues[m_szOutputLeft ] = ezMath::Max(0.0f, -vDir.x) * fLength;
    m_InputSlotValues[m_szOutputRight] = ezMath::Max(0.0f,  vDir.x) * fLength;
    m_InputSlotValues[m_szOutputUp   ] = ezMath::Max(0.0f,  vDir.y) * fLength;
    m_InputSlotValues[m_szOutputDown ] = ezMath::Max(0.0f, -vDir.y) * fLength;
  }
}

void ezVirtualThumbStick::RegisterInputSlots()
{
  RegisterInputSlot(ezInputSlot_Controller0_LeftStick_NegX,   "Left Stick Left",   ezInputSlotFlags::IsAnalogStick);
  RegisterInputSlot(ezInputSlot_Controller0_LeftStick_PosX,   "Left Stick Right",  ezInputSlotFlags::IsAnalogStick);
  RegisterInputSlot(ezInputSlot_Controller0_LeftStick_NegY,   "Left Stick Down",   ezInputSlotFlags::IsAnalogStick);
  RegisterInputSlot(ezInputSlot_Controller0_LeftStick_PosY,   "Left Stick Up",     ezInputSlotFlags::IsAnalogStick);

  RegisterInputSlot(ezInputSlot_Controller0_RightStick_NegX,  "Right Stick Left",  ezInputSlotFlags::IsAnalogStick);
  RegisterInputSlot(ezInputSlot_Controller0_RightStick_PosX,  "Right Stick Right", ezInputSlotFlags::IsAnalogStick);
  RegisterInputSlot(ezInputSlot_Controller0_RightStick_NegY,  "Right Stick Down",  ezInputSlotFlags::IsAnalogStick);
  RegisterInputSlot(ezInputSlot_Controller0_RightStick_PosY,  "Right Stick Up",    ezInputSlotFlags::IsAnalogStick);
}



