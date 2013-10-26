#include <Core/PCH.h>
#include <Foundation/Logging/Log.h>
#include <Core/Input/InputDevice.h>
#include <Core/Input/InputManager.h>

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezInputDevice);

ezKeyState::Enum ezKeyState::GetNewKeyState(ezKeyState::Enum PrevState, bool bKeyDown)
{
  switch (PrevState)
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
  m_LastCharacter = '\0';
}

void ezInputDevice::RegisterInputSlot(const char* szName, const char* szDefaultDisplayName, ezBitflags<ezInputSlotFlags> SlotFlags)
{
  ezInputManager::RegisterInputSlot(szName, szDefaultDisplayName, SlotFlags);
}

void ezInputDevice::Initialize()
{
  if (m_bInitialized)
    return;

  EZ_LOG_BLOCK("Initializing Input Device", GetDeviceName());

  ezLog::Info("Input Device Type: %s, Device Name: %s", GetDeviceType(), GetDeviceName());

  m_bInitialized = true;

  RegisterInputSlots();
  InitializeDevice();
}


void ezInputDevice::UpdateAllHardwareStates(ezTime tTimeDifference)
{
  // tell each device to update its hardware
  for (ezInputDevice* pDevice = ezInputDevice::GetFirstInstance(); pDevice != NULL; pDevice = pDevice->GetNextInstance())
  {
    pDevice->UpdateHardwareState(tTimeDifference);
  }
}

void ezInputDevice::UpdateAllDevices()
{
  // tell each device to update its current input slot values
  for (ezInputDevice* pDevice = ezInputDevice::GetFirstInstance(); pDevice != NULL; pDevice = pDevice->GetNextInstance())
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
  for (ezInputDevice* pDevice = ezInputDevice::GetFirstInstance(); pDevice != NULL; pDevice = pDevice->GetNextInstance())
  {
    pDevice->ResetInputSlotValues();
  }
}

wchar_t ezInputDevice::RetrieveLastCharacter()
{ 
  wchar_t Temp = m_LastCharacter;
  m_LastCharacter = L'\0';
  return Temp;
}

wchar_t ezInputDevice::RetrieveLastCharacterFromAllDevices()
{
  for (ezInputDevice* pDevice = ezInputDevice::GetFirstInstance(); pDevice != NULL; pDevice = pDevice->GetNextInstance())
  {
    const wchar_t Char = pDevice->RetrieveLastCharacter();

    if (Char != L'\0')
      return Char;
  }

  return '\0';
}

