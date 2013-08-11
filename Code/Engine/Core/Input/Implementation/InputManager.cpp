#include <Core/PCH.h>
#include <Core/Input/InputManager.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Configuration/Startup.h>

ezInputManager::InternalData* ezInputManager::s_pData = NULL;
wchar_t ezInputManager::s_LastCharacter = '\0';

ezInputManager::InternalData& ezInputManager::GetInternals()
{
  if (s_pData == NULL)
    s_pData = EZ_DEFAULT_NEW(InternalData);

  return *s_pData;
}

void ezInputManager::DeallocateInternals()
{
  EZ_DEFAULT_DELETE(s_pData);
}

ezInputManager::ezInputSlot::ezInputSlot()
{
  m_fValue = 0.0f;
  m_State = ezKeyState::Up;
  m_fDeadZone = 0.0f;
}

void ezInputManager::RegisterInputSlot(const char* szInputSlot, const char* szDefaultDisplayName, ezBitflags<ezInputSlotFlags> SlotFlags)
{
  ezMap<ezString, ezInputSlot>::Iterator it = GetInternals().s_InputSlots.Find(szInputSlot);

  
  if (it.IsValid())
  {
    if (it.Value().m_SlotFlags != SlotFlags)
    {
      if ((it.Value().m_SlotFlags != ezInputSlotFlags::Default) && (SlotFlags != ezInputSlotFlags::Default))
        ezLog::Warning("Different devices register Input Slot '%s' with different Slot Flags: %16b vs. %16b", szInputSlot, it.Value().m_SlotFlags, SlotFlags);

      it.Value().m_SlotFlags |= SlotFlags;
    }

    // If the key already exists, but key and display string are identical, than overwrite the display string with the incoming string
    if (it.Value().m_sDisplayName != it.Key())
      return;
  }


  ezLog::Dev("Registered Input Slot: '%s'", szInputSlot);

  GetInternals().s_InputSlots[szInputSlot].m_sDisplayName = szDefaultDisplayName;
  GetInternals().s_InputSlots[szInputSlot].m_SlotFlags = SlotFlags;
}

ezBitflags<ezInputSlotFlags> ezInputManager::GetInputSlotFlags(const char* szInputSlot)
{
  ezMap<ezString, ezInputSlot>::ConstIterator it = GetInternals().s_InputSlots.Find(szInputSlot);

  if (it.IsValid())
    return it.Value().m_SlotFlags;

  ezLog::Warning("ezInputManager::GetInputSlotFlags: Input Slot '%s' does not exist (yet).", szInputSlot);

  return ezInputSlotFlags::Default;
}

void ezInputManager::SetInputSlotDisplayName(const char* szInputSlot, const char* szDefaultDisplayName)
{
  RegisterInputSlot(szInputSlot, szDefaultDisplayName, ezInputSlotFlags::Default);
  GetInternals().s_InputSlots[szInputSlot].m_sDisplayName = szDefaultDisplayName;
}

const char* ezInputManager::GetInputSlotDisplayName(const char* szInputSlot)
{
  ezMap<ezString, ezInputSlot>::ConstIterator it = GetInternals().s_InputSlots.Find(szInputSlot);

  if (it.IsValid())
    return it.Value().m_sDisplayName.GetData();

  ezLog::Warning("ezInputManager::GetInputSlotDisplayName: Input Slot '%s' does not exist (yet).", szInputSlot);
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
  RegisterInputSlot(szInputSlot, szInputSlot, ezInputSlotFlags::Default);
  GetInternals().s_InputSlots[szInputSlot].m_fDeadZone = ezMath::Max(fDeadZone, 0.0001f);
}

float ezInputManager::GetInputSlotDeadZone(const char* szInputSlot)
{
  ezMap<ezString, ezInputSlot>::ConstIterator it = GetInternals().s_InputSlots.Find(szInputSlot);

  if (it.IsValid())
    return it.Value().m_fDeadZone;

  ezLog::Warning("ezInputManager::GetInputSlotDeadZone: Input Slot '%s' does not exist (yet).", szInputSlot);

  ezInputSlot s;
  return s.m_fDeadZone; // return the default value
}

