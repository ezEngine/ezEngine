#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Command/NodeCommands.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>
#include <ToolsFoundation/NodeObject/NodeCommandAccessor.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezNodeCommandAccessor, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezNodeCommandAccessor::ezNodeCommandAccessor(ezCommandHistory* pHistory)
  : ezObjectCommandAccessor(pHistory)
{
}

ezNodeCommandAccessor::~ezNodeCommandAccessor() = default;

ezStatus ezNodeCommandAccessor::SetValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index /*= ezVariant()*/)
{
  if (m_pHistory->InTemporaryTransaction() == false)
  {
    auto pNodeObject = pObject;
    auto pDynamicPinProperty = pProp;

    if (IsNode(pObject) == false)
    {
      auto pParent = pObject->GetParent();
      if (pParent != nullptr && IsNode(pParent))
      {
        pNodeObject = pParent;
        pDynamicPinProperty = pParent->GetType()->FindPropertyByName(pObject->GetParentProperty());
      }
    }

    if (IsDynamicPinProperty(pNodeObject, pDynamicPinProperty))
    {
      ezHybridArray<ConnectionInfo, 16> oldConnections;
      EZ_SUCCEED_OR_RETURN(DisconnectAllPins(pNodeObject, oldConnections));

      // TODO: remap oldConnections

      EZ_SUCCEED_OR_RETURN(ezObjectCommandAccessor::SetValue(pObject, pProp, newValue, index));

      return TryReconnectAllPins(pNodeObject, oldConnections);
    }
  }

  return ezObjectCommandAccessor::SetValue(pObject, pProp, newValue, index);
}

ezStatus ezNodeCommandAccessor::InsertValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& newValue, ezVariant index /*= ezVariant()*/)
{
  if (IsDynamicPinProperty(pObject, pProp))
  {
    ezHybridArray<ConnectionInfo, 16> oldConnections;
    EZ_SUCCEED_OR_RETURN(DisconnectAllPins(pObject, oldConnections));

    EZ_SUCCEED_OR_RETURN(ezObjectCommandAccessor::InsertValue(pObject, pProp, newValue, index));

    return TryReconnectAllPins(pObject, oldConnections);
  }
  else
  {
    return ezObjectCommandAccessor::InsertValue(pObject, pProp, newValue, index);
  }
}

ezStatus ezNodeCommandAccessor::RemoveValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, ezVariant index /*= ezVariant()*/)
{
  if (IsDynamicPinProperty(pObject, pProp))
  {
    ezHybridArray<ConnectionInfo, 16> oldConnections;
    EZ_SUCCEED_OR_RETURN(DisconnectAllPins(pObject, oldConnections));

    EZ_SUCCEED_OR_RETURN(ezObjectCommandAccessor::RemoveValue(pObject, pProp, index));

    return TryReconnectAllPins(pObject, oldConnections);
  }
  else
  {
    return ezObjectCommandAccessor::RemoveValue(pObject, pProp, index);
  }
}

ezStatus ezNodeCommandAccessor::MoveValue(const ezDocumentObject* pObject, const ezAbstractProperty* pProp, const ezVariant& oldIndex, const ezVariant& newIndex)
{
  if (IsDynamicPinProperty(pObject, pProp))
  {
    ezHybridArray<ConnectionInfo, 16> oldConnections;
    EZ_SUCCEED_OR_RETURN(DisconnectAllPins(pObject, oldConnections));

    // TODO: remap oldConnections

    EZ_SUCCEED_OR_RETURN(ezObjectCommandAccessor::MoveValue(pObject, pProp, oldIndex, newIndex));

    return TryReconnectAllPins(pObject, oldConnections);
  }
  else
  {
    return ezObjectCommandAccessor::MoveValue(pObject, pProp, oldIndex, newIndex);
  }
}

ezStatus ezNodeCommandAccessor::AddObject(const ezDocumentObject* pParent, const ezAbstractProperty* pParentProp, const ezVariant& index, const ezRTTI* pType, ezUuid& inout_objectGuid)
{
  if (IsDynamicPinProperty(pParent, pParentProp))
  {
    ezHybridArray<ConnectionInfo, 16> oldConnections;
    EZ_SUCCEED_OR_RETURN(DisconnectAllPins(pParent, oldConnections));

    // TODO: remap oldConnections

    EZ_SUCCEED_OR_RETURN(ezObjectCommandAccessor::AddObject(pParent, pParentProp, index, pType, inout_objectGuid));

    return TryReconnectAllPins(pParent, oldConnections);
  }
  else
  {
    return ezObjectCommandAccessor::AddObject(pParent, pParentProp, index, pType, inout_objectGuid);
  }
}

