#include <PCH.h>
#include <ToolsFoundation/Command/NodeCommands.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRemoveNodeCommand, 1, ezRTTIDefaultAllocator<ezRemoveNodeCommand>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ObjectGuid", m_Object),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMoveNodeCommand, 1, ezRTTIDefaultAllocator<ezMoveNodeCommand>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ObjectGuid", m_Object),
    EZ_MEMBER_PROPERTY("NewPos", m_NewPos),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezConnectNodePinsCommand, 1, ezRTTIDefaultAllocator<ezConnectNodePinsCommand>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("SourceGuid", m_ObjectSource),
    EZ_MEMBER_PROPERTY("TargetGuid", m_ObjectTarget),
    EZ_MEMBER_PROPERTY("SourcePin", m_sSourcePin),
    EZ_MEMBER_PROPERTY("TargetPin", m_sTargetPin),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDisconnectNodePinsCommand, 1, ezRTTIDefaultAllocator<ezDisconnectNodePinsCommand>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("SourceGuid", m_ObjectSource),
    EZ_MEMBER_PROPERTY("TargetGuid", m_ObjectTarget),
    EZ_MEMBER_PROPERTY("SourcePin", m_sSourcePin),
    EZ_MEMBER_PROPERTY("TargetPin", m_sTargetPin),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

////////////////////////////////////////////////////////////////////////
// ezRemoveNodeCommand
////////////////////////////////////////////////////////////////////////

ezRemoveNodeCommand::ezRemoveNodeCommand() :
  m_pObject(nullptr)
{
}