ezKeyState::Enum ezInputManager::GetInputSlotState(const char* szInputSlot, float* pValue)
{
  ezMap<ezString, ezInputSlot>::ConstIterator it = GetInternals().s_InputSlots.Find(szInputSlot);

  if (it.IsValid())
  {
    if (pValue)
      *pValue = it.Value().m_fValue;

    return it.Value().m_State;
  }

  if (pValue)
    *pValue = 0.0f;

  ezLog::Warning("ezInputManager::GetInputSlotState: Input Slot '%s' does not exist (yet).", szInputSlot);

  return ezKeyState::Up;
}



void ezInputManager::Update(double fTimeDifference)
{
  ezInputDevice::UpdateAllDevices(fTimeDifference);

  GatherDeviceInputSlotValues();
  UpdateInputSlotStates();

  s_LastCharacter = ezInputDevice::RetrieveLastCharacterFromAllDevices();

  UpdateInputActions(fTimeDifference);

  ezInputDevice::ResetAllDevices();
}

void ezInputManager::ResetInputSlotValues()
{
  // set all input slot values to zero
  // this is crucial for accumulating the new values and for reseting the input state later
  for (ezInputSlotsMap::Iterator it = GetInternals().s_InputSlots.GetIterator(); it.IsValid(); it.Next())
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
        ezInputManager::ezInputSlot& Slot = GetInternals().s_InputSlots[it.Key()];

        // do not store a value larger than 0 unless it exceeds the deadzone threshold
        if (it.Value() > Slot.m_fDeadZone)
          Slot.m_fValue = ezMath::Max(Slot.m_fValue, it.Value()); // 'accumulate' the values for one slot from all the connected devices
      }
    }
  }

  ezMap<ezString, float>::Iterator it = GetInternals().s_InjectedInputSlots.GetIterator();

  for ( ; it.IsValid(); ++it)
  {
    ezInputManager::ezInputSlot& Slot = GetInternals().s_InputSlots[it.Key()];

    // do not store a value larger than 0 unless it exceeds the deadzone threshold
    if (it.Value() > Slot.m_fDeadZone)
      Slot.m_fValue = ezMath::Max(Slot.m_fValue, it.Value()); // 'accumulate' the values for one slot from all the connected devices
  }

  GetInternals().s_InjectedInputSlots.Clear();
}

void ezInputManager::UpdateInputSlotStates()
{
  for (ezInputSlotsMap::Iterator it = GetInternals().s_InputSlots.GetIterator(); it.IsValid(); it.Next())
  {
    // update the state of the input slot, depending on its current value
    // its value will only be larger than zero, if it is also larger than its deadzone value
    it.Value().m_State = ezKeyState::GetNewKeyState(it.Value().m_State, it.Value().m_fValue > 0.0f);
  }
}

void ezInputManager::RetrieveAllKnownInputSlots(ezDynamicArray<const char*>& out_InputSlots)
{
  out_InputSlots.Clear();
  out_InputSlots.Reserve(GetInternals().s_InputSlots.GetCount());

  // just copy all slot names into the given array
  for (ezInputSlotsMap::Iterator it = GetInternals().s_InputSlots.GetIterator(); it.IsValid(); it.Next())
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

void ezInputManager::InjectInputSlotValue(const char* szInputSlot, float fValue)
{
  GetInternals().s_InjectedInputSlots[szInputSlot] = ezMath::Max(GetInternals().s_InjectedInputSlots[szInputSlot], fValue);
}

const char* ezInputManager::GetPressedInputSlot(ezInputSlotFlags::Enum MustHaveFlags, ezInputSlotFlags::Enum MustNotHaveFlags)
{
  for (ezInputSlotsMap::Iterator it = GetInternals().s_InputSlots.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value().m_State != ezKeyState::Pressed)
      continue;

    if (it.Value().m_SlotFlags.IsAnySet(MustNotHaveFlags))
      continue;

    if (it.Value().m_SlotFlags.AreAllSet(MustHaveFlags))
      return it.Key().GetData();
  }

  return ezInputSlot_None;
}





