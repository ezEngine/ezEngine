#include <Core/CorePCH.h>

#include <Core/Input/InputManager.h>

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezInputDevice);

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezInputDevice, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezKeyState::Enum ezKeyState::GetNewKeyState(ezKeyState::Enum prevState, bool bKeyDown)
{
  switch (prevState)
  {
    case ezKeyState::Down:
    case ezKeyState::Pressed:
      return bKeyDown ? ezKeyState::Down : ezKeyState::Released;
    case ezKeyState::Released:
    case ezKeyState::Up:
      return bKeyDown ? ezKeyState::Pressed : ezKeyState::Up;
  }

  return ezKeyState::Up;
}

ezInputDevice::ezInputDevice()
{
  m_bInitialized = false;
}

void ezInputDevice::RegisterInputSlot(ezStringView sName, ezStringView sDefaultDisplayName, ezBitflags<ezInputSlotFlags> SlotFlags)
{
  ezInputManager::RegisterInputSlot(sName, sDefaultDisplayName, SlotFlags);
}

void ezInputDevice::Initialize()
{
  if (m_bInitialized)
    return;

  EZ_LOG_BLOCK("Initializing Input Device", GetDynamicRTTI()->GetTypeName());

  ezLog::Dev("Input Device Type: {0}, Device Name: {1}", GetDynamicRTTI()->GetParentType()->GetTypeName(), GetDynamicRTTI()->GetTypeName());

  m_bInitialized = true;

  RegisterInputSlots();
  InitializeDevice();
}


void ezInputDevice::UpdateAllHardwareStates(ezTime tTimeDifference)
{
  // tell each device to update its hardware
  for (ezInputDevice* pDevice = ezInputDevice::GetFirstInstance(); pDevice != nullptr; pDevice = pDevice->GetNextInstance())
  {
    pDevice->UpdateHardwareState(tTimeDifference);
  }
}

void ezInputDevice::UpdateAllDevices()
{
  // tell each device to update its current input slot values
  for (ezInputDevice* pDevice = ezInputDevice::GetFirstInstance(); pDevice != nullptr; pDevice = pDevice->GetNextInstance())
  {
    pDevice->Initialize();
    pDevice->UpdateInputSlotValues();
  }
}

void ezInputDevice::ResetAllDevices()
{
  // tell all devices that the input update is through and they might need to reset some values now
  // this is especially important for device types that will get input messages at some undefined time after this call
  // but not during 'UpdateInputSlotValues'
  for (ezInputDevice* pDevice = ezInputDevice::GetFirstInstance(); pDevice != nullptr; pDevice = pDevice->GetNextInstance())
  {
    pDevice->ResetInputSlotValues();
  }
}

ezString ezInputDevice::RetrieveLastCharacters()
{
  ezString sResult = m_sLastCharacters;
  m_sLastCharacters.Clear();
  return sResult;
}

ezString ezInputDevice::RetrieveLastCharactersFromAllDevices()
{
  for (ezInputDevice* pDevice = ezInputDevice::GetFirstInstance(); pDevice != nullptr; pDevice = pDevice->GetNextInstance())
  {
    ezString sChars = pDevice->RetrieveLastCharacters();

    if (!sChars.IsEmpty())
      return sChars;
  }

  return ezString();
}

float ezInputDevice::GetInputSlotState(ezStringView sSlot) const
{
  return m_InputSlotValues.GetValueOrDefault(sSlot, 0.f);
}

bool ezInputDevice::HasDeviceBeenUsedLastFrame() const
{
  return m_bGeneratedInputRecently;
}

EZ_STATICLINK_FILE(Core, Core_Input_Implementation_InputDevice);
