#include <Core/PCH.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringBuilder.h>
#include <InputXBox360/InputDeviceXBox.h>
#include <Core/Input/InputManager.h>
#include <Xinput.h>

ezInputDeviceXBox360::ezInputDeviceXBox360()
{
  for (ezInt32 i = 0; i < 4; ++i)
  {
    m_bControllerConnected[i] = false;
    m_iMapControllerTo[i] = i;
    m_bEnableVibration[i] = false;
  }
}

void ezInputDeviceXBox360::RegisterControllerButton(const char* szButton, const char* szName, ezBitflags<ezInputSlotFlags> SlotFlags)
{
  ezStringBuilder s, s2;

  for (ezInt32 i = 0; i < 4; ++i)
  {
    s.Format("controller%i_%s", i, szButton);
    s2.Format("Cont %i: %s", i + 1, szName);
    RegisterInputSlot(s.GetData(), s2.GetData(), SlotFlags);
  }

  for (ezInt32 i = 0; i < 4; ++i)
  {
    s.Format("controller%i_%s", i, szButton);
    ezInputManager::SetInputSlotDeadZone(s.GetData(), 0.23f);
    ezInputManager::SetInputSlotScale(s.GetData(), -3.0f);
  }
}

void ezInputDeviceXBox360::SetDeadZoneAndScale(const char* szButton)
{
  ezStringBuilder s;

  for (ezInt32 i = 0; i < 4; ++i)
  {
    s.Format("controller%i_%s", i, szButton);
    ezInputManager::SetInputSlotDeadZone(s.GetData(), 0.23f);
    ezInputManager::SetInputSlotScale(s.GetData(), -3.0f);
  }
}

void ezInputDeviceXBox360::RegisterInputSlots()
{
  RegisterControllerButton("button_a", "Button A", ezInputSlotFlags::IsButton);
  RegisterControllerButton("button_b", "Button B", ezInputSlotFlags::IsButton);
  RegisterControllerButton("button_x", "Button X", ezInputSlotFlags::IsButton);
  RegisterControllerButton("button_y", "Button Y", ezInputSlotFlags::IsButton);
  RegisterControllerButton("button_start", "Start", ezInputSlotFlags::IsButton);
  RegisterControllerButton("button_back", "Back", ezInputSlotFlags::IsButton);
  RegisterControllerButton("left_shoulder", "Left Shoulder", ezInputSlotFlags::IsButton);
  RegisterControllerButton("right_shoulder", "Right Shoulder", ezInputSlotFlags::IsButton);
  RegisterControllerButton("left_trigger", "Left Trigger", ezInputSlotFlags::IsAnalogTrigger);
  RegisterControllerButton("right_trigger", "Right Trigger", ezInputSlotFlags::IsAnalogTrigger);
  RegisterControllerButton("pad_up", "Pad Up", ezInputSlotFlags::IsButton);
  RegisterControllerButton("pad_down", "Pad Down", ezInputSlotFlags::IsButton);
  RegisterControllerButton("pad_left", "Pad Left", ezInputSlotFlags::IsButton);
  RegisterControllerButton("pad_right", "Pad Right", ezInputSlotFlags::IsButton);
  RegisterControllerButton("left_stick", "Left Stick", ezInputSlotFlags::IsButton);
  RegisterControllerButton("right_stick", "Right Stick", ezInputSlotFlags::IsButton);

  RegisterControllerButton("leftstick_negx", "Left Stick Left", ezInputSlotFlags::IsAnalogStick);
  RegisterControllerButton("leftstick_posx", "Left Stick Right", ezInputSlotFlags::IsAnalogStick);
  RegisterControllerButton("leftstick_negy", "Left Stick Down", ezInputSlotFlags::IsAnalogStick);
  RegisterControllerButton("leftstick_posy", "Left Stick Up", ezInputSlotFlags::IsAnalogStick);

  RegisterControllerButton("rightstick_negx", "Right Stick Left", ezInputSlotFlags::IsAnalogStick);
  RegisterControllerButton("rightstick_posx", "Right Stick Right", ezInputSlotFlags::IsAnalogStick);
  RegisterControllerButton("rightstick_negy", "Right Stick Down", ezInputSlotFlags::IsAnalogStick);
  RegisterControllerButton("rightstick_posy", "Right Stick Up", ezInputSlotFlags::IsAnalogStick);

  SetDeadZoneAndScale("left_trigger");
  SetDeadZoneAndScale("right_trigger");
  SetDeadZoneAndScale("left_stick_negx");
  SetDeadZoneAndScale("left_stick_posx");
  SetDeadZoneAndScale("left_stick_negy");
  SetDeadZoneAndScale("left_stick_posy");
  SetDeadZoneAndScale("right_stick_negx");
  SetDeadZoneAndScale("right_stick_posx");
  SetDeadZoneAndScale("right_stick_negy");
  SetDeadZoneAndScale("right_stick_posy");

  ezLog::Success("Initialized XBox 360 Controller.");
}