ezStatus ezRemoveNodeCommand::DoInternal(bool bRedo)
{
  ezDocument* pDocument = GetDocument();

  if (!bRedo)
  {
    ezDocumentNodeManager* pManager = static_cast<ezDocumentNodeManager*>(pDocument->GetObjectManager());
    m_pObject = pManager->GetObject(m_Object);
    if (m_pObject == nullptr)
      return ezStatus("Remove Node: The given object does not exist!");

    auto inputs = pManager->GetInputPins(m_pObject);
    for (const ezPin* pPinTarget : inputs)
    {
      while (true)
      {
        auto outputs = pPinTarget->GetConnections();

        if (outputs.IsEmpty())
          break;

        const ezPin* pPinSource = outputs[0]->GetSourcePin();
        ezDisconnectNodePinsCommand cmd;
        cmd.m_ObjectSource = pPinSource->GetParent()->GetGuid();
        cmd.m_ObjectTarget = pPinTarget->GetParent()->GetGuid();
        cmd.m_sSourcePin = pPinSource->GetName();
        cmd.m_sTargetPin = pPinTarget->GetName();
        auto res = AddSubCommand(cmd);
        if (res.m_Result.Failed())
        {
          return res;
        }
      }
    }

    auto outputs = pManager->GetOutputPins(m_pObject);
    for (const ezPin* pPinSource : outputs)
    {
      while (true)
      {
        auto inputs = pPinSource->GetConnections();

        if (inputs.IsEmpty())
          break;

        const ezPin* pPinTarget = inputs[0]->GetTargetPin();
        ezDisconnectNodePinsCommand cmd;
        cmd.m_ObjectSource = pPinSource->GetParent()->GetGuid();
        cmd.m_ObjectTarget = pPinTarget->GetParent()->GetGuid();
        cmd.m_sSourcePin = pPinSource->GetName();
        cmd.m_sTargetPin = pPinTarget->GetName();
        auto res = AddSubCommand(cmd);
        if (res.m_Result.Failed())
        {
          return res;
        }
      }
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

void ezRemoveNodeCommand::CleanupInternal(CommandState state)
{
}


////////////////////////////////////////////////////////////////////////
// ezMoveObjectCommand
////////////////////////////////////////////////////////////////////////

ezMoveNodeCommand::ezMoveNodeCommand()
{
  m_pObject = nullptr;
  m_NewPos = ezVec2::ZeroVector();
  m_OldPos = ezVec2::ZeroVector();
}

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

ezConnectNodePinsCommand::ezConnectNodePinsCommand()
{
  m_pObjectSource = nullptr;
  m_pObjectTarget = nullptr;
}

ezStatus ezConnectNodePinsCommand::DoInternal(bool bRedo)
{
  ezDocument* pDocument = GetDocument();
  ezDocumentNodeManager* pManager = static_cast<ezDocumentNodeManager*>(pDocument->GetObjectManager());

  if (!bRedo)
  {
    m_pObjectSource = pManager->GetObject(m_ObjectSource);
    if (m_pObjectSource == nullptr)
      return ezStatus("Connect Node: The given node does not exist!");
    m_pObjectTarget = pManager->GetObject(m_ObjectTarget);
    if (m_pObjectTarget == nullptr)
      return ezStatus("Connect Node: The given node does not exist!");
  }

  const ezPin* pOutput = pManager->GetOutputPinByName(m_pObjectSource, m_sSourcePin);
  if (pOutput == nullptr)
    return ezStatus("Connect Node: The given pin does not exist!");

  const ezPin* pInput = pManager->GetInputPinByName(m_pObjectTarget, m_sTargetPin);
  if (pInput == nullptr)
    return ezStatus("Connect Node: The given pin does not exist!");

  ezDocumentNodeManager::CanConnectResult res;
  EZ_SUCCEED_OR_RETURN(pManager->CanConnect(pOutput, pInput, res));

  pManager->Connect(pOutput, pInput);
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezConnectNodePinsCommand::UndoInternal(bool bFireEvents)
{
  ezDocument* pDocument = GetDocument();
  ezDocumentNodeManager* pManager = static_cast<ezDocumentNodeManager*>(pDocument->GetObjectManager());

  const ezPin* pOutput = pManager->GetOutputPinByName(m_pObjectSource, m_sSourcePin);
  if (pOutput == nullptr)
    return ezStatus("Connect Node: The given pin does not exist!");

  const ezPin* pInput = pManager->GetInputPinByName(m_pObjectTarget, m_sTargetPin);
  if (pInput == nullptr)
    return ezStatus("Connect Node: The given pin does not exist!");

  EZ_SUCCEED_OR_RETURN(pManager->CanDisconnect(pOutput, pInput));

  pManager->Disconnect(pOutput, pInput);
  return ezStatus(EZ_SUCCESS);
}


////////////////////////////////////////////////////////////////////////
// ezDisconnectNodePinsCommand
////////////////////////////////////////////////////////////////////////

ezDisconnectNodePinsCommand::ezDisconnectNodePinsCommand()
{
  m_pObjectSource = nullptr;
  m_pObjectTarget = nullptr;
}

ezStatus ezDisconnectNodePinsCommand::DoInternal(bool bRedo)
{
  ezDocument* pDocument = GetDocument();
  ezDocumentNodeManager* pManager = static_cast<ezDocumentNodeManager*>(pDocument->GetObjectManager());

  if (!bRedo)
  {
    m_pObjectSource = pManager->GetObject(m_ObjectSource);
    if (m_pObjectSource == nullptr)
      return ezStatus("Connect Node: The given node does not exist!");
    m_pObjectTarget = pManager->GetObject(m_ObjectTarget);
    if (m_pObjectTarget == nullptr)
      return ezStatus("Connect Node: The given node does not exist!");
  }

  const ezPin* pOutput = pManager->GetOutputPinByName(m_pObjectSource, m_sSourcePin);
  if (pOutput == nullptr)
    return ezStatus("Connect Node: The given pin does not exist!");

  const ezPin* pInput = pManager->GetInputPinByName(m_pObjectTarget, m_sTargetPin);
  if (pInput == nullptr)
    return ezStatus("Connect Node: The given pin does not exist!");

  EZ_SUCCEED_OR_RETURN(pManager->CanDisconnect(pOutput, pInput));

  pManager->Disconnect(pOutput, pInput);
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
  EZ_SUCCEED_OR_RETURN(pManager->CanConnect(pOutput, pInput, res));

  pManager->Connect(pOutput, pInput);
  return ezStatus(EZ_SUCCESS);
}
