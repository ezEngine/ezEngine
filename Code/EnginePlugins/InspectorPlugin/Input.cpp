#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Core/Input/InputManager.h>
#include <Foundation/Communication/Telemetry.h>

namespace InputDetail
{

  static void SendInputSlotData(ezStringView sInputSlot)
  {
    float fValue = 0.0f;

    ezTelemetryMessage msg;
    msg.SetMessageID('INPT', 'SLOT');
    msg.GetWriter() << sInputSlot;
    msg.GetWriter() << ezInputManager::GetInputSlotFlags(sInputSlot).GetValue();
    msg.GetWriter() << (ezUInt8)ezInputManager::GetInputSlotState(sInputSlot, &fValue);
    msg.GetWriter() << fValue;
    msg.GetWriter() << ezInputManager::GetInputSlotDeadZone(sInputSlot);

    ezTelemetry::Broadcast(ezTelemetry::Reliable, msg);
  }

  static void SendInputActionData(ezStringView sInputSet, ezStringView sInputAction)
  {
    float fValue = 0.0f;

    const ezInputActionConfig cfg = ezInputManager::GetInputActionConfig(sInputSet, sInputAction);

    ezTelemetryMessage msg;
    msg.SetMessageID('INPT', 'ACTN');
    msg.GetWriter() << sInputSet;
    msg.GetWriter() << sInputAction;
    msg.GetWriter() << (ezUInt8)ezInputManager::GetInputActionState(sInputSet, sInputAction, &fValue);
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
    ezDynamicArray<ezStringView> InputSlots;
    ezInputManager::RetrieveAllKnownInputSlots(InputSlots);

    for (ezUInt32 i = 0; i < InputSlots.GetCount(); ++i)
    {
      SendInputSlotData(InputSlots[i]);
    }
  }

  static void SendAllInputActions()
  {
    ezDynamicArray<ezString> InputSetNames;
    ezInputManager::GetAllInputSets(InputSetNames);

    for (ezUInt32 s = 0; s < InputSetNames.GetCount(); ++s)
    {
      ezHybridArray<ezString, 24> InputActions;

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

      default:
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
        SendInputActionData(e.m_sInputSet, e.m_sInputAction);
        break;
      case ezInputManager::InputEventData::InputSlotChanged:
        SendInputSlotData(e.m_sInputSlot);
        break;

      default:
        break;
    }
  }
} // namespace InputDetail

void AddInputEventHandler()
{
  ezTelemetry::AddEventHandler(InputDetail::TelemetryEventsHandler);
  ezInputManager::AddEventHandler(InputDetail::InputManagerEventHandler);
}

void RemoveInputEventHandler()
{
  ezInputManager::RemoveEventHandler(InputDetail::InputManagerEventHandler);
  ezTelemetry::RemoveEventHandler(InputDetail::TelemetryEventsHandler);
}


