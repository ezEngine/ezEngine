#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Command/NodeCommands.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRemoveNodeCommand, 1, ezRTTIDefaultAllocator<ezRemoveNodeCommand>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ObjectGuid", m_Object),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMoveNodeCommand, 1, ezRTTIDefaultAllocator<ezMoveNodeCommand>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ObjectGuid", m_Object),
    EZ_MEMBER_PROPERTY("NewPos", m_NewPos),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezConnectNodePinsCommand, 1, ezRTTIDefaultAllocator<ezConnectNodePinsCommand>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ConnectionGuid", m_ConnectionObject),
    EZ_MEMBER_PROPERTY("SourceGuid", m_ObjectSource),
    EZ_MEMBER_PROPERTY("TargetGuid", m_ObjectTarget),
    EZ_MEMBER_PROPERTY("SourcePin", m_sSourcePin),
    EZ_MEMBER_PROPERTY("TargetPin", m_sTargetPin),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDisconnectNodePinsCommand, 1, ezRTTIDefaultAllocator<ezDisconnectNodePinsCommand>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ConnectionGuid", m_ConnectionObject),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

////////////////////////////////////////////////////////////////////////
// ezRemoveNodeCommand
////////////////////////////////////////////////////////////////////////

ezRemoveNodeCommand::ezRemoveNodeCommand() = default;

