#include <Core/PCH.h>
#include <Core/Input/VirtualThumbStick.h>
#include <Core/Input/InputManager.h>
#include <Foundation/Strings/StringBuilder.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVirtualThumbStick, ezInputDevice, 1, ezRTTINoAllocator);
  // no properties or message handlers
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezInt32 ezVirtualThumbStick::s_iThumbsticks = 0;

ezVirtualThumbStick::ezVirtualThumbStick()
{
  SetAreaFocusMode(ezInputActionConfig::RequireKeyUp, ezInputActionConfig::KeepFocus);
  SetTriggerInputSlot(ezVirtualThumbStick::Input::Touchpoint);
  SetThumbstickOutput(ezVirtualThumbStick::Output::Controller0_LeftStick);

  SetInputArea(ezVec2(0.0f), ezVec2(0.0f), 0.0f, 0.0f);

  ezStringBuilder s;
  s.Format("Thumbstick_%i", s_iThumbsticks);
  m_sName = s;

  ++s_iThumbsticks;

  m_bEnabled = false;
  m_bConfigChanged = false;
  m_bIsActive = false;
}

ezVirtualThumbStick::~ezVirtualThumbStick()
{
  ezInputManager::RemoveInputAction(GetDynamicRTTI()->GetTypeName(), m_sName.GetData());
}

void ezVirtualThumbStick::SetTriggerInputSlot(ezVirtualThumbStick::Input::Enum Input, const ezInputActionConfig* pCustomConfig)
{
  for (ezInt32 i = 0; i < ezInputActionConfig::MaxInputSlotAlternatives; ++i)
  {
    m_ActionConfig.m_sFilterByInputSlotX[i] = ezInputSlot_None;
    m_ActionConfig.m_sFilterByInputSlotY[i] = ezInputSlot_None;
    m_ActionConfig.m_sInputSlotTrigger[i]   = ezInputSlot_None;
  }

  switch (Input)
  {
  case ezVirtualThumbStick::Input::Touchpoint:
    {
      m_ActionConfig.m_sFilterByInputSlotX[0] = ezInputSlot_TouchPoint0_PositionX;
      m_ActionConfig.m_sFilterByInputSlotY[0] = ezInputSlot_TouchPoint0_PositionY;
      m_ActionConfig.m_sInputSlotTrigger[0]   = ezInputSlot_TouchPoint0;

      m_ActionConfig.m_sFilterByInputSlotX[1] = ezInputSlot_TouchPoint1_PositionX;
      m_ActionConfig.m_sFilterByInputSlotY[1] = ezInputSlot_TouchPoint1_PositionY;
      m_ActionConfig.m_sInputSlotTrigger[1]   = ezInputSlot_TouchPoint1;

      m_ActionConfig.m_sFilterByInputSlotX[2] = ezInputSlot_TouchPoint2_PositionX;
      m_ActionConfig.m_sFilterByInputSlotY[2] = ezInputSlot_TouchPoint2_PositionY;
      m_ActionConfig.m_sInputSlotTrigger[2]   = ezInputSlot_TouchPoint2;
    }
    break;
  case ezVirtualThumbStick::Input::MousePosition:
    {
      m_ActionConfig.m_sFilterByInputSlotX[0] = ezInputSlot_MousePositionX;
      m_ActionConfig.m_sFilterByInputSlotY[0] = ezInputSlot_MousePositionY;
      m_ActionConfig.m_sInputSlotTrigger[0]   = ezInputSlot_MouseButton0;
    }
    break;
  case ezVirtualThumbStick::Input::Custom:
    {
      EZ_ASSERT_DEV(pCustomConfig != nullptr, "Must pass a custom config, if you want to have a custom config.");

      for (ezInt32 i = 0; i < ezInputActionConfig::MaxInputSlotAlternatives; ++i)
      {
        m_ActionConfig.m_sFilterByInputSlotX[i] = pCustomConfig->m_sFilterByInputSlotX[i];
        m_ActionConfig.m_sFilterByInputSlotY[i] = pCustomConfig->m_sFilterByInputSlotY[i];
        m_ActionConfig.m_sInputSlotTrigger[i]   = pCustomConfig->m_sInputSlotTrigger[i];
      }
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

void ezVirtualThumbStick::SetAreaFocusMode(ezInputActionConfig::OnEnterArea OnEnter, ezInputActionConfig::OnLeaveArea OnLeave)
{
  m_bConfigChanged = true;

  m_ActionConfig.m_OnEnterArea = OnEnter;
  m_ActionConfig.m_OnLeaveArea = OnLeave;
}

void ezVirtualThumbStick::SetInputArea(const ezVec2& vLowerLeft, const ezVec2& vUpperRight, float fThumbstickRadius, float fPriority, CenterMode::Enum center)
{
  m_bConfigChanged = true;

  m_vLowerLeft = vLowerLeft;
  m_vUpperRight = vUpperRight;
  m_fRadius = fThumbstickRadius;
  m_ActionConfig.m_fFilteredPriority = fPriority;
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

  m_ActionConfig.m_fFilterXMinValue = m_vLowerLeft.x;
  m_ActionConfig.m_fFilterXMaxValue = m_vUpperRight.x;
  m_ActionConfig.m_fFilterYMinValue = m_vLowerLeft.y;
  m_ActionConfig.m_fFilterYMaxValue = m_vUpperRight.y;

  ezInputManager::SetInputActionConfig(GetDynamicRTTI()->GetTypeName(), m_sName.GetData(), m_ActionConfig, false);

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
    ezInputManager::RemoveInputAction(GetDynamicRTTI()->GetTypeName(), m_sName.GetData());
    return;
  }

  UpdateActionMapping();

  float fValue;
  ezInt8 iTriggerAlt;

  const ezKeyState::Enum ks = ezInputManager::GetInputActionState(GetDynamicRTTI()->GetTypeName(), m_sName.GetData(), &fValue, &iTriggerAlt);
  
  if (ks != ezKeyState::Up)
  {
    m_bIsActive = true;

    ezVec2 vTouchPos(0.0f);

    ezInputManager::GetInputSlotState(m_ActionConfig.m_sFilterByInputSlotX[iTriggerAlt].GetData(), &vTouchPos.x);
    ezInputManager::GetInputSlotState(m_ActionConfig.m_sFilterByInputSlotY[iTriggerAlt].GetData(), &vTouchPos.y);

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


EZ_STATICLINK_FILE(Core, Core_Input_Implementation_VirtualThumbStick);

