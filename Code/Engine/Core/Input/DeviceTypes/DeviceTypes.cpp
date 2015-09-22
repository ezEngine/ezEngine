#include <Core/PCH.h>
#include <Core/Input/DeviceTypes/Controller.h>
#include <Core/Input/DeviceTypes/MouseKeyboard.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezInputDeviceMouseKeyboard, ezInputDevice, 1, ezRTTINoAllocator);
  // no properties or message handlers
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezInputDeviceController, ezInputDevice, 1, ezRTTINoAllocator);
  // no properties or message handlers
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezInputDeviceController::ezInputDeviceController()
{
  m_uiVibrationTrackPos = 0;

  for (ezInt32 c = 0; c < MaxControllers; ++c)
  {
    m_bVibrationEnabled[c] = false;
    m_iControllerMapping[c] = c;

    for (ezInt32 m = 0; m < Motor::ENUM_COUNT; ++m)
    {
      m_fVibrationStrength[c][m] = 0.0f;

      for (ezUInt32 t = 0; t < MaxVibrationSamples; ++t)
        m_fVibrationTracks[c][m][t] = 0.0f;
    }
  }
}

void ezInputDeviceController::EnableVibration(ezUInt8 uiVirtual, bool bEnable)
{
  EZ_ASSERT_DEV(uiVirtual < MaxControllers, "Controller Index %i is larger than allowed (%i).", uiVirtual, MaxControllers);

  m_bVibrationEnabled[uiVirtual] = bEnable;
}

bool ezInputDeviceController::IsVibrationEnabled(ezUInt8 uiVirtual) const
{
  EZ_ASSERT_DEV(uiVirtual < MaxControllers, "Controller Index %i is larger than allowed (%i).", uiVirtual, MaxControllers);

  return m_bVibrationEnabled[uiVirtual];
}

void ezInputDeviceController::SetVibrationStrength(ezUInt8 uiVirtual, Motor::Enum eMotor, float fValue)
{
  EZ_ASSERT_DEV(uiVirtual < MaxControllers, "Controller Index %i is larger than allowed (%i).", uiVirtual, MaxControllers);
  EZ_ASSERT_DEV(eMotor < Motor::ENUM_COUNT, "Invalid Vibration Motor Index.");

  m_fVibrationStrength[uiVirtual][eMotor] = ezMath::Clamp(fValue, 0.0f, 1.0f);
}

float ezInputDeviceController::GetVibrationStrength(ezUInt8 uiVirtual, Motor::Enum eMotor)
{
  EZ_ASSERT_DEV(uiVirtual < MaxControllers, "Controller Index %i is larger than allowed (%i).", uiVirtual, MaxControllers);
  EZ_ASSERT_DEV(eMotor < Motor::ENUM_COUNT, "Invalid Vibration Motor Index.");

  return m_fVibrationStrength[uiVirtual][eMotor];
}

void ezInputDeviceController::SetControllerMapping(ezUInt8 uiVirtualController, ezInt8 iTakeInputFromPhysical)
{
  EZ_ASSERT_DEV(uiVirtualController < MaxControllers, "Virtual Controller Index %i is larger than allowed (%i).", uiVirtualController, MaxControllers);
  EZ_ASSERT_DEV(iTakeInputFromPhysical < MaxControllers, "Physical Controller Index %i is larger than allowed (%i).", iTakeInputFromPhysical, MaxControllers);

  if (iTakeInputFromPhysical < 0)
  {
    // deactivates this virtual controller
    m_iControllerMapping[uiVirtualController] = -1;
  }
  else
  {
    // if any virtual controller already maps to the given physical controller, let it use the physical controller that
    // uiVirtualController is currently mapped to
    for (ezInt32 c = 0; c < MaxControllers; ++c)
    {
      if (m_iControllerMapping[c] == iTakeInputFromPhysical)
      {
        m_iControllerMapping[c] = m_iControllerMapping[uiVirtualController];
        break;
      }
    }

    m_iControllerMapping[uiVirtualController] = iTakeInputFromPhysical;
  }
}

ezInt8 ezInputDeviceController::GetControllerMapping(ezUInt8 uiVirtual) const
{
  EZ_ASSERT_DEV(uiVirtual < MaxControllers, "Virtual Controller Index %i is larger than allowed (%i).", uiVirtual, MaxControllers);

  return m_iControllerMapping[uiVirtual];
}

void ezInputDeviceController::AddVibrationTrack(ezUInt8 uiVirtual, Motor::Enum eMotor, float* fVibrationTrackValue, ezUInt32 uiSamples, float fScalingFactor)
{
  uiSamples = ezMath::Min<ezUInt32>(uiSamples, MaxVibrationSamples);

  for (ezUInt32 s = 0; s < uiSamples; ++s)
  {
    float& fVal = m_fVibrationTracks[uiVirtual][eMotor][(m_uiVibrationTrackPos + 1 + s) % MaxVibrationSamples];

    fVal = ezMath::Max(fVal, fVibrationTrackValue[s] * fScalingFactor);
    fVal = ezMath::Clamp(fVal, 0.0f, 1.0f);
  }
}

void ezInputDeviceController::UpdateVibration(ezTime tTimeDifference)
{
  static ezTime tElapsedTime;
  tElapsedTime += tTimeDifference;

  const ezTime tTimePerSample = ezTime::Seconds(1.0 / VibrationSamplesPerSecond);

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
      fVibrationToApply[c][m] = 0.0f;
  }

  // go through all controllers and motors
  for (ezUInt32 c = 0; c < MaxControllers; ++c)
  {
    // ignore if vibration is disabled on this controller
    if (!m_bVibrationEnabled[c])
      continue;

    // check which physical controller this virtual controller is attached to
    const ezInt8 iPhysical = GetControllerMapping(c);

    // if it is attached to any physical controller, store the vibration value
    if (iPhysical >= 0)
    {
      for (ezUInt32 m = 0; m < Motor::ENUM_COUNT; ++m)
        fVibrationToApply[(ezUInt8)iPhysical][m] = ezMath::Max(m_fVibrationStrength[c][m], m_fVibrationTracks[c][m][m_uiVibrationTrackPos]);
    }
  }

  // now send the back-end all the information about how to vibrate which physical controller
  // this also always resets vibration to zero for controllers that might have been changed to another virtual controller etc.
  for (ezUInt32 c = 0; c < MaxControllers; ++c)
  {
    for (ezUInt32 m = 0; m < Motor::ENUM_COUNT; ++m)
      ApplyVibration(c, (Motor::Enum) m, fVibrationToApply[c][m]);
  }
}


EZ_STATICLINK_FILE(Core, Core_Input_DeviceTypes_DeviceTypes);

