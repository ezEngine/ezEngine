#include <Core/CorePCH.h>

#include <Core/Input/DeviceTypes/MouseKeyboard.h>
#include <Core/Input/InputManager.h>
#include <Foundation/Threading/ThreadUtils.h>

ezInputManager::ezEventInput ezInputManager::s_InputEvents;
ezInputManager::InternalData* ezInputManager::s_pData = nullptr;
ezString ezInputManager::s_sLastCharacters;
bool ezInputManager::s_bInputSlotResetRequired = true;
ezString ezInputManager::s_sExclusiveInputSet;
ezMouseCursorDesc ezInputManager::s_MouseCursor;
ezUInt32 ezInputManager::s_uiMouseCursorChangeCounter = 0;
ezUInt32 ezInputManager::s_uiMouseCursorIdChangeCounter = 0;
ezHybridArray<ezInputManager::MouseCursorOverride, 4> ezInputManager::s_MouseCursorOverrides;
ezUInt32 ezInputManager::s_uiNextMouseCursorOverrideId = 0;
ezUInt32 ezInputManager::s_uiHardwareCursorSize = 0;
ezUInt32 ezInputManager::s_uiUpdateCount = 0;

bool ezMouseCursorDesc::operator==(const ezMouseCursorDesc& rhs) const
{
  return m_sCursor == rhs.m_sCursor &&
         m_fSize == rhs.m_fSize &&
         m_vHotspot == rhs.m_vHotspot &&
         m_vUvTopLeft == rhs.m_vUvTopLeft &&
         m_vUvBottomRight == rhs.m_vUvBottomRight &&
         m_Rotation == rhs.m_Rotation &&
         m_Color == rhs.m_Color;
}

void ezInputManager::SetMouseCursor(const ezMouseCursorDesc& desc)
{
  if (s_MouseCursor == desc)
    return;

  const bool bIdentifierChanged = (s_MouseCursor.m_sCursor != desc.m_sCursor);

  s_MouseCursor = desc;
  ++s_uiMouseCursorChangeCounter;

  if (bIdentifierChanged)
  {
    // only whether a custom cursor is used at all can affect the OS cursor,
    // all the other properties are pure visuals
    ++s_uiMouseCursorIdChangeCounter;

    UpdateMouseCursorState();
  }
}

ezUInt32 ezInputManager::PushMouseCursorOverride(const ezMouseCursorOverrideDesc& desc)
{
  EZ_ASSERT_DEV(ezThreadUtils::IsMainThread(), "Mouse cursor overrides may only be modified from the main thread.");

  auto& o = s_MouseCursorOverrides.ExpandAndGetRef();
  o.m_uiId = ++s_uiNextMouseCursorOverrideId;
  o.m_Desc = desc;

  UpdateMouseCursorState();

  return o.m_uiId;
}

void ezInputManager::PopMouseCursorOverride(ezUInt32 uiOverrideId)
{
  if (uiOverrideId == 0)
    return;

  EZ_ASSERT_DEV(ezThreadUtils::IsMainThread(), "Mouse cursor overrides may only be modified from the main thread.");

  for (ezUInt32 i = 0; i < s_MouseCursorOverrides.GetCount(); ++i)
  {
    if (s_MouseCursorOverrides[i].m_uiId == uiOverrideId)
    {
      // must preserve the order of the remaining overrides, the last one wins
      s_MouseCursorOverrides.RemoveAtAndCopy(i);

      UpdateMouseCursorState();
      return;
    }
  }
}

ezMouseCursorOverrideDesc ezInputManager::GetActiveMouseCursorOverride()
{
  if (!s_MouseCursorOverrides.IsEmpty())
    return s_MouseCursorOverrides.PeekBack().m_Desc;

  ezMouseCursorOverrideDesc d;
  d.m_bForceNoClip = false;
  d.m_OSCursor = ezMouseCursorOverride::None;

  return d;
}

bool ezInputManager::IsCustomMouseCursorActive()
{
  // An explicit override wins over the custom cursor, which in turn wins over what the application wants.
  return !s_MouseCursor.m_sCursor.IsEmpty() && (GetActiveMouseCursorOverride().m_OSCursor != ezMouseCursorOverride::ForceOSCursor);
}

void ezInputManager::UpdateMouseCursorState()
{
  for (auto pDevice = ezInputDevice::GetFirstInstance(); pDevice != nullptr; pDevice = pDevice->GetNextInstance())
  {
    if (auto pMouse = ezDynamicCast<ezInputDeviceMouseKeyboard*>(pDevice))
    {
      pMouse->UpdateEffectiveMouseCursorState();
    }
  }
}

