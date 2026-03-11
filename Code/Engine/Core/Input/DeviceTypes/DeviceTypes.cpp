#include <Core/CorePCH.h>

#include <Core/Input/DeviceTypes/Controller.h>
#include <Core/Input/DeviceTypes/MouseKeyboard.h>
#include <Foundation/Time/Clock.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezInputDeviceMouseKeyboard, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezInputDeviceController, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezInputDevice* ezInputDeviceMouseKeyboard::s_pMouseOver = nullptr;

ezInputDeviceController::ezInputDeviceController()
{
  for (ezInt8 c = 0; c < MaxControllers; ++c)
  {
    m_bVibrationEnabled[c] = false;
    m_iPhysicalToVirtualControllerMapping[c] = 0; // by default, map all physical controllers to the first virtual controller
    m_RecentPhysicalControllerInput[c].Clear();

    for (ezInt8 m = 0; m < Motor::ENUM_COUNT; ++m)
    {
      m_fVibrationStrength[c][m] = 0.0f;

      for (ezUInt8 t = 0; t < MaxVibrationSamples; ++t)
        m_fVibrationTracks[c][m][t] = 0.0f;
    }
  }
}

void ezInputDeviceController::EnableVibration(ezUInt8 uiVirtual, bool bEnable)
{
  EZ_ASSERT_DEV(uiVirtual < MaxControllers, "Controller Index {0} is larger than allowed ({1}).", uiVirtual, MaxControllers);

  m_bVibrationEnabled[uiVirtual] = bEnable;
}

bool ezInputDeviceController::IsVibrationEnabled(ezUInt8 uiVirtual) const
{
  EZ_ASSERT_DEV(uiVirtual < MaxControllers, "Controller Index {0} is larger than allowed ({1}).", uiVirtual, MaxControllers);

  return m_bVibrationEnabled[uiVirtual];
}

void ezInputDeviceController::SetVibrationStrength(ezUInt8 uiVirtual, Motor::Enum motor, float fValue)
{
  EZ_ASSERT_DEV(uiVirtual < MaxControllers, "Controller Index {0} is larger than allowed ({1}).", uiVirtual, MaxControllers);
  EZ_ASSERT_DEV(motor < Motor::ENUM_COUNT, "Invalid Vibration Motor Index.");

  m_fVibrationStrength[uiVirtual][motor] = ezMath::Clamp(fValue, 0.0f, 1.0f);
}

float ezInputDeviceController::GetVibrationStrength(ezUInt8 uiVirtual, Motor::Enum motor)
{
  EZ_ASSERT_DEV(uiVirtual < MaxControllers, "Controller Index {0} is larger than allowed ({1}).", uiVirtual, MaxControllers);
  EZ_ASSERT_DEV(motor < Motor::ENUM_COUNT, "Invalid Vibration Motor Index.");

  return m_fVibrationStrength[uiVirtual][motor];
}

void ezInputDeviceController::SetPhysicalControllerMapping(ezUInt8 uiPhysicalController, ezInt8 iVirtualController)
{
  EZ_ASSERT_DEV(uiPhysicalController < MaxControllers, "Physical Controller Index {0} is larger than allowed ({1}).", uiPhysicalController, MaxControllers);
  EZ_ASSERT_DEV(iVirtualController < MaxControllers, "Virtual Controller Index {0} is larger than allowed ({1}).", iVirtualController, MaxControllers);

  m_iPhysicalToVirtualControllerMapping[uiPhysicalController] = iVirtualController;

  if (iVirtualController < 0)
  {
    ezLog::Dev("Input from physical controller {} got deactivated", uiPhysicalController);
  }
  else
  {
    ezLog::Dev("Mapped physical controller {} to virtual controller {}", uiPhysicalController, iVirtualController);
  }
}

ezInt8 ezInputDeviceController::GetPhysicalControllerMapping(ezUInt8 uiPhysical) const
{
  EZ_ASSERT_DEV(uiPhysical < MaxControllers, "Physical Controller Index {0} is larger than allowed ({1}).", uiPhysical, MaxControllers);

  return m_iPhysicalToVirtualControllerMapping[uiPhysical];
}

ezBitflags<ezPhysicalControllerInput> ezInputDeviceController::GetRecentPhysicalControllerInput(ezUInt8 uiPhysical) const
{
  EZ_ASSERT_DEV(uiPhysical < MaxControllers, "Physical Controller Index {0} is larger than allowed ({1}).", uiPhysical, MaxControllers);

  return m_RecentPhysicalControllerInput[uiPhysical];
}