ezStatus ezRemoveNodeCommand::DoInternal(bool bRedo)
{
  ezDocument* pDocument = GetDocument();
  ezDocumentNodeManager* pManager = static_cast<ezDocumentNodeManager*>(pDocument->GetObjectManager());

  auto RemoveConnections = [&](const ezPin& pin) {
    while (true)
    {
      auto connections = pManager->GetConnections(pin);

      if (connections.IsEmpty())
        break;

      ezDisconnectNodePinsCommand cmd;
      cmd.m_ConnectionObject = connections[0]->GetParent()->GetGuid();
      ezStatus res = AddSubCommand(cmd);
      if (res.m_Result.Succeeded())
      {
        ezRemoveObjectCommand remove;
        remove.m_Object = cmd.m_ConnectionObject;
        res = AddSubCommand(remove);
      }

      EZ_SUCCEED_OR_RETURN(res);
    }
    return ezStatus(EZ_SUCCESS);
  };

  if (!bRedo)
  {    
    m_pObject = pManager->GetObject(m_Object);
    if (m_pObject == nullptr)
      return ezStatus("Remove Node: The given object does not exist!");

    auto inputs = pManager->GetInputPins(m_pObject);
    for (auto& pPinTarget : inputs)
    {
      EZ_SUCCEED_OR_RETURN(RemoveConnections(*pPinTarget));
    }

    auto outputs = pManager->GetOutputPins(m_pObject);
    for (auto& pPinSource : outputs)
    {
      EZ_SUCCEED_OR_RETURN(RemoveConnections(*pPinSource));
    }

    ezRemoveObjectCommand cmd;
    cmd.m_Object = m_Object;
    auto res = AddSubCommand(cmd);
    if (res.m_Result.Failed())
    {
      return res;
    }
  }
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezRemoveNodeCommand::UndoInternal(bool bFireEvents)
{
  EZ_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");
  return ezStatus(EZ_SUCCESS);
}

void ezRemoveNodeCommand::CleanupInternal(CommandState state) {}


////////////////////////////////////////////////////////////////////////
// ezMoveObjectCommand
////////////////////////////////////////////////////////////////////////

ezMoveNodeCommand::ezMoveNodeCommand() = default;

ezStatus ezMoveNodeCommand::DoInternal(bool bRedo)
{
  ezDocument* pDocument = GetDocument();
  ezDocumentNodeManager* pManager = static_cast<ezDocumentNodeManager*>(pDocument->GetObjectManager());

  if (!bRedo)
  {
    m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
    if (m_pObject == nullptr)
      return ezStatus("Move Node: The given object does not exist!");

    m_OldPos = pManager->GetNodePos(m_pObject);
    EZ_SUCCEED_OR_RETURN(pManager->CanMoveNode(m_pObject, m_NewPos));
  }

  pManager->MoveNode(m_pObject, m_NewPos);
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezMoveNodeCommand::UndoInternal(bool bFireEvents)
{
  ezDocument* pDocument = GetDocument();
  ezDocumentNodeManager* pManager = static_cast<ezDocumentNodeManager*>(pDocument->GetObjectManager());
  EZ_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");

  EZ_SUCCEED_OR_RETURN(pManager->CanMoveNode(m_pObject, m_OldPos));

  pManager->MoveNode(m_pObject, m_OldPos);

  return ezStatus(EZ_SUCCESS);
}


////////////////////////////////////////////////////////////////////////
// ezConnectNodePinsCommand
////////////////////////////////////////////////////////////////////////

ezConnectNodePinsCommand::ezConnectNodePinsCommand() = default;

ezStatus ezConnectNodePinsCommand::DoInternal(bool bRedo)
{
  ezDocument* pDocument = GetDocument();
  ezDocumentNodeManager* pManager = static_cast<ezDocumentNodeManager*>(pDocument->GetObjectManager());

  if (!bRedo)
  {
    m_pConnectionObject = pManager->GetObject(m_ConnectionObject);
    if (!pManager->IsConnection(m_pConnectionObject))
      return ezStatus("Connect Node Pins: The given connection object is not valid connection!");

    m_pObjectSource = pManager->GetObject(m_ObjectSource);
    if (m_pObjectSource == nullptr)
      return ezStatus("Connect Node Pins: The given node does not exist!");
    m_pObjectTarget = pManager->GetObject(m_ObjectTarget);
    if (m_pObjectTarget == nullptr)
      return ezStatus("Connect Node Pins: The given node does not exist!");
  }

  const ezPin* pOutput = pManager->GetOutputPinByName(m_pObjectSource, m_sSourcePin);
  if (pOutput == nullptr)
    return ezStatus("Connect Node Pins: The given pin does not exist!");

  const ezPin* pInput = pManager->GetInputPinByName(m_pObjectTarget, m_sTargetPin);
  if (pInput == nullptr)
    return ezStatus("Connect Node Pins: The given pin does not exist!");

  ezDocumentNodeManager::CanConnectResult res;
  EZ_SUCCEED_OR_RETURN(pManager->CanConnect(*pOutput, *pInput, res));

  pManager->Connect(m_pConnectionObject, *pOutput, *pInput);
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezConnectNodePinsCommand::UndoInternal(bool bFireEvents)
{
  ezDocument* pDocument = GetDocument();
  ezDocumentNodeManager* pManager = static_cast<ezDocumentNodeManager*>(pDocument->GetObjectManager());

  EZ_SUCCEED_OR_RETURN(pManager->CanDisconnect(m_pConnectionObject));

  pManager->Disconnect(m_pConnectionObject);
  return ezStatus(EZ_SUCCESS);
}


////////////////////////////////////////////////////////////////////////
// ezDisconnectNodePinsCommand
////////////////////////////////////////////////////////////////////////

ezDisconnectNodePinsCommand::ezDisconnectNodePinsCommand() = default;

ezStatus ezDisconnectNodePinsCommand::DoInternal(bool bRedo)
{
  ezDocument* pDocument = GetDocument();
  ezDocumentNodeManager* pManager = static_cast<ezDocumentNodeManager*>(pDocument->GetObjectManager());

  if (!bRedo)
  {
    m_pConnectionObject = pManager->GetObject(m_ConnectionObject);
    if (!pManager->IsConnection(m_pConnectionObject))
      return ezStatus("Disconnect Node Pins: The given connection object is not valid connection!");

    EZ_SUCCEED_OR_RETURN(pManager->CanRemove(m_pConnectionObject));

    const ezConnection& connection = pManager->GetConnection(m_pConnectionObject);
    const ezPin& pinSource = connection.GetSourcePin();
    const ezPin& pinTarget = connection.GetTargetPin();

    m_pObjectSource = pinSource.GetParent();
    m_pObjectTarget = pinTarget.GetParent();
    m_sSourcePin = pinSource.GetName();
    m_sTargetPin = pinTarget.GetName();
  }

  EZ_SUCCEED_OR_RETURN(pManager->CanDisconnect(m_pConnectionObject));

  pManager->Disconnect(m_pConnectionObject);

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezDisconnectNodePinsCommand::UndoInternal(bool bFireEvents)
{
  ezDocument* pDocument = GetDocument();
  ezDocumentNodeManager* pManager = static_cast<ezDocumentNodeManager*>(pDocument->GetObjectManager());

  const ezPin* pOutput = pManager->GetOutputPinByName(m_pObjectSource, m_sSourcePin);
  if (pOutput == nullptr)
    return ezStatus("Connect Node: The given pin does not exist!");

  const ezPin* pInput = pManager->GetInputPinByName(m_pObjectTarget, m_sTargetPin);
  if (pInput == nullptr)
    return ezStatus("Connect Node: The given pin does not exist!");

  ezDocumentNodeManager::CanConnectResult res;
  EZ_SUCCEED_OR_RETURN(pManager->CanConnect(*pOutput, *pInput, res));

  pManager->Connect(m_pConnectionObject, *pOutput, *pInput);
  return ezStatus(EZ_SUCCESS);
}


////////////////////////////////////////////////////////////////////////
// ezNodeCommands
////////////////////////////////////////////////////////////////////////

// static
ezStatus ezNodeCommands::AddAndConnectCommand(ezCommandHistory* history, const ezRTTI* pConnectionType, const ezPin& sourcePin, const ezPin& targetPin)
{
  ezAddObjectCommand cmd;
  cmd.m_pType = pConnectionType;
  cmd.m_NewObjectGuid.CreateNewUuid();
  cmd.m_Index = -1;

  ezStatus res = history->AddCommand(cmd);
  if (res.m_Result.Succeeded())
  {
    ezConnectNodePinsCommand connect;
    connect.m_ConnectionObject = cmd.m_NewObjectGuid;
    connect.m_ObjectSource = sourcePin.GetParent()->GetGuid();
    connect.m_ObjectTarget = targetPin.GetParent()->GetGuid();
    connect.m_sSourcePin = sourcePin.GetName();
    connect.m_sTargetPin = targetPin.GetName();

    res = history->AddCommand(connect);
  }

  return res;
}

// static
ezStatus ezNodeCommands::DisconnectAndRemoveCommand(ezCommandHistory* history, const ezUuid& connectionObject)
{
  ezDisconnectNodePinsCommand cmd;
  cmd.m_ConnectionObject = connectionObject;

  ezStatus res = history->AddCommand(cmd);
  if (res.m_Result.Succeeded())
  {
    ezRemoveObjectCommand remove;
    remove.m_Object = cmd.m_ConnectionObject;

    res = history->AddCommand(remove);
  }

  return res;
}


