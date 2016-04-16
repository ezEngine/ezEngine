#include <ToolsFoundation/PCH.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/RttiConverter.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezConnection, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPin, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

////////////////////////////////////////////////////////////////////////
// ezDocumentNodeManager Internal
////////////////////////////////////////////////////////////////////////

struct ConnectionInternal
{
  ConnectionInternal() {}
  ConnectionInternal(const char* szPinSource, const ezUuid& target, const char* szPinTarget)
  : m_SourcePin(szPinSource), m_Target(target), m_TargetPin(szPinTarget) {}

  ezString m_SourcePin;
  ezUuid m_Target;
  ezString m_TargetPin;
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, ConnectionInternal);

EZ_BEGIN_STATIC_REFLECTED_TYPE(ConnectionInternal, ezNoBase, 1, ezRTTIDefaultAllocator<ConnectionInternal>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("SourcePin", m_SourcePin),
    EZ_MEMBER_PROPERTY("Target", m_Target),
    EZ_MEMBER_PROPERTY("TargetPin", m_TargetPin),
  }
  EZ_END_PROPERTIES
}
EZ_END_STATIC_REFLECTED_TYPE

struct NodeDataInternal
{
  ezVec2 m_NodePos;
  ezDynamicArray<ConnectionInternal> m_Connections;
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, NodeDataInternal);

EZ_BEGIN_STATIC_REFLECTED_TYPE(NodeDataInternal, ezNoBase, 1, ezRTTIDefaultAllocator<NodeDataInternal>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Node::Pos", m_NodePos),
    EZ_ARRAY_MEMBER_PROPERTY("Node::Connections", m_Connections),
  }
  EZ_END_PROPERTIES
}
EZ_END_STATIC_REFLECTED_TYPE

////////////////////////////////////////////////////////////////////////
// ezDocumentNodeManager
////////////////////////////////////////////////////////////////////////

ezDocumentNodeManager::ezDocumentNodeManager()
{
  m_ObjectEvents.AddEventHandler(ezMakeDelegate(&ezDocumentNodeManager::ObjectHandler, this));
  m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezDocumentNodeManager::StructureEventHandler, this));
}

ezDocumentNodeManager::~ezDocumentNodeManager()
{
  m_ObjectEvents.RemoveEventHandler(ezMakeDelegate(&ezDocumentNodeManager::ObjectHandler, this));
  m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezDocumentNodeManager::StructureEventHandler, this));

  EZ_ASSERT_DEV(m_PinsToConnection.IsEmpty(), "Not all pins have been destroyed!");
  
}

void ezDocumentNodeManager::DestroyAllObjects()
{
  for (auto it = m_PinsToConnection.GetIterator(); it.IsValid(); ++it)
  {
    InternalDestroyConnection(it.Value());
  }
  m_PinsToConnection.Clear();
  ezDocumentObjectManager::DestroyAllObjects();
}

ezVec2 ezDocumentNodeManager::GetNodePos(const ezDocumentObject* pObject) const
{
  EZ_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  auto it = m_ObjectToNode.Find(pObject->GetGuid());
  EZ_ASSERT_DEV(it.IsValid(), "Can't get pos of objects that aren't nodes!");
  return it.Value().m_vPos;
}

const ezPin* ezDocumentNodeManager::GetInputPinByName(const ezDocumentObject* pObject, const char* szName) const
{
  EZ_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  auto it = m_ObjectToNode.Find(pObject->GetGuid());
  EZ_ASSERT_DEV(it.IsValid(), "Can't get input pins of objects that aren't nodes!");
  for (auto pPin : it.Value().m_Inputs)
  {
    if (ezStringUtils::IsEqual(pPin->GetName(), szName))
      return pPin;
  }
  return nullptr;
}

