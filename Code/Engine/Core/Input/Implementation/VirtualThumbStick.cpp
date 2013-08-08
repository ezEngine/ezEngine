#include <Core/PCH.h>
#include <Core/Input/VirtualThumbStick.h>
#include <Core/Input/InputManager.h>
#include <Foundation/Strings/StringBuilder.h>

ezInt32 ezVirtualThumbStick::s_iThumbsticks = 0;

ezVirtualThumbStick::ezVirtualThumbStick()
{
  SetAreaFocusMode(ezInputManager::ezInputActionConfig::OnEnterArea::RequireKeyUp, ezInputManager::ezInputActionConfig::OnLeaveArea::KeepFocus);
  SetTriggerInputSlot(ezVirtualThumbStick::Input::Touchpoint);
  SetThumbstickOutput(ezVirtualThumbStick::Output::Controller0_LeftStick);

  SetInputArea(ezVec2(0.0f), ezVec2(0.0f), 0.0f, 0.0f);

  ezStringBuilder s;
  s.Format("Thumbstick_%i", s_iThumbsticks);
  m_sName = s.GetData();

  ++s_iThumbsticks;

  m_bEnabled = false;
  m_bConfigChanged = false;
  m_bIsActive = false;
}

ezVirtualThumbStick::~ezVirtualThumbStick()
{
  ezInputManager::RemoveInputAction(GetDeviceName(), m_sName.GetData());
}