void ezInputDeviceController::AddVibrationTrack(ezUInt8 uiVirtual, Motor::Enum motor, float* pVibrationTrackValue, ezUInt32 uiSamples, float fScalingFactor)
{
  uiSamples = ezMath::Min<ezUInt32>(uiSamples, MaxVibrationSamples);

  for (ezUInt32 s = 0; s < uiSamples; ++s)
  {
    float& fVal = m_fVibrationTracks[uiVirtual][motor][(m_uiVibrationTrackPos + 1 + s) % MaxVibrationSamples];

    fVal = ezMath::Max(fVal, pVibrationTrackValue[s] * fScalingFactor);
    fVal = ezMath::Clamp(fVal, 0.0f, 1.0f);
  }
}

void ezInputDeviceController::UpdateVibration(ezTime tTimeDifference)
{
  static ezTime tElapsedTime;
  tElapsedTime += tTimeDifference;

  const ezTime tTimePerSample = ezTime::MakeFromSeconds(1.0 / (double)VibrationSamplesPerSecond);

  // advance the vibration track sampling
  while (tElapsedTime >= tTimePerSample)
  {
    tElapsedTime -= tTimePerSample;

    for (ezUInt32 c = 0; c < MaxControllers; ++c)
    {
      for (ezUInt32 m = 0; m < Motor::ENUM_COUNT; ++m)
        m_fVibrationTracks[c][m][m_uiVibrationTrackPos] = 0.0f;
    }

    m_uiVibrationTrackPos = (m_uiVibrationTrackPos + 1) % MaxVibrationSamples;
  }

  // we will temporarily store how much vibration is to be applied on each physical controller
  float fVibrationToApply[MaxControllers][Motor::ENUM_COUNT];

  // Initialize with zero (we might not set all values later)
  for (ezUInt32 c = 0; c < MaxControllers; ++c)
  {
    for (ezUInt32 m = 0; m < Motor::ENUM_COUNT; ++m)
    {
      fVibrationToApply[c][m] = 0.0f;
    }
  }

  // go through all controllers and motors
  for (ezUInt8 c = 0; c < MaxControllers; ++c)
  {
    // ignore if vibration is disabled on this controller
    if (!m_bVibrationEnabled[c])
      continue;

    for (ezUInt8 p = 0; p < MaxControllers; ++p)
    {
      if (m_iPhysicalToVirtualControllerMapping[p] == c)
      {
        for (ezUInt32 m = 0; m < Motor::ENUM_COUNT; ++m)
        {
          fVibrationToApply[p][m] = ezMath::Max(m_fVibrationStrength[c][m], m_fVibrationTracks[c][m][m_uiVibrationTrackPos]);
        }
      }
    }
  }

  // now send the back-end all the information about how to vibrate which physical controller
  // this also always resets vibration to zero for controllers that might have been changed to another virtual controller etc.
  for (ezUInt8 c = 0; c < MaxControllers; ++c)
  {
    for (ezUInt32 m = 0; m < Motor::ENUM_COUNT; ++m)
    {
      ApplyVibration(c, (Motor::Enum)m, fVibrationToApply[c][m]);
    }
  }
}

void ezInputDeviceMouseKeyboard::UpdateInputSlotValues()
{
  const char* slots[3] = {ezInputSlot_MouseButton0, ezInputSlot_MouseButton1, ezInputSlot_MouseButton2};
  const char* dlbSlots[3] = {ezInputSlot_MouseDblClick0, ezInputSlot_MouseDblClick1, ezInputSlot_MouseDblClick2};

  const ezTime tNow = ezClock::GetGlobalClock()->GetLastUpdateTime();

  for (int i = 0; i < 3; ++i)
  {
    m_InputSlotValues[dlbSlots[i]] = 0.0f;

    const bool bDown = m_InputSlotValues[slots[i]] > 0;
    if (bDown)
    {
      if (!m_bMouseDown[i])
      {
        if (tNow - m_LastMouseClick[i] <= m_DoubleClickTime)
        {
          m_InputSlotValues[dlbSlots[i]] = 1.0f;
          m_LastMouseClick[i] = ezTime::MakeZero(); // this prevents triple-clicks from appearing as two double clicks
        }
        else
        {
          m_LastMouseClick[i] = tNow;
        }
      }
    }

    m_bMouseDown[i] = bDown;
  }
}

EZ_STATICLINK_FILE(Core, Core_Input_DeviceTypes_DeviceTypes);