const ezPin* ezDocumentNodeManager::GetOutputPinByName(const ezDocumentObject* pObject, const char* szName) const
{
  EZ_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  auto it = m_ObjectToNode.Find(pObject->GetGuid());
  EZ_ASSERT_DEV(it.IsValid(), "Can't get input pins of objects that aren't nodes!");
  for (auto pPin : it.Value().m_Outputs)
  {
    if (ezStringUtils::IsEqual(pPin->GetName(), szName))
      return pPin;
  }
  return nullptr;
}

const ezArrayPtr<ezPin* const> ezDocumentNodeManager::GetInputPins(const ezDocumentObject* pObject) const
{
  EZ_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  auto it = m_ObjectToNode.Find(pObject->GetGuid());
  EZ_ASSERT_DEV(it.IsValid(), "Can't get input pins of objects that aren't nodes!");
  return it.Value().m_Inputs;
}

const ezArrayPtr<ezPin* const> ezDocumentNodeManager::GetOutputPins(const ezDocumentObject* pObject) const
{
  EZ_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  auto it = m_ObjectToNode.Find(pObject->GetGuid());
  EZ_ASSERT_DEV(it.IsValid(), "Can't get input pins of objects that aren't nodes!");
  return it.Value().m_Outputs;
}

const ezConnection* ezDocumentNodeManager::GetConnection(const ezPin* pSource, const ezPin* pTarget) const
{
  auto it = m_PinsToConnection.Find(PinTuple(pSource, pTarget));
  return it.IsValid() ? it.Value() : nullptr;
}

bool ezDocumentNodeManager::IsNode(const ezDocumentObject* pObject) const
{
  EZ_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  if (pObject == nullptr)
    return false;
  if (pObject == GetRootObject())
    return false;

  return InternalIsNode(pObject);
}

ezStatus ezDocumentNodeManager::CanConnect(const ezPin* pSource, const ezPin* pTarget) const
{
  EZ_ASSERT_DEV(pSource != nullptr, "Invalid input!");
  EZ_ASSERT_DEV(pTarget != nullptr, "Invalid input!");
  if (pSource->m_Type != ezPin::Type::Output)
    return ezStatus("Source pin is not an output pin!");
  if (pTarget->m_Type != ezPin::Type::Input)
    return ezStatus("Target pin is not an input pin!");

  if (pSource->m_pParent == pTarget->m_pParent)
    return ezStatus("Can't connect node to itself!");

  if (m_PinsToConnection.Contains(PinTuple(pSource, pTarget)))
    return ezStatus("Pins already connected!");

  return InternalCanConnect(pSource, pTarget);
}

ezStatus ezDocumentNodeManager::CanDisconnect(const ezConnection* pConnection) const
{
  EZ_ASSERT_DEV(pConnection != nullptr, "Invalid input!");
  return CanDisconnect(pConnection->m_pSourcePin, pConnection->m_pTargetPin);
}

ezStatus ezDocumentNodeManager::CanDisconnect(const ezPin* pSource, const ezPin* pTarget) const
{
  EZ_ASSERT_DEV(pSource != nullptr, "Invalid input!");
  EZ_ASSERT_DEV(pTarget != nullptr, "Invalid input!");
  if (pSource->m_Type != ezPin::Type::Output)
    return ezStatus("Source pin is not an output pin!");
  if (pTarget->m_Type != ezPin::Type::Input)
    return ezStatus("Target pin is not an input pin!");

  if (!m_PinsToConnection.Contains(PinTuple(pSource, pTarget)))
    return ezStatus("Pins not connected!");

  return InternalCanDisconnect(pSource, pTarget);
}

ezStatus ezDocumentNodeManager::CanMoveNode(const ezDocumentObject* pObject, const ezVec2& vPos) const
{
  EZ_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  if (!IsNode(pObject))
    return ezStatus("The given object is not a node!");

  return InternalCanMoveNode(pObject, vPos);
}