void ezVirtualThumbStick::SetTriggerInputSlot(ezVirtualThumbStick::Input::Enum Input, const char* szFilterAxisX, const char* szFilterAxisY, const char* szTrigger1, const char* szTrigger2, const char* szTrigger3)
{
  switch (Input)
  {
  case ezVirtualThumbStick::Input::Touchpoint:
    {
      m_szFilterAxisX = ezInputSlot_TouchPoint0_PositionX;
      m_szFilterAxisY = ezInputSlot_TouchPoint0_PositionY;
      m_szTrigger1    = ezInputSlot_TouchPoint0;
      m_szTrigger2    = ezInputSlot_None;
      m_szTrigger3    = ezInputSlot_None;
    }
    break;
  case ezVirtualThumbStick::Input::MousePosition:
    {
      m_szFilterAxisX = ezInputSlot_MousePositionX;
      m_szFilterAxisY = ezInputSlot_MousePositionY;
      m_szTrigger1    = ezInputSlot_MouseButton0;
      m_szTrigger2    = ezInputSlot_None;
      m_szTrigger3    = ezInputSlot_None;
    }
    break;
  case ezVirtualThumbStick::Input::Custom:
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

void ezVirtualThumbStick::SetThumbstickOutput(ezVirtualThumbStick::Output::Enum Output, const char* szOutputLeft, const char* szOutputRight, const char* szOutputUp, const char* szOutputDown)
{
  switch (Output)
  {
  case ezVirtualThumbStick::Output::Controller0_LeftStick:
    {
      m_szOutputLeft  = ezInputSlot_Controller0_LeftStick_NegX;
      m_szOutputRight = ezInputSlot_Controller0_LeftStick_PosX;
      m_szOutputUp    = ezInputSlot_Controller0_LeftStick_PosY;
      m_szOutputDown  = ezInputSlot_Controller0_LeftStick_NegY;
    }
    break;
  case ezVirtualThumbStick::Output::Controller0_RightStick:
    {
      m_szOutputLeft  = ezInputSlot_Controller0_RightStick_NegX;
      m_szOutputRight = ezInputSlot_Controller0_RightStick_PosX;
      m_szOutputUp    = ezInputSlot_Controller0_RightStick_PosY;
      m_szOutputDown  = ezInputSlot_Controller0_RightStick_NegY;
    }
    break;
  case ezVirtualThumbStick::Output::Controller1_LeftStick:
    {
      m_szOutputLeft  = ezInputSlot_Controller1_LeftStick_NegX;
      m_szOutputRight = ezInputSlot_Controller1_LeftStick_PosX;
      m_szOutputUp    = ezInputSlot_Controller1_LeftStick_PosY;
      m_szOutputDown  = ezInputSlot_Controller1_LeftStick_NegY;
    }
    break;
  case ezVirtualThumbStick::Output::Controller1_RightStick:
    {
      m_szOutputLeft  = ezInputSlot_Controller1_RightStick_NegX;
      m_szOutputRight = ezInputSlot_Controller1_RightStick_PosX;
      m_szOutputUp    = ezInputSlot_Controller1_RightStick_PosY;
      m_szOutputDown  = ezInputSlot_Controller1_RightStick_NegY;
    }
    break;
  case ezVirtualThumbStick::Output::Controller2_LeftStick:
    {
      m_szOutputLeft  = ezInputSlot_Controller2_LeftStick_NegX;
      m_szOutputRight = ezInputSlot_Controller2_LeftStick_PosX;
      m_szOutputUp    = ezInputSlot_Controller2_LeftStick_PosY;
      m_szOutputDown  = ezInputSlot_Controller2_LeftStick_NegY;
    }
    break;
  case ezVirtualThumbStick::Output::Controller2_RightStick:
    {
      m_szOutputLeft  = ezInputSlot_Controller2_RightStick_NegX;
      m_szOutputRight = ezInputSlot_Controller2_RightStick_PosX;
      m_szOutputUp    = ezInputSlot_Controller2_RightStick_PosY;
      m_szOutputDown  = ezInputSlot_Controller2_RightStick_NegY;
    }
    break;
  case ezVirtualThumbStick::Output::Controller3_LeftStick:
    {
      m_szOutputLeft  = ezInputSlot_Controller3_LeftStick_NegX;
      m_szOutputRight = ezInputSlot_Controller3_LeftStick_PosX;
      m_szOutputUp    = ezInputSlot_Controller3_LeftStick_PosY;
      m_szOutputDown  = ezInputSlot_Controller3_LeftStick_NegY;
    }
    break;
  case ezVirtualThumbStick::Output::Controller3_RightStick:
    {
      m_szOutputLeft  = ezInputSlot_Controller3_RightStick_NegX;
      m_szOutputRight = ezInputSlot_Controller3_RightStick_PosX;
      m_szOutputUp    = ezInputSlot_Controller3_RightStick_PosY;
      m_szOutputDown  = ezInputSlot_Controller3_RightStick_NegY;
    }
    break;
  case ezVirtualThumbStick::Output::Custom:
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

void ezVirtualThumbStick::SetInputArea(const ezVec2& vLowerLeft, const ezVec2& vUpperRight, float fThumbstickRadius, float fPriority, CenterMode::Enum center)
{
  m_bConfigChanged = true;

  m_vLowerLeft = vLowerLeft;
  m_vUpperRight = vUpperRight;
  m_fRadius = fThumbstickRadius;
  m_fPriority = fPriority;
  m_CenterMode = center;
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
  m_bIsActive = false;

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

  const ezKeyState::Enum ks = ezInputManager::GetInputActionState(GetDeviceName(), m_sName.GetData());
  
  if (ks != ezKeyState::Up)
  {
    m_bIsActive = true;

    ezVec2 vTouchPos(0.0f);

    ezInputManager::GetInputSlotState(m_szFilterAxisX, &vTouchPos.x);
    ezInputManager::GetInputSlotState(m_szFilterAxisY, &vTouchPos.y);

    if (ks == ezKeyState::Pressed)
    {
      switch (m_CenterMode)
      {
      case CenterMode::InputArea:
        m_vCenter = m_vLowerLeft + (m_vUpperRight - m_vLowerLeft) * 0.5f;
        break;
      case CenterMode::ActivationPoint:
        m_vCenter = vTouchPos;
        break;
      }
    }

    ezVec2 vDir = vTouchPos - m_vCenter;
    vDir.y *= -1;

    const float fLength = ezMath::Min(vDir.GetLength(), m_fRadius) / m_fRadius;
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



