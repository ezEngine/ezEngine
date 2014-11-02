#include <Core/PCH.h>
#include <Foundation/Logging/Log.h>
#include <Core/Input/InputDevice.h>
#include <Core/Input/InputManager.h>

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezInputDevice);

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezInputDevice, ezReflectedClass, 1, ezRTTINoAllocator);
  // no properties or message handlers
EZ_END_DYNAMIC_REFLECTED_TYPE();

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

  EZ_LOG_BLOCK("Initializing Input Device", GetDynamicRTTI()->GetTypeName());

  ezLog::Info("Input Device Type: %s, Device Name: %s", GetDynamicRTTI()->GetParentType()->GetTypeName(), GetDynamicRTTI()->GetTypeName());

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

ezUInt32 ezInputDevice::RetrieveLastCharacter()
{ 
  ezUInt32 Temp = m_LastCharacter;
  m_LastCharacter = L'\0';
  return Temp;
}

ezUInt32 ezInputDevice::RetrieveLastCharacterFromAllDevices()
{
  for (ezInputDevice* pDevice = ezInputDevice::GetFirstInstance(); pDevice != nullptr; pDevice = pDevice->GetNextInstance())
  {
    const ezUInt32 Char = pDevice->RetrieveLastCharacter();

    if (Char != L'\0')
      return Char;
  }

  return '\0';
}



EZ_STATICLINK_FILE(Core, Core_Input_Implementation_InputDevice);