void ezDocumentNodeManager::Connect(const ezPin* pSource, const ezPin* pTarget)
{
  EZ_ASSERT_DEBUG(CanConnect(pSource, pTarget).m_Result.Succeeded(), "Connect: Sanity check failed!");

  ezConnection* pConnection = InternalCreateConnection(pSource, pTarget);
  pConnection->m_pSourcePin = pSource;
  pConnection->m_pTargetPin = pTarget;
  m_PinsToConnection.Insert(PinTuple(pSource, pTarget), pConnection);

  const_cast<ezPin*>(pSource)->m_Connections.PushBack(pConnection);
  const_cast<ezPin*>(pTarget)->m_Connections.PushBack(pConnection);

  {
    ezDocumentNodeManagerEvent e(ezDocumentNodeManagerEvent::Type::AfterPinsConnected, nullptr, pConnection);
    m_NodeEvents.Broadcast(e);
  }
}

void ezDocumentNodeManager::Disconnect(const ezPin* pSource, const ezPin* pTarget)
{
  EZ_ASSERT_DEBUG(CanDisconnect(pSource, pTarget).m_Result.Succeeded(), "Disconnect: Sanity check failed!");

  auto it = m_PinsToConnection.Find(PinTuple(pSource, pTarget));
  ezConnection* pConnection = it.Value();
  {
    ezDocumentNodeManagerEvent e(ezDocumentNodeManagerEvent::Type::BeforePinsDisonnected, nullptr, pConnection);
    m_NodeEvents.Broadcast(e);
  }

  const_cast<ezPin*>(pSource)->m_Connections.Remove(pConnection);
  const_cast<ezPin*>(pTarget)->m_Connections.Remove(pConnection);
  m_PinsToConnection.Remove(it);
  InternalDestroyConnection(pConnection);
}

void ezDocumentNodeManager::MoveNode(ezDocumentObject* pObject, const ezVec2& vPos)
{
  EZ_ASSERT_DEBUG(CanMoveNode(pObject, vPos).m_Result.Succeeded(), "MoveNode: Sanity check failed!");

  auto it = m_ObjectToNode.Find(pObject->GetGuid());
  EZ_ASSERT_DEBUG(it.IsValid(), "Moveable node does not exist, CanMoveNode impl invalid!");
  it.Value().m_vPos = vPos;

  ezDocumentNodeManagerEvent e(ezDocumentNodeManagerEvent::Type::NodeMoved, pObject);
  m_NodeEvents.Broadcast(e);
}

void ezDocumentNodeManager::AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph)
{
  auto& AllNodes = graph.GetAllNodes();

  auto pType = ezGetStaticRTTI<NodeDataInternal>();
  NodeDataInternal data;
  ezRttiConverterContext context;
  ezRttiConverterWriter rttiConverter(&graph, &context, true, true);

  for (auto it = AllNodes.GetIterator(); it.IsValid(); ++it)
  {
    auto* pNode = it.Value();
    const ezUuid& guid = pNode->GetGuid();

    auto it2 = m_ObjectToNode.Find(guid);
    if (it2.IsValid())
    {
      const NodeInternal& node = it2.Value();

      data.m_NodePos = node.m_vPos;
      data.m_Connections.Clear();

      const ezDocumentObject* pObject = GetObject(guid);
      auto outputs = GetOutputPins(pObject);
      for (const ezPin* pPinSource : outputs)
      {
        auto connections = pPinSource->GetConnections();
        for (const ezConnection* pConnection : connections)
        {
          const ezPin* pPinTarget = pConnection->m_pTargetPin;      
          data.m_Connections.PushBack(ConnectionInternal(pPinSource->GetName(), pPinTarget->GetParent()->GetGuid(), pPinTarget->GetName()));
        }
      }

      rttiConverter.AddProperties(pNode, pType, &data);
    }
  }

}

