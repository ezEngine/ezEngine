#include <Core/PCH.h>
#include <Core/Input/InputManager.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Configuration/Startup.h>

ezMap<ezString, ezInputManager::ezInputSlot, ezCompareHelper<ezString>, ezStaticAllocatorWrapper> ezInputManager::s_InputSlots;
wchar_t ezInputManager::s_LastCharacter = '\0';

ezInputManager::ezInputSlot::ezInputSlot()
{
  m_fValue = 0.0f;
  m_State = ezKeyState::Up;
  m_fDeadZone = 0.0f;
  m_fScale = 1.0f;
}

void ezInputManager::RegisterInputSlot(const char* szInputSlot, const char* szDefaultDisplayName)
{
  ezMap<ezString, ezInputSlot>::ConstIterator it = s_InputSlots.Find(szInputSlot);

  // If the key already exists, but key and display string are identical, than overwrite the display string with the incoming string
  if (it.IsValid() && (it.Value().m_sDisplayName != it.Key()))
    return;

  s_InputSlots[szInputSlot].m_sDisplayName = szDefaultDisplayName;
}

void ezInputManager::SetInputSlotDisplayName(const char* szInputSlot, const char* szDefaultDisplayName)
{
  s_InputSlots[szInputSlot].m_sDisplayName = szDefaultDisplayName;
}

const char* ezInputManager::GetInputSlotDisplayName(const char* szInputSlot)
{
  ezMap<ezString, ezInputSlot>::ConstIterator it = s_InputSlots.Find(szInputSlot);

  if (it.IsValid())
    return it.Value().m_sDisplayName.GetData();

  return szInputSlot;
}

ezInputDevice* ezInputManager::GetInputDeviceByType(const char* szType)
{
  ezInputDevice* pDevice = ezInputDevice::GetFirstInstance();

  while (pDevice)
  {
    if (ezStringUtils::IsEqual_NoCase(pDevice->GetDeviceType(), szType))
      return pDevice;

    pDevice = pDevice->GetNextInstance();
  }

  return NULL;
}

ezInputDevice* ezInputManager::GetInputDeviceByName(const char* szName)
{
  ezInputDevice* pDevice = ezInputDevice::GetFirstInstance();

  while (pDevice)
  {
    if (ezStringUtils::IsEqual_NoCase(pDevice->GetDeviceName(), szName))
      return pDevice;

    pDevice = pDevice->GetNextInstance();
  }

  return NULL;
}

void ezInputManager::SetInputSlotDeadZone(const char* szInputSlot, float fDeadZone)
{
  RegisterInputSlot(szInputSlot, szInputSlot);
  s_InputSlots[szInputSlot].m_fDeadZone = ezMath::Max(fDeadZone, 0.0001f);
}

float ezInputManager::GetInputSlotDeadZone(const char* szInputSlot)
{
  ezMap<ezString, ezInputSlot>::ConstIterator it = s_InputSlots.Find(szInputSlot);

  if (it.IsValid())
    return it.Value().m_fDeadZone;

  ezInputSlot s;
  return s.m_fDeadZone; // return the default value
}

void ezInputManager::SetInputSlotScale(const char* szInputSlot, float fScale)
{
  RegisterInputSlot(szInputSlot, szInputSlot);
  s_InputSlots[szInputSlot].m_fScale = fScale;
}

float ezInputManager::GetInputSlotScale(const char* szInputSlot)
{
  ezMap<ezString, ezInputSlot>::ConstIterator it = s_InputSlots.Find(szInputSlot);

  if (it.IsValid())
    return it.Value().m_fScale;

  ezInputSlot s;
  return s.m_fScale; // return the default value
}

ezKeyState::Enum ezInputManager::GetInputSlotState(const char* szInputSlot, float* pValue)
{
  ezMap<ezString, ezInputSlot>::ConstIterator it = s_InputSlots.Find(szInputSlot);

  if (it.IsValid())
  {
    if (pValue)
      *pValue = it.Value().m_fValue;

    return it.Value().m_State;
  }

  if (pValue)
    *pValue = 0.0f;

  return ezKeyState::Up;
}



void ezInputManager::Update()
{
  ezInputDevice::UpdateAllDevices();

  GatherDeviceInputSlotValues();
  UpdateInputSlotStates();

  s_LastCharacter = ezInputDevice::RetrieveLastCharacterFromAllDevices();

  UpdateInputActions();

  ezInputDevice::ResetAllDevices();
}

void ezInputManager::ResetInputSlotValues()
{
  // set all input slot values to zero
  // this is crucial for accumulating the new values and for reseting the input state later
  for (ezInputSlotsMap::Iterator it = s_InputSlots.GetIterator(); it.IsValid(); it.Next())
  {
    it.Value().m_fValue = 0.0f;
  }
}

void ezInputManager::GatherDeviceInputSlotValues()
{
  ResetInputSlotValues();

  for (ezInputDevice* pDevice = ezInputDevice::GetFirstInstance(); pDevice != NULL; pDevice = pDevice->GetNextInstance())
  {
    ezMap<ezString, float, ezCompareHelper<ezString>, ezStaticAllocatorWrapper>::ConstIterator it = pDevice->m_InputSlotValues.GetIterator();

    // iterate over all the input slots that this device provides
    for ( ; it.IsValid(); it.Next())
    {
      if (it.Value() > 0.0f)
      {
        ezInputManager::ezInputSlot& Slot = s_InputSlots[it.Key()];

        // do not store a value larger than 0 unless it exceeds the deadzone threshold
        if (it.Value() > Slot.m_fDeadZone)
        {
          const float fStoreValue = (Slot.m_fScale >= 0.0f) ? it.Value() * Slot.m_fScale : ezMath::Pow(it.Value(), -Slot.m_fScale);

          Slot.m_fValue = ezMath::Max(Slot.m_fValue, fStoreValue); // 'accumulate' the values for one slot from all the connected devices
        }
      }
    }
  }
}

void ezInputManager::UpdateInputSlotStates()
{
  for (ezInputSlotsMap::Iterator it = s_InputSlots.GetIterator(); it.IsValid(); it.Next())
  {
    // update the state of the input slot, depending on its current value
    // its value will only be larger than zero, if it is also larger than its deadzone value
    it.Value().m_State = ezKeyState::GetNewKeyState(it.Value().m_State, it.Value().m_fValue > 0.0f);
  }
}

void ezInputManager::RetrieveAllKnownInputSlots(ezDynamicArray<const char*>& out_InputSlots)
{
  out_InputSlots.Clear();
  out_InputSlots.Reserve(s_InputSlots.GetCount());

  // just copy all slot names into the given array
  for (ezInputSlotsMap::Iterator it = s_InputSlots.GetIterator(); it.IsValid(); it.Next())
  {
    out_InputSlots.PushBack(it.Key().GetData());
  }
}

wchar_t ezInputManager::RetrieveLastCharacter(bool bResetCurrent)
{
  if (!bResetCurrent)
    return s_LastCharacter;

  wchar_t Temp = s_LastCharacter;
  s_LastCharacter = L'\0';
  return Temp;
}

