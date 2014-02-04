#include <PCH.h>
#include <Foundation/Communication/Telemetry.h>
#include <Core/Input/InputManager.h>

static void SendInputSlotData(const char* szInputSlot)
{
  float fValue = 0.0f;

  ezTelemetryMessage msg;
  msg.SetMessageID('INPT', 'SLOT');
  msg.GetWriter() << szInputSlot;
  msg.GetWriter() << ezInputManager::GetInputSlotFlags(szInputSlot).GetValue();
  msg.GetWriter() << (ezUInt8) ezInputManager::GetInputSlotState(szInputSlot, &fValue);
  msg.GetWriter() << fValue;
  msg.GetWriter() << ezInputManager::GetInputSlotDeadZone(szInputSlot);

  ezTelemetry::Broadcast(ezTelemetry::Reliable, msg);
}

static void SendInputActionData(const char* szInputSet, const char* szInputAction)
{
  float fValue = 0.0f;

  const ezInputActionConfig cfg = ezInputManager::GetInputActionConfig(szInputSet, szInputAction);

  ezTelemetryMessage msg;
  msg.SetMessageID('INPT', 'ACTN');
  msg.GetWriter() << szInputSet;
  msg.GetWriter() << szInputAction;
  msg.GetWriter() << (ezUInt8) ezInputManager::GetInputActionState(szInputSet, szInputAction, &fValue);
  msg.GetWriter() << fValue;
  msg.GetWriter() << cfg.m_bApplyTimeScaling;

  for (ezUInt32 i = 0; i < ezInputActionConfig::MaxInputSlotAlternatives; ++i)
  {
    msg.GetWriter() << cfg.m_sInputSlotTrigger[i];
    msg.GetWriter() << cfg.m_fInputSlotScale[i];
  }

  ezTelemetry::Broadcast(ezTelemetry::Reliable, msg);
}

static void SendAllInputSlots()
{
  ezDynamicArray<const char*> InputSlots;
  ezInputManager::RetrieveAllKnownInputSlots(InputSlots);

  for (ezUInt32 i = 0; i < InputSlots.GetCount(); ++i)
    SendInputSlotData(InputSlots[i]);
}

static void SendAllInputActions()
{
  ezDynamicArray<ezString> InputSetNames;
  ezInputManager::GetAllInputSets(InputSetNames);

  for (ezUInt32 s = 0; s < InputSetNames.GetCount(); ++s)
  {
    ezDynamicArray<ezString> InputActions;

    ezInputManager::GetAllInputActions(InputSetNames[s].GetData(), InputActions);

    for (ezUInt32 a = 0; a < InputActions.GetCount(); ++a)
      SendInputActionData(InputSetNames[s].GetData(), InputActions[a].GetData());
  }
}

static void TelemetryEventsHandler(const ezTelemetry::TelemetryEventData& e)
{
  if (!ezTelemetry::IsConnectedToClient())
    return;

  switch (e.m_EventType)
  {
  case ezTelemetry::TelemetryEventData::ConnectedToClient:
    SendAllInputSlots();
    SendAllInputActions();
    break;
  }
}

static void InputManagerEventHandler(const ezInputManager::InputEventData& e)
{
  if (!ezTelemetry::IsConnectedToClient())
    return;

  switch (e.m_EventType)
  {
  case ezInputManager::InputEventData::InputActionChanged:
    SendInputActionData(e.m_szInputSet, e.m_szInputAction);
    break;
  case ezInputManager::InputEventData::InputSlotChanged:
    SendInputSlotData(e.m_szInputSlot);
    break;
  }
}

void AddInputEventHandler()
{
  ezTelemetry::AddEventHandler(TelemetryEventsHandler);
  ezInputManager::AddEventHandler(InputManagerEventHandler);
}

void RemoveInputEventHandler()
{
  ezInputManager::RemoveEventHandler(InputManagerEventHandler);
  ezTelemetry::RemoveEventHandler(TelemetryEventsHandler);
}



EZ_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_Input);

