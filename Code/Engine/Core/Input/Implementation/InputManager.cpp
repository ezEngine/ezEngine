#include <Core/CorePCH.h>

#include <Core/Input/InputManager.h>

ezInputManager::ezEventInput ezInputManager::s_InputEvents;
ezInputManager::InternalData* ezInputManager::s_pData = nullptr;
ezUInt32 ezInputManager::s_LastCharacter = '\0';
bool ezInputManager::s_bInputSlotResetRequired = true;
ezString ezInputManager::s_sExclusiveInputSet;

ezInputManager::InternalData& ezInputManager::GetInternals()
{
  if (s_pData == nullptr)
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
      {
        ezStringBuilder tmp;
        tmp.Printf("Different devices register Input Slot '%s' with different Slot Flags: %16b vs. %16b", szInputSlot,
          it.Value().m_SlotFlags.GetValue(), SlotFlags.GetValue());
        ezLog::Warning(tmp);
      }

      it.Value().m_SlotFlags |= SlotFlags;
    }

    // If the key already exists, but key and display string are identical, then overwrite the display string with the incoming string
    if (it.Value().m_sDisplayName != it.Key())
      return;
  }

  // ezLog::Debug("Registered Input Slot: '{0}'", szInputSlot);

  ezInputSlot& sm = GetInternals().s_InputSlots[szInputSlot];

  sm.m_sDisplayName = szDefaultDisplayName;
  sm.m_SlotFlags = SlotFlags;

  InputEventData e;
  e.m_EventType = InputEventData::InputSlotChanged;
  e.m_szInputSlot = szInputSlot;

  s_InputEvents.Broadcast(e);
}

ezBitflags<ezInputSlotFlags> ezInputManager::GetInputSlotFlags(const char* szInputSlot)
{
  ezMap<ezString, ezInputSlot>::ConstIterator it = GetInternals().s_InputSlots.Find(szInputSlot);

  if (it.IsValid())
    return it.Value().m_SlotFlags;

  ezLog::Warning("ezInputManager::GetInputSlotFlags: Input Slot '{0}' does not exist (yet).", szInputSlot);

  return ezInputSlotFlags::Default;
}

void ezInputManager::SetInputSlotDisplayName(const char* szInputSlot, const char* szDefaultDisplayName)
{
  RegisterInputSlot(szInputSlot, szDefaultDisplayName, ezInputSlotFlags::Default);
  GetInternals().s_InputSlots[szInputSlot].m_sDisplayName = szDefaultDisplayName;

  InputEventData e;
  e.m_EventType = InputEventData::InputSlotChanged;
  e.m_szInputSlot = szInputSlot;

  s_InputEvents.Broadcast(e);
}

const char* ezInputManager::GetInputSlotDisplayName(const char* szInputSlot)
{
  ezMap<ezString, ezInputSlot>::ConstIterator it = GetInternals().s_InputSlots.Find(szInputSlot);

  if (it.IsValid())
    return it.Value().m_sDisplayName.GetData();

  ezLog::Warning("ezInputManager::GetInputSlotDisplayName: Input Slot '{0}' does not exist (yet).", szInputSlot);
  return szInputSlot;
}

const char* ezInputManager::GetInputSlotDisplayName(const char* szInputSet, const char* szAction, ezInt32 iTrigger)
{
  /// \test This is new

  const auto cfg = GetInputActionConfig(szInputSet, szAction);

  if (iTrigger < 0)
  {
    for (iTrigger = 0; iTrigger < ezInputActionConfig::MaxInputSlotAlternatives; ++iTrigger)
    {
      if (!cfg.m_sInputSlotTrigger[iTrigger].IsEmpty())
        break;
    }
  }

  if (iTrigger >= ezInputActionConfig::MaxInputSlotAlternatives)
    return nullptr;

  return GetInputSlotDisplayName(cfg.m_sInputSlotTrigger[iTrigger]);
}

void ezInputManager::SetInputSlotDeadZone(const char* szInputSlot, float fDeadZone)
{
  RegisterInputSlot(szInputSlot, szInputSlot, ezInputSlotFlags::Default);
  GetInternals().s_InputSlots[szInputSlot].m_fDeadZone = ezMath::Max(fDeadZone, 0.0001f);

  InputEventData e;
  e.m_EventType = InputEventData::InputSlotChanged;
  e.m_szInputSlot = szInputSlot;

  s_InputEvents.Broadcast(e);
}

float ezInputManager::GetInputSlotDeadZone(const char* szInputSlot)
{
  ezMap<ezString, ezInputSlot>::ConstIterator it = GetInternals().s_InputSlots.Find(szInputSlot);

  if (it.IsValid())
    return it.Value().m_fDeadZone;

  ezLog::Warning("ezInputManager::GetInputSlotDeadZone: Input Slot '{0}' does not exist (yet).", szInputSlot);

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

  ezLog::Warning("ezInputManager::GetInputSlotState: Input Slot '{0}' does not exist (yet). To ensure all devices are initialized, call "
                 "ezInputManager::Update before querying device states, or at least call ezInputManager::PollHardware.",
    szInputSlot);
  RegisterInputSlot(szInputSlot, szInputSlot, ezInputSlotFlags::None);

  return ezKeyState::Up;
}

void ezInputManager::PollHardware()
{
  if (s_bInputSlotResetRequired)
  {
    s_bInputSlotResetRequired = false;
    ResetInputSlotValues();
  }

  ezInputDevice::UpdateAllDevices();

  GatherDeviceInputSlotValues();
}