ezMouseCursorOverrideRequest::ezMouseCursorOverrideRequest(ezMouseCursorOverrideRequest&& rhs)
{
  m_uiOverrideId = rhs.m_uiOverrideId;
  m_Desc = rhs.m_Desc;

  rhs.m_uiOverrideId = 0;
}

void ezMouseCursorOverrideRequest::operator=(ezMouseCursorOverrideRequest&& rhs)
{
  if (this == &rhs)
    return;

  Release();

  m_uiOverrideId = rhs.m_uiOverrideId;
  m_Desc = rhs.m_Desc;

  rhs.m_uiOverrideId = 0;
}

void ezMouseCursorOverrideRequest::Request(const ezMouseCursorOverrideDesc& desc)
{
  if (IsActive())
  {
    if (m_Desc == desc)
      return;

    Release();
  }

  m_Desc = desc;
  m_uiOverrideId = ezInputManager::PushMouseCursorOverride(desc);
}

void ezMouseCursorOverrideRequest::Release()
{
  if (!IsActive())
    return;

  ezInputManager::PopMouseCursorOverride(m_uiOverrideId);
  m_uiOverrideId = 0;
}

void ezInputManager::ClearMouseCursor()
{
  SetMouseCursor(ezMouseCursorDesc());
}

ezUInt32 ezInputManager::GetHardwareCursorSize()
{
  // Update() re-queries this periodically, this only covers being called before the first Update().
  if (s_uiHardwareCursorSize == 0)
  {
    UpdateHardwareCursorSize();
  }

  // fall back to the typical cursor size at 100% scaling, if no device could report one
  return s_uiHardwareCursorSize > 0 ? s_uiHardwareCursorSize : 32;
}

void ezInputManager::UpdateHardwareCursorSize()
{
  s_uiHardwareCursorSize = 0;

  if (auto pMouse = GetInputDeviceOfType<ezInputDeviceMouseKeyboard>())
  {
    s_uiHardwareCursorSize = pMouse->GetHardwareCursorSize();
  }
}

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

void ezInputManager::RegisterInputSlot(ezStringView sInputSlot, ezStringView sDefaultDisplayName, ezBitflags<ezInputSlotFlags> SlotFlags)
{
  ezMap<ezString, ezInputSlot>::Iterator it = GetInternals().s_InputSlots.Find(sInputSlot);

  if (it.IsValid())
  {
    if (it.Value().m_SlotFlags != SlotFlags)
    {
      if ((it.Value().m_SlotFlags != ezInputSlotFlags::Default) && (SlotFlags != ezInputSlotFlags::Default))
      {
        ezStringBuilder tmp, tmp2;
        tmp.SetPrintf("Different devices register Input Slot '%s' with different Slot Flags: %16b vs. %16b",
          sInputSlot.GetData(tmp2), it.Value().m_SlotFlags.GetValue(), SlotFlags.GetValue());

        ezLog::Warning(tmp);
      }

      it.Value().m_SlotFlags |= SlotFlags;
    }

    // If the key already exists, but key and display string are identical, then overwrite the display string with the incoming string
    if (it.Value().m_sDisplayName != it.Key())
      return;
  }

  // ezLog::Debug("Registered Input Slot: '{0}'", sInputSlot);

  ezInputSlot& sm = GetInternals().s_InputSlots[sInputSlot];

  sm.m_sDisplayName = sDefaultDisplayName;
  sm.m_SlotFlags = SlotFlags;

  InputEventData e;
  e.m_EventType = InputEventData::InputSlotChanged;
  e.m_sInputSlot = sInputSlot;

  s_InputEvents.Broadcast(e);
}

ezBitflags<ezInputSlotFlags> ezInputManager::GetInputSlotFlags(ezStringView sInputSlot)
{
  ezMap<ezString, ezInputSlot>::ConstIterator it = GetInternals().s_InputSlots.Find(sInputSlot);

  if (it.IsValid())
    return it.Value().m_SlotFlags;

  ezLog::Warning("ezInputManager::GetInputSlotFlags: Input Slot '{0}' does not exist (yet).", sInputSlot);

  return ezInputSlotFlags::Default;
}