void ezInputDeviceXBox360::SetValue(ezInt32 iController, const char* szButton, float fValue)
{
  ezStringBuilder s;
  s.Format("controller%i_%s", iController, szButton);
  float& fVal = m_InputSlotValues[s.GetData()];
  fVal = ezMath::Max(fVal, fValue);
}

void ezInputDeviceXBox360::UpdateInputSlotValues()
{
  // reset all keys
  for (ezMap<ezString, float, ezCompareHelper<ezString>, ezStaticAllocatorWrapper>::Iterator it = m_InputSlotValues.GetIterator(); it.IsValid(); ++it)
    it.Value() = 0.0f;

  for (ezInt32 iController = 0; iController < 4; ++iController)
  {
    XINPUT_STATE State;
    ezMemoryUtils::ZeroFill(&State);

    const bool bIsAvailable = (XInputGetState(iController, &State) == ERROR_SUCCESS);

    if (m_bControllerConnected[iController] != bIsAvailable)
    {
      ezLog::Info("XBox Controller %i has been %s.", iController, bIsAvailable ? "connected" : "disconnected");

      m_bControllerConnected[iController] = bIsAvailable;
    }

    if (!bIsAvailable)
      continue;

    const ezInt32 iMapTo = m_iMapControllerTo[iController];

    if ((iMapTo < 0) || (iMapTo > 3))
      continue;

    SetValue(iMapTo, "pad_up"                , ((State.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP)        != 0) ? 1.0f : 0.0f);
    SetValue(iMapTo, "pad_down"              , ((State.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN)      != 0) ? 1.0f : 0.0f);
    SetValue(iMapTo, "pad_left"              , ((State.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)      != 0) ? 1.0f : 0.0f);
    SetValue(iMapTo, "pad_right"             , ((State.Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)     != 0) ? 1.0f : 0.0f);
    SetValue(iMapTo, "button_start"          , ((State.Gamepad.wButtons & XINPUT_GAMEPAD_START)          != 0) ? 1.0f : 0.0f);
    SetValue(iMapTo, "button_back"           , ((State.Gamepad.wButtons & XINPUT_GAMEPAD_BACK)           != 0) ? 1.0f : 0.0f);
    SetValue(iMapTo, "left_stick"            , ((State.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB)     != 0) ? 1.0f : 0.0f);
    SetValue(iMapTo, "right_stick"           , ((State.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB)    != 0) ? 1.0f : 0.0f);
    SetValue(iMapTo, "left_shoulder"         , ((State.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)  != 0) ? 1.0f : 0.0f);
    SetValue(iMapTo, "right_shoulder"        , ((State.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0) ? 1.0f : 0.0f);
    SetValue(iMapTo, "button_a"              , ((State.Gamepad.wButtons & XINPUT_GAMEPAD_A)              != 0) ? 1.0f : 0.0f);
    SetValue(iMapTo, "button_b"              , ((State.Gamepad.wButtons & XINPUT_GAMEPAD_B)              != 0) ? 1.0f : 0.0f);
    SetValue(iMapTo, "button_x"              , ((State.Gamepad.wButtons & XINPUT_GAMEPAD_X)              != 0) ? 1.0f : 0.0f);
    SetValue(iMapTo, "button_y"              , ((State.Gamepad.wButtons & XINPUT_GAMEPAD_Y)              != 0) ? 1.0f : 0.0f);

    const float fTriggerRange = 255.0f;

    SetValue(iMapTo, "left_trigger",  State.Gamepad.bLeftTrigger  / fTriggerRange);
    SetValue(iMapTo, "right_trigger", State.Gamepad.bRightTrigger / fTriggerRange);

    // all input points have dead-zones, so we can let the state handler do the rest
    SetValue(iMapTo, "leftstick_negx", (State.Gamepad.sThumbLX < 0) ? (-State.Gamepad.sThumbLX / 32767.0f) : 0.0f);
    SetValue(iMapTo, "leftstick_posx", (State.Gamepad.sThumbLX > 0) ? ( State.Gamepad.sThumbLX / 32767.0f) : 0.0f);
    SetValue(iMapTo, "leftstick_negy", (State.Gamepad.sThumbLY < 0) ? (-State.Gamepad.sThumbLY / 32767.0f) : 0.0f);
    SetValue(iMapTo, "leftstick_posy", (State.Gamepad.sThumbLY > 0) ? ( State.Gamepad.sThumbLY / 32767.0f) : 0.0f);

    SetValue(iMapTo, "rightstick_negx", (State.Gamepad.sThumbRX < 0) ? (-State.Gamepad.sThumbRX / 32767.0f) : 0.0f);
    SetValue(iMapTo, "rightstick_posx", (State.Gamepad.sThumbRX > 0) ? ( State.Gamepad.sThumbRX / 32767.0f) : 0.0f);
    SetValue(iMapTo, "rightstick_negy", (State.Gamepad.sThumbRY < 0) ? (-State.Gamepad.sThumbRY / 32767.0f) : 0.0f);
    SetValue(iMapTo, "rightstick_posy", (State.Gamepad.sThumbRY > 0) ? ( State.Gamepad.sThumbRY / 32767.0f) : 0.0f);
  }
}


