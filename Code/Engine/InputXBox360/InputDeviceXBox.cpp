#include <InputXBox360/PCH.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringBuilder.h>
#include <InputXBox360/InputDeviceXBox.h>
#include <Core/Input/InputManager.h>
#include <Foundation/Time/Time.h>
#include <Xinput.h>


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezInputDeviceXBox360, ezInputDeviceController, 1, ezRTTINoAllocator);
  // no properties or message handlers
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezInputDeviceXBox360::ezInputDeviceXBox360()
{
  for (ezInt32 i = 0; i < MaxControllers; ++i)
    m_bControllerConnected[i] = false;
}

void ezInputDeviceXBox360::RegisterControllerButton(const char* szButton, const char* szName, ezBitflags<ezInputSlotFlags> SlotFlags)
{
  ezStringBuilder s, s2;

  for (ezInt32 i = 0; i < MaxControllers; ++i)
  {
    s.Format("controller%i_%s", i, szButton);
    s2.Format("Cont %i: %s", i + 1, szName);
    RegisterInputSlot(s.GetData(), s2.GetData(), SlotFlags);
  }
}

void ezInputDeviceXBox360::SetDeadZone(const char* szButton)
{
  ezStringBuilder s;

  for (ezInt32 i = 0; i < MaxControllers; ++i)
  {
    s.Format("controller%i_%s", i, szButton);
    ezInputManager::SetInputSlotDeadZone(s.GetData(), 0.23f);
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
  RegisterControllerButton("pad_up", "Pad Up", ezInputSlotFlags::IsDPad);
  RegisterControllerButton("pad_down", "Pad Down", ezInputSlotFlags::IsDPad);
  RegisterControllerButton("pad_left", "Pad Left", ezInputSlotFlags::IsDPad);
  RegisterControllerButton("pad_right", "Pad Right", ezInputSlotFlags::IsDPad);
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

  SetDeadZone("left_trigger");
  SetDeadZone("right_trigger");
  SetDeadZone("leftstick_negx");
  SetDeadZone("leftstick_posx");
  SetDeadZone("leftstick_negy");
  SetDeadZone("leftstick_posy");
  SetDeadZone("rightstick_negx");
  SetDeadZone("rightstick_posx");
  SetDeadZone("rightstick_negy");
  SetDeadZone("rightstick_posy");

  ezLog::Success("Initialized XBox 360 Controller.");
}

const char* szControllerName[] = 
{
  "controller0_",
  "controller1_",
  "controller2_",
  "controller3_",

  "controller4_",
  "controller5_",
  "controller6_",
  "controller7_",
};

EZ_CHECK_AT_COMPILETIME(EZ_ARRAY_SIZE(szControllerName) >= ezInputDeviceXBox360::MaxControllers);

void ezInputDeviceXBox360::SetValue(ezInt32 iController, const char* szButton, float fValue)
{
  ezStringBuilder s = szControllerName[iController];
  s.Append(szButton);
  float& fVal = m_InputSlotValues[s];
  fVal = ezMath::Max(fVal, fValue);
}

void ezInputDeviceXBox360::UpdateHardwareState(ezTime tTimeDifference)
{
  UpdateVibration(tTimeDifference);
}

void ezInputDeviceXBox360::UpdateInputSlotValues()
{
  // reset all keys
  for (auto it = m_InputSlotValues.GetIterator(); it.IsValid(); ++it)
    it.Value() = 0.0f;

  XINPUT_STATE State[MaxControllers];
  bool bIsAvailable[MaxControllers];

  // update not connected controllers only every few milliseconds, apparently it takes quite some time to do this
  // even on not connected controllers
  static ezTime tLastControllerSearch;
  const ezTime tNow = ezTime::Now();
  const bool bSearchControllers = tNow - tLastControllerSearch > ezTime::Seconds(0.5);

  if (bSearchControllers)
    tLastControllerSearch = tNow;

  // get the data from all physical devices
  for (ezInt32 iPhysical = 0; iPhysical < MaxControllers; ++iPhysical)
  {
    if (bSearchControllers || m_bControllerConnected[iPhysical])
    {
      bIsAvailable[iPhysical] = (XInputGetState(iPhysical, &State[iPhysical]) == ERROR_SUCCESS);

      if (m_bControllerConnected[iPhysical] != bIsAvailable[iPhysical])
      {
        ezLog::Info("XBox Controller %i has been %s.", iPhysical, bIsAvailable ? "connected" : "disconnected");

        // this makes sure to reset all values below
        if (!bIsAvailable)
          ezMemoryUtils::ZeroFill(&State[iPhysical]);
      }
    }
    else
      bIsAvailable[iPhysical] = m_bControllerConnected[iPhysical];
  }

  // now update all virtual controllers
  for (ezInt32 iVirtual = 0; iVirtual < MaxControllers; ++iVirtual)
  {
    // check from which physical device to take the input data
    const ezInt32 iPhysical = GetControllerMapping(iVirtual);

    // if the mapping is negative (which means 'deactivated'), ignore this controller
    if ((iPhysical < 0) || (iPhysical >= MaxControllers))
      continue;

    // if the controller is not active, no point in updating it
    // if it just got inactive, this will reset it once, because the state is only passed on after this loop
    if (!m_bControllerConnected[iPhysical])
      continue;

    SetValue(iVirtual, "pad_up"                , ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP)        != 0) ? 1.0f : 0.0f);
    SetValue(iVirtual, "pad_down"              , ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN)      != 0) ? 1.0f : 0.0f);
    SetValue(iVirtual, "pad_left"              , ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)      != 0) ? 1.0f : 0.0f);
    SetValue(iVirtual, "pad_right"             , ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)     != 0) ? 1.0f : 0.0f);
    SetValue(iVirtual, "button_start"          , ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_START)          != 0) ? 1.0f : 0.0f);
    SetValue(iVirtual, "button_back"           , ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_BACK)           != 0) ? 1.0f : 0.0f);
    SetValue(iVirtual, "left_stick"            , ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB)     != 0) ? 1.0f : 0.0f);
    SetValue(iVirtual, "right_stick"           , ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB)    != 0) ? 1.0f : 0.0f);
    SetValue(iVirtual, "left_shoulder"         , ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)  != 0) ? 1.0f : 0.0f);
    SetValue(iVirtual, "right_shoulder"        , ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0) ? 1.0f : 0.0f);
    SetValue(iVirtual, "button_a"              , ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_A)              != 0) ? 1.0f : 0.0f);
    SetValue(iVirtual, "button_b"              , ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_B)              != 0) ? 1.0f : 0.0f);
    SetValue(iVirtual, "button_x"              , ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_X)              != 0) ? 1.0f : 0.0f);
    SetValue(iVirtual, "button_y"              , ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_Y)              != 0) ? 1.0f : 0.0f);

    const float fTriggerRange = 255.0f;

    SetValue(iVirtual, "left_trigger",     State[iPhysical].Gamepad.bLeftTrigger  / fTriggerRange);
    SetValue(iVirtual, "right_trigger",    State[iPhysical].Gamepad.bRightTrigger / fTriggerRange);

    // all input points have dead-zones, so we can let the state handler do the rest
    SetValue(iVirtual, "leftstick_negx",  (State[iPhysical].Gamepad.sThumbLX < 0) ? (-State[iPhysical].Gamepad.sThumbLX / 32767.0f) : 0.0f);
    SetValue(iVirtual, "leftstick_posx",  (State[iPhysical].Gamepad.sThumbLX > 0) ? ( State[iPhysical].Gamepad.sThumbLX / 32767.0f) : 0.0f);
    SetValue(iVirtual, "leftstick_negy",  (State[iPhysical].Gamepad.sThumbLY < 0) ? (-State[iPhysical].Gamepad.sThumbLY / 32767.0f) : 0.0f);
    SetValue(iVirtual, "leftstick_posy",  (State[iPhysical].Gamepad.sThumbLY > 0) ? ( State[iPhysical].Gamepad.sThumbLY / 32767.0f) : 0.0f);

    SetValue(iVirtual, "rightstick_negx", (State[iPhysical].Gamepad.sThumbRX < 0) ? (-State[iPhysical].Gamepad.sThumbRX / 32767.0f) : 0.0f);
    SetValue(iVirtual, "rightstick_posx", (State[iPhysical].Gamepad.sThumbRX > 0) ? ( State[iPhysical].Gamepad.sThumbRX / 32767.0f) : 0.0f);
    SetValue(iVirtual, "rightstick_negy", (State[iPhysical].Gamepad.sThumbRY < 0) ? (-State[iPhysical].Gamepad.sThumbRY / 32767.0f) : 0.0f);
    SetValue(iVirtual, "rightstick_posy", (State[iPhysical].Gamepad.sThumbRY > 0) ? ( State[iPhysical].Gamepad.sThumbRY / 32767.0f) : 0.0f);
  }

  for (ezInt32 iPhysical = 0; iPhysical < MaxControllers; ++iPhysical)
    m_bControllerConnected[iPhysical] = bIsAvailable[iPhysical];
}

bool ezInputDeviceXBox360::IsControllerConnected(ezUInt8 uiPhysical) const
{ 
  EZ_ASSERT_DEV(uiPhysical < MaxControllers, "Invalid Controller Index %i", uiPhysical);
  
  return m_bControllerConnected[uiPhysical];
}

void ezInputDeviceXBox360::ApplyVibration(ezUInt8 uiPhysicalController, Motor::Enum eMotor, float fStrength)
{
  if (!m_bControllerConnected[uiPhysicalController])
    return;

  static XINPUT_VIBRATION v[MaxControllers];

  if (eMotor == Motor::LeftMotor)
    v[uiPhysicalController].wLeftMotorSpeed = (WORD) (fStrength * 65535.0f);

  if (eMotor == Motor::RightMotor)
  {
    v[uiPhysicalController].wRightMotorSpeed = (WORD) (fStrength * 65535.0f);

    XInputSetState(uiPhysicalController, &v[uiPhysicalController]);
  }
}




EZ_STATICLINK_FILE(InputXBox360, InputXBox360_InputDeviceXBox);