void ezInputManager::SetInputSlotDisplayName(ezStringView sInputSlot, ezStringView sDefaultDisplayName)
{
  RegisterInputSlot(sInputSlot, sDefaultDisplayName, ezInputSlotFlags::Default);
  GetInternals().s_InputSlots[sInputSlot].m_sDisplayName = sDefaultDisplayName;

  InputEventData e;
  e.m_EventType = InputEventData::InputSlotChanged;
  e.m_sInputSlot = sInputSlot;

  s_InputEvents.Broadcast(e);
}

ezStringView ezInputManager::GetInputSlotDisplayName(ezStringView sInputSlot)
{
  ezMap<ezString, ezInputSlot>::ConstIterator it = GetInternals().s_InputSlots.Find(sInputSlot);

  if (it.IsValid())
    return it.Value().m_sDisplayName.GetData();

  ezLog::Warning("ezInputManager::GetInputSlotDisplayName: Input Slot '{0}' does not exist (yet).", sInputSlot);
  return sInputSlot;
}

ezStringView ezInputManager::GetInputSlotDisplayName(ezStringView sInputSet, ezStringView sAction, ezInt32 iTrigger)
{
  /// \test This is new

  const auto cfg = GetInputActionConfig(sInputSet, sAction);

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

void ezInputManager::SetInputSlotDeadZone(ezStringView sInputSlot, float fDeadZone)
{
  RegisterInputSlot(sInputSlot, sInputSlot, ezInputSlotFlags::Default);
  GetInternals().s_InputSlots[sInputSlot].m_fDeadZone = ezMath::Max(fDeadZone, 0.0001f);

  InputEventData e;
  e.m_EventType = InputEventData::InputSlotChanged;
  e.m_sInputSlot = sInputSlot;

  s_InputEvents.Broadcast(e);
}

float ezInputManager::GetInputSlotDeadZone(ezStringView sInputSlot)
{
  ezMap<ezString, ezInputSlot>::ConstIterator it = GetInternals().s_InputSlots.Find(sInputSlot);

  if (it.IsValid())
    return it.Value().m_fDeadZone;

  ezLog::Warning("ezInputManager::GetInputSlotDeadZone: Input Slot '{0}' does not exist (yet).", sInputSlot);

  ezInputSlot s;
  return s.m_fDeadZone; // return the default value
}

ezKeyState::Enum ezInputManager::GetInputSlotState(ezStringView sInputSlot, float* pValue)
{
  ezMap<ezString, ezInputSlot>::ConstIterator it = GetInternals().s_InputSlots.Find(sInputSlot);

  if (it.IsValid())
  {
    if (pValue)
    {
      *pValue = s_bInputSlotResetRequired ? it.Value().m_fValue : it.Value().m_fValueOld;
    }
    return it.Value().m_State;
  }

  if (pValue)
    *pValue = 0.0f;

  ezLog::Warning("ezInputManager::GetInputSlotState: Input Slot '{0}' does not exist (yet). To ensure all devices are initialized, call "
                 "ezInputManager::Update before querying device states, or at least call ezInputManager::PollHardware.",
    sInputSlot);

  RegisterInputSlot(sInputSlot, sInputSlot, ezInputSlotFlags::None);

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

void ezInputManager::Update(ezTime timeDifference)
{
  PollHardware();

  UpdateInputSlotStates();

  s_sLastCharacters = ezInputDevice::RetrieveLastCharactersFromAllDevices();

  UpdateInputActions(timeDifference);

  ezInputDevice::ResetAllDevices();

  ezInputDevice::UpdateAllHardwareStates(timeDifference);

  // safety net, so that devices that were created after the last state change pick it up as well
  UpdateMouseCursorState();

  // The OS cursor size only changes when the user changes their settings, or when the window moves
  // to a monitor with a different DPI scaling, so re-querying it every few hundred updates is enough.
  if ((s_uiUpdateCount % 250) == 0)
  {
    UpdateHardwareCursorSize();
  }

  ++s_uiUpdateCount;

  s_bInputSlotResetRequired = true;
}

void ezInputManager::ResetInputSlotValues()
{
  // set all input slot values to zero
  // this is crucial for accumulating the new values and for resetting the input state later
  for (ezInputSlotsMap::Iterator it = GetInternals().s_InputSlots.GetIterator(); it.IsValid(); it.Next())
  {
    it.Value().m_fValueOld = it.Value().m_fValue;
    it.Value().m_fValue = 0.0f;
  }
}

void ezInputManager::GatherDeviceInputSlotValues()
{
  struct AbsValue
  {
    float fOrdinary = 0.0f;
    float fOverride = 0.0f;
    bool bHasOverride = false;
  };

  ezMap<ezString, AbsValue> absValues(ezTempAllocator::Get());

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
          const bool bAbsolute = Slot.m_SlotFlags.IsSet(ezInputSlotFlags::FullAxis) && !Slot.m_SlotFlags.IsSet(ezInputSlotFlags::ReportsRelativeValues);

          if (bAbsolute && pDevice->m_bOverridesAbsoluteInput)
          {
            auto& val = absValues[it.Key()];
            val.fOverride = ezMath::Max(val.fOverride, it.Value());
            val.bHasOverride = true;
          }
          else if (bAbsolute)
          {
            auto& val = absValues[it.Key()];
            val.fOrdinary = ezMath::Max(val.fOrdinary, it.Value());
          }
          else
          {
            Slot.m_fValue = ezMath::Max(Slot.m_fValue, it.Value()); // 'accumulate' the values for one slot from all the connected devices
          }

          // Only count as recent input if the slot represents something the user actively interacted with.
          // This includes buttons/keys (Pressable/Holdable)
          // This excludes passive position tracking (mouse position, touch position) which maintain
          // non-zero values even when no actual user interaction is happening.
          // Mouse move events are also excluded, as they could easily happen accidentally
          if (Slot.m_SlotFlags.IsAnySet(ezInputSlotFlags::Pressable | ezInputSlotFlags::Holdable))
          {
            pDevice->m_bGeneratedInputRecently = true;
          }
        }
      }
    }
  }

  for (auto it = absValues.GetIterator(); it.IsValid(); ++it)
  {
    auto& val = it.Value();
    if (val.bHasOverride)
    {
      GetInternals().s_InputSlots[it.Key()].m_fValue = val.fOverride;
    }
    else
    {
      GetInternals().s_InputSlots[it.Key()].m_fValue = val.fOrdinary;
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
      e.m_sInputSlot = it.Key().GetData();

      s_InputEvents.Broadcast(e);
    }
  }
}

