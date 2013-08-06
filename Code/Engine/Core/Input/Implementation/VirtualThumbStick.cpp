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

  SetInputArea(ezVec2(0.1f), ezVec2(0.2f));

  ezStringBuilder s;
  s.Format("Thumbstick_%i", s_iThumbsticks);
  m_sName = s.GetData();

  ++s_iThumbsticks;

  m_bEnabled = true;
}

ezVirtualThumbStick::~ezVirtualThumbStick()
{
  ezInputManager::RemoveInputAction("VirtualThumbsticks", m_sName.GetData());
}

void ezVirtualThumbStick::SetTriggerInputSlot(ezVirtualThumbStick::InputTrigger::Enum Input, const char* szFilterAxisX, const char* szFilterAxisY, const char* szTrigger1, const char* szTrigger2, const char* szTrigger3)
{
  switch (Input)
  {
  case ezVirtualThumbStick::InputTrigger::Touchpoint:
    {
      m_szFilterAxisX = "touchpoint_0_position_x";
      m_szFilterAxisY = "touchpoint_0_position_y";
      m_szTrigger1    = "touchpoint_0";
      m_szTrigger2    = "touchpoint_1";
      m_szTrigger3    = "touchpoint_2";
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
}

void ezVirtualThumbStick::SetThumbstickOutput(ezVirtualThumbStick::OutputTrigger::Enum Output, const char* szOutputLeft, const char* szOutputRight, const char* szOutputUp, const char* szOutputDown)
{
  switch (Output)
  {
  case ezVirtualThumbStick::OutputTrigger::Controller0_LeftStick:
    {
      m_szOutputLeft  = "controller0_leftstick_negx";
      m_szOutputRight = "controller0_leftstick_posx";
      m_szOutputUp    = "controller0_leftstick_posy";
      m_szOutputDown  = "controller0_leftstick_negy";
    }
    break;
  case ezVirtualThumbStick::OutputTrigger::Controller0_RightStick:
    {
      m_szOutputLeft  = "controller0_rightstick_negx";
      m_szOutputRight = "controller0_rightstick_posx";
      m_szOutputUp    = "controller0_rightstick_posy";
      m_szOutputDown  = "controller0_rightstick_negy";
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
}

void ezVirtualThumbStick::SetAreaFocusMode(ezInputManager::ezInputActionConfig::OnEnterArea OnEnter, ezInputManager::ezInputActionConfig::OnLeaveArea OnLeave)
{
  m_OnEnter = OnEnter;
  m_OnLeave = OnLeave;
}

void ezVirtualThumbStick::SetInputArea(const ezVec2& vLowerLeft, const ezVec2& vUpperRight)
{
  m_vLowerLeft = vLowerLeft;
  m_vUpperRight = vUpperRight;
}

void ezVirtualThumbStick::GetInputArea(ezVec2& out_vLowerLeft, ezVec2& out_vUpperRight)
{
  out_vLowerLeft = m_vLowerLeft;
  out_vUpperRight = m_vUpperRight;
}

void ezVirtualThumbStick::UpdateActionMapping()
{
  ezInputManager::ezInputActionConfig ac;

  ac.m_fFilterXMinValue = m_vLowerLeft.x;
  ac.m_fFilterXMaxValue = m_vUpperRight.x;
  ac.m_fFilterYMinValue = m_vLowerLeft.y;
  ac.m_fFilterYMaxValue = m_vUpperRight.y;
  ac.m_fFilteredPriority = 1.0f;
  ac.m_OnEnterArea = m_OnEnter;
  ac.m_OnLeaveArea = m_OnLeave;
  ac.m_sFilterByInputSlotX  = m_szFilterAxisX;
  ac.m_sFilterByInputSlotY  = m_szFilterAxisY;
  ac.m_sInputSlotTrigger[0] = m_szTrigger1;
  ac.m_sInputSlotTrigger[1] = m_szTrigger2;
  ac.m_sInputSlotTrigger[2] = m_szTrigger3;

  ezInputManager::SetInputActionConfig("VirtualThumbsticks", m_sName.GetData(), ac, false);
}

void ezVirtualThumbStick::UpdateInputSlotValues()
{
  m_InputSlotValues[m_szOutputLeft ]  = 0.0f;
  m_InputSlotValues[m_szOutputRight]  = 0.0f;
  m_InputSlotValues[m_szOutputUp   ]  = 0.0f;
  m_InputSlotValues[m_szOutputDown ]  = 0.0f;

  if (!m_bEnabled)
  {
    ezInputManager::RemoveInputAction("VirtualThumbsticks", m_sName.GetData());
    return;
  }

  UpdateActionMapping();
  
  if (ezInputManager::GetInputActionState("VirtualThumbsticks", m_sName.GetData()) != ezKeyState::Up)
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
  RegisterInputSlot("controller0_leftstick_negx",   "Left Stick Left",   ezInputSlotFlags::IsAnalogStick);
  RegisterInputSlot("controller0_leftstick_posx",   "Left Stick Right",  ezInputSlotFlags::IsAnalogStick);
  RegisterInputSlot("controller0_leftstick_negy",   "Left Stick Down",   ezInputSlotFlags::IsAnalogStick);
  RegisterInputSlot("controller0_leftstick_posy",   "Left Stick Up",     ezInputSlotFlags::IsAnalogStick);

  RegisterInputSlot("controller0_rightstick_negx",  "Right Stick Left",  ezInputSlotFlags::IsAnalogStick);
  RegisterInputSlot("controller0_rightstick_posx",  "Right Stick Right", ezInputSlotFlags::IsAnalogStick);
  RegisterInputSlot("controller0_rightstick_negy",  "Right Stick Down",  ezInputSlotFlags::IsAnalogStick);
  RegisterInputSlot("controller0_rightstick_posy",  "Right Stick Up",    ezInputSlotFlags::IsAnalogStick);
}