void ezInputManager::Update(ezTime tTimeDifference)
{
  PollHardware();

  UpdateInputSlotStates();

  s_LastCharacter = ezInputDevice::RetrieveLastCharacterFromAllDevices();

  UpdateInputActions(tTimeDifference);

  ezInputDevice::ResetAllDevices();

  ezInputDevice::UpdateAllHardwareStates(tTimeDifference);

  s_bInputSlotResetRequired = true;
}

void ezInputManager::ResetInputSlotValues()
{
  // set all input slot values to zero
  // this is crucial for accumulating the new values and for resetting the input state later
  for (ezInputSlotsMap::Iterator it = GetInternals().s_InputSlots.GetIterator(); it.IsValid(); it.Next())
  {
    it.Value().m_fValue = 0.0f;
  }
}

void ezInputManager::GatherDeviceInputSlotValues()
{
  for (ezInputDevice* pDevice = ezInputDevice::GetFirstInstance(); pDevice != nullptr; pDevice = pDevice->GetNextInstance())
  {
    pDevice->m_bGeneratedInputRecently = false;

    // iterate over all the input slots that this device provides
    for (auto it = pDevice->m_InputSlotValues.GetIterator(); it.IsValid(); it.Next())
    {
      if (it.Value() > 0.0f)
      {
        ezInputManager::ezInputSlot& Slot = GetInternals().s_InputSlots[it.Key()];

        // do not store a value larger than 0 unless it exceeds the dead-zone threshold
        if (it.Value() > Slot.m_fDeadZone)
        {
          Slot.m_fValue = ezMath::Max(Slot.m_fValue, it.Value()); // 'accumulate' the values for one slot from all the connected devices

          pDevice->m_bGeneratedInputRecently = true;
        }
      }
    }
  }

  ezMap<ezString, float>::Iterator it = GetInternals().s_InjectedInputSlots.GetIterator();

  for (; it.IsValid(); ++it)
  {
    ezInputManager::ezInputSlot& Slot = GetInternals().s_InputSlots[it.Key()];

    // do not store a value larger than 0 unless it exceeds the dead-zone threshold
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
    // its value will only be larger than zero, if it is also larger than its dead-zone value
    const ezKeyState::Enum NewState = ezKeyState::GetNewKeyState(it.Value().m_State, it.Value().m_fValue > 0.0f);

    if ((it.Value().m_State != NewState) || (NewState != ezKeyState::Up))
    {
      it.Value().m_State = NewState;

      InputEventData e;
      e.m_EventType = InputEventData::InputSlotChanged;
      e.m_szInputSlot = it.Key().GetData();

      s_InputEvents.Broadcast(e);
    }
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

ezUInt32 ezInputManager::RetrieveLastCharacter(bool bResetCurrent)
{
  if (!bResetCurrent)
    return s_LastCharacter;

  ezUInt32 Temp = s_LastCharacter;
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

const char* ezInputManager::GetInputSlotTouchPoint(unsigned int index)
{
  switch (index)
  {
    case 0:
      return ezInputSlot_TouchPoint0;
    case 1:
      return ezInputSlot_TouchPoint1;
    case 2:
      return ezInputSlot_TouchPoint2;
    case 3:
      return ezInputSlot_TouchPoint3;
    case 4:
      return ezInputSlot_TouchPoint4;
    case 5:
      return ezInputSlot_TouchPoint5;
    case 6:
      return ezInputSlot_TouchPoint6;
    case 7:
      return ezInputSlot_TouchPoint7;
    case 8:
      return ezInputSlot_TouchPoint8;
    case 9:
      return ezInputSlot_TouchPoint9;
    default:
      EZ_REPORT_FAILURE("Maximum number of supported input touch points is 10");
      return "";
  }
}

const char* ezInputManager::GetInputSlotTouchPointPositionX(unsigned int index)
{
  switch (index)
  {
    case 0:
      return ezInputSlot_TouchPoint0_PositionX;
    case 1:
      return ezInputSlot_TouchPoint1_PositionX;
    case 2:
      return ezInputSlot_TouchPoint2_PositionX;
    case 3:
      return ezInputSlot_TouchPoint3_PositionX;
    case 4:
      return ezInputSlot_TouchPoint4_PositionX;
    case 5:
      return ezInputSlot_TouchPoint5_PositionX;
    case 6:
      return ezInputSlot_TouchPoint6_PositionX;
    case 7:
      return ezInputSlot_TouchPoint7_PositionX;
    case 8:
      return ezInputSlot_TouchPoint8_PositionX;
    case 9:
      return ezInputSlot_TouchPoint9_PositionX;
    default:
      EZ_REPORT_FAILURE("Maximum number of supported input touch points is 10");
      return "";
  }
}

const char* ezInputManager::GetInputSlotTouchPointPositionY(unsigned int index)
{
  switch (index)
  {
    case 0:
      return ezInputSlot_TouchPoint0_PositionY;
    case 1:
      return ezInputSlot_TouchPoint1_PositionY;
    case 2:
      return ezInputSlot_TouchPoint2_PositionY;
    case 3:
      return ezInputSlot_TouchPoint3_PositionY;
    case 4:
      return ezInputSlot_TouchPoint4_PositionY;
    case 5:
      return ezInputSlot_TouchPoint5_PositionY;
    case 6:
      return ezInputSlot_TouchPoint6_PositionY;
    case 7:
      return ezInputSlot_TouchPoint7_PositionY;
    case 8:
      return ezInputSlot_TouchPoint8_PositionY;
    case 9:
      return ezInputSlot_TouchPoint9_PositionY;
    default:
      EZ_REPORT_FAILURE("Maximum number of supported input touch points is 10");
      return "";
  }
}

EZ_STATICLINK_FILE(Core, Core_Input_Implementation_InputManager);