ezStatus ezNodeCommandAccessor::RemoveObject(const ezDocumentObject* pObject)
{
  if (const ezDocumentObject* pParent = pObject->GetParent())
  {
    const ezAbstractProperty* pProp = pParent->GetType()->FindPropertyByName(pObject->GetParentProperty());
    if (IsDynamicPinProperty(pParent, pProp))
    {
      ezHybridArray<ConnectionInfo, 16> oldConnections;
      EZ_SUCCEED_OR_RETURN(DisconnectAllPins(pParent, oldConnections));

      // TODO: remap oldConnections

      EZ_SUCCEED_OR_RETURN(ezObjectCommandAccessor::RemoveObject(pObject));

      return TryReconnectAllPins(pParent, oldConnections);
    }
  }

  return ezObjectCommandAccessor::RemoveObject(pObject);
}


bool ezNodeCommandAccessor::IsNode(const ezDocumentObject* pObject) const
{
  auto pManager = static_cast<const ezDocumentNodeManager*>(pObject->GetDocumentObjectManager());

  return pManager->IsNode(pObject);
}

bool ezNodeCommandAccessor::IsDynamicPinProperty(const ezDocumentObject* pObject, const ezAbstractProperty* pProp) const
{
  auto pManager = static_cast<const ezDocumentNodeManager*>(pObject->GetDocumentObjectManager());

  return pManager->IsDynamicPinProperty(pObject, pProp);
}

ezStatus ezNodeCommandAccessor::DisconnectAllPins(const ezDocumentObject* pObject, ezDynamicArray<ConnectionInfo>& out_oldConnections)
{
  auto pManager = static_cast<const ezDocumentNodeManager*>(pObject->GetDocumentObjectManager());

  auto Disconnect = [&](ezArrayPtr<const ezConnection* const> connections) -> ezStatus
  {
    for (const ezConnection* pConnection : connections)
    {
      auto& connectionInfo = out_oldConnections.ExpandAndGetRef();
      connectionInfo.m_pSource = pConnection->GetSourcePin().GetParent();
      connectionInfo.m_pTarget = pConnection->GetTargetPin().GetParent();
      connectionInfo.m_sSourcePin = pConnection->GetSourcePin().GetName();
      connectionInfo.m_sTargetPin = pConnection->GetTargetPin().GetName();

      EZ_SUCCEED_OR_RETURN(ezNodeCommands::DisconnectAndRemoveCommand(m_pHistory, pConnection->GetParent()->GetGuid()));
    }

    return ezStatus(EZ_SUCCESS);
  };

  auto inputs = pManager->GetInputPins(pObject);
  for (auto& pInputPin : inputs)
  {
    EZ_SUCCEED_OR_RETURN(Disconnect(pManager->GetConnections(*pInputPin)));
  }

  auto outputs = pManager->GetOutputPins(pObject);
  for (auto& pOutputPin : outputs)
  {
    EZ_SUCCEED_OR_RETURN(Disconnect(pManager->GetConnections(*pOutputPin)));
  }

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezNodeCommandAccessor::TryReconnectAllPins(const ezDocumentObject* pObject, const ezDynamicArray<ConnectionInfo>& oldConnections)
{
  auto pManager = static_cast<const ezDocumentNodeManager*>(pObject->GetDocumentObjectManager());
  const ezRTTI* pConnectionType = pManager->GetConnectionType();

  for (auto& connectionInfo : oldConnections)
  {
    const ezPin* pSourcePin = pManager->GetOutputPinByName(connectionInfo.m_pSource, connectionInfo.m_sSourcePin);
    const ezPin* pTargetPin = pManager->GetInputPinByName(connectionInfo.m_pTarget, connectionInfo.m_sTargetPin);

    // This connection can't be restored because a pin doesn't exist anymore, which is ok in this case.
    if (pSourcePin == nullptr || pTargetPin == nullptr)
      continue;

    // This connection is not valid anymore after pins have changed.
    ezDocumentNodeManager::CanConnectResult res;
    if (pManager->CanConnect(pConnectionType, *pSourcePin, *pTargetPin, res).Failed())
      continue;

    EZ_SUCCEED_OR_RETURN(ezNodeCommands::AddAndConnectCommand(m_pHistory, pConnectionType, *pSourcePin, *pTargetPin));
  }

  return ezStatus(EZ_SUCCESS);
}