void ezInputManager::RetrieveAllKnownInputSlots(ezDynamicArray<ezStringView>& out_inputSlots)
{
  out_inputSlots.Clear();
  out_inputSlots.Reserve(GetInternals().s_InputSlots.GetCount());

  // just copy all slot names into the given array
  for (ezInputSlotsMap::Iterator it = GetInternals().s_InputSlots.GetIterator(); it.IsValid(); it.Next())
  {
    out_inputSlots.PushBack(it.Key().GetData());
  }
}

ezString ezInputManager::RetrieveLastCharacters(bool bResetCurrent)
{
  if (!bResetCurrent)
    return s_sLastCharacters;

  ezString sResult = s_sLastCharacters;
  s_sLastCharacters.Clear();
  return sResult;
}

void ezInputManager::InjectInputSlotValue(ezStringView sInputSlot, float fValue)
{
  GetInternals().s_InjectedInputSlots[sInputSlot] = ezMath::Max(GetInternals().s_InjectedInputSlots[sInputSlot], fValue);
}

ezStringView ezInputManager::GetPressedInputSlot(ezInputSlotFlags::Enum mustHaveFlags, ezInputSlotFlags::Enum mustNotHaveFlags)
{
  for (ezInputSlotsMap::Iterator it = GetInternals().s_InputSlots.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value().m_State != ezKeyState::Pressed)
      continue;

    if (it.Value().m_SlotFlags.IsAnySet(mustNotHaveFlags))
      continue;

    if (it.Value().m_SlotFlags.AreAllSet(mustHaveFlags))
      return it.Key().GetData();
  }

  return ezInputSlot_None;
}

ezStringView ezInputManager::GetInputSlotTouchPoint(ezUInt32 uiIndex)
{
  switch (uiIndex)
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

ezStringView ezInputManager::GetInputSlotTouchPointPositionX(ezUInt32 uiIndex)
{
  switch (uiIndex)
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

ezStringView ezInputManager::GetInputSlotTouchPointPositionY(ezUInt32 uiIndex)
{
  switch (uiIndex)
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

void ezInputManager::GetInputDevicesOfType(const ezRTTI* pRtti, ezDynamicArray<ezInputDevice*>& out_devices)
{
  out_devices.Clear();

  for (auto pDev = ezInputDevice::GetFirstInstance(); pDev; pDev = pDev->GetNextInstance())
  {
    if (pDev->IsInstanceOf(pRtti))
    {
      out_devices.PushBack(pDev);
    }
  }
}