void ezDocumentNodeManager::RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph)
{
  auto& AllNodes = graph.GetAllNodes();

  auto pType = ezGetStaticRTTI<NodeDataInternal>();
  NodeDataInternal data;
  
  ezRttiConverterContext context;
  ezRttiConverterReader rttiConverter(&graph, &context);

  for (auto it = AllNodes.GetIterator(); it.IsValid(); ++it)
  {
    auto* pNode = it.Value();
    const ezUuid& guid = pNode->GetGuid();

    ezDocumentObject* pObject = GetObject(guid);

    if (pObject)
    {
      if (IsNode(pObject))
      {
        data.m_Connections.Clear();
        rttiConverter.ApplyPropertiesToObject(pNode, pType, &data);

        if (CanMoveNode(pObject, data.m_NodePos).m_Result.Succeeded())
        {
          MoveNode(pObject, data.m_NodePos);
        }

        for (const auto& con : data.m_Connections)
        {
          ezDocumentObject* pTarget = GetObject(con.m_Target);
          if (pTarget)
          {
            const ezPin* pSourcePin = GetOutputPinByName(pObject, con.m_SourcePin);
            const ezPin* pTargetPin = GetInputPinByName(pTarget, con.m_TargetPin);

            if (CanConnect(pSourcePin, pTargetPin).m_Result.Succeeded())
            {
              Connect(pSourcePin, pTargetPin);
            }
          }
        }
      }
    }
  }
}


void ezDocumentNodeManager::ObjectHandler(const ezDocumentObjectEvent& e)
{
  switch (e.m_EventType)
  {
  case ezDocumentObjectEvent::Type::AfterObjectCreated:
    {
      if (IsNode(e.m_pObject))
      {
        EZ_ASSERT_DEBUG(!m_ObjectToNode.Contains(e.m_pObject->GetGuid()), "Sanity check failed!");
        m_ObjectToNode[e.m_pObject->GetGuid()] = NodeInternal();
        InternalCreatePins(e.m_pObject, m_ObjectToNode[e.m_pObject->GetGuid()]);
        // TODO: Sanity check pins (duplicate names etc).
      }
    }
    break;
  case ezDocumentObjectEvent::Type::BeforeObjectDestroyed:
    {
      if (IsNode(e.m_pObject))
      {
        auto it = m_ObjectToNode.Find(e.m_pObject->GetGuid());
        EZ_ASSERT_DEBUG(it.IsValid(), "Sanity check failed!");
        
        InternalDestroyPins(e.m_pObject, it.Value());       
        m_ObjectToNode.Remove(e.m_pObject->GetGuid());
      }
    }
    break;
  }
}

void ezDocumentNodeManager::StructureEventHandler(const ezDocumentObjectStructureEvent& e)
{
  switch (e.m_EventType)
  {
  case ezDocumentObjectStructureEvent::Type::BeforeObjectAdded:
    {
      if (IsNode(e.m_pObject))
      {
        ezDocumentNodeManagerEvent e2(ezDocumentNodeManagerEvent::Type::BeforeNodeAdded, e.m_pObject);
        m_NodeEvents.Broadcast(e2);
      }
    }
    break;
  case ezDocumentObjectStructureEvent::Type::AfterObjectAdded:
    {
      if (IsNode(e.m_pObject))
      {
        ezDocumentNodeManagerEvent e2(ezDocumentNodeManagerEvent::Type::AfterNodeAdded, e.m_pObject);
        m_NodeEvents.Broadcast(e2);
      }
    }
    break;
  case ezDocumentObjectStructureEvent::Type::BeforeObjectRemoved:
    {
      if (IsNode(e.m_pObject))
      {
        ezDocumentNodeManagerEvent e2(ezDocumentNodeManagerEvent::Type::BeforeNodeRemoved, e.m_pObject);
        m_NodeEvents.Broadcast(e2);
      }
    }
    break;
  case ezDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    {
      if (IsNode(e.m_pObject))
      {
        ezDocumentNodeManagerEvent e2(ezDocumentNodeManagerEvent::Type::AfterNodeRemoved, e.m_pObject);
        m_NodeEvents.Broadcast(e2);
      }
    }
    break;
  }
}
