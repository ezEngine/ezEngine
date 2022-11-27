#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <ToolsFoundation/Command/NodeCommands.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPin, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

////////////////////////////////////////////////////////////////////////
// ezDocumentNodeManager Internal
////////////////////////////////////////////////////////////////////////

struct DocumentNodeManager_NodeMetaData
{
  ezVec2 m_Pos = ezVec2::ZeroVector();
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, DocumentNodeManager_NodeMetaData);

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(DocumentNodeManager_NodeMetaData, ezNoBase, 1, ezRTTIDefaultAllocator<DocumentNodeManager_NodeMetaData>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Node::Pos", m_Pos),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

struct DocumentNodeManager_ConnectionMetaData
{
  ezUuid m_Source;
  ezUuid m_Target;
  ezString m_SourcePin;
  ezString m_TargetPin;
};
EZ_DECLARE_REFLECTABLE_TYPE(EZ_NO_LINKAGE, DocumentNodeManager_ConnectionMetaData);

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(DocumentNodeManager_ConnectionMetaData, ezNoBase, 1, ezRTTIDefaultAllocator<DocumentNodeManager_ConnectionMetaData>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Connection::Source", m_Source),
    EZ_MEMBER_PROPERTY("Connection::Target", m_Target),
    EZ_MEMBER_PROPERTY("Connection::SourcePin", m_SourcePin),    
    EZ_MEMBER_PROPERTY("Connection::TargetPin", m_TargetPin),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

class DocumentNodeManager_DefaultConnection : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(DocumentNodeManager_DefaultConnection, ezReflectedClass);
};

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(DocumentNodeManager_DefaultConnection, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

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
}

const ezRTTI* ezDocumentNodeManager::GetConnectionType() const
{
  return ezGetStaticRTTI<DocumentNodeManager_DefaultConnection>();
}

ezVec2 ezDocumentNodeManager::GetNodePos(const ezDocumentObject* pObject) const
{
  EZ_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  auto it = m_ObjectToNode.Find(pObject->GetGuid());
  EZ_ASSERT_DEV(it.IsValid(), "Can't get pos of objects that aren't nodes!");
  return it.Value().m_vPos;
}

const ezConnection& ezDocumentNodeManager::GetConnection(const ezDocumentObject* pObject) const
{
  EZ_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  auto it = m_ObjectToConnection.Find(pObject->GetGuid());
  EZ_ASSERT_DEV(it.IsValid(), "Can't get connection for objects that aren't connections!");
  return *it.Value();
}

const ezPin* ezDocumentNodeManager::GetInputPinByName(const ezDocumentObject* pObject, const char* szName) const
{
  EZ_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  auto it = m_ObjectToNode.Find(pObject->GetGuid());
  EZ_ASSERT_DEV(it.IsValid(), "Can't get input pins of objects that aren't nodes!");
  for (auto& pPin : it.Value().m_Inputs)
  {
    if (ezStringUtils::IsEqual(pPin->GetName(), szName))
      return pPin.Borrow();
  }
  return nullptr;
}

const ezPin* ezDocumentNodeManager::GetOutputPinByName(const ezDocumentObject* pObject, const char* szName) const
{
  EZ_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  auto it = m_ObjectToNode.Find(pObject->GetGuid());
  EZ_ASSERT_DEV(it.IsValid(), "Can't get input pins of objects that aren't nodes!");
  for (auto& pPin : it.Value().m_Outputs)
  {
    if (ezStringUtils::IsEqual(pPin->GetName(), szName))
      return pPin.Borrow();
  }
  return nullptr;
}

ezArrayPtr<const ezUniquePtr<const ezPin>> ezDocumentNodeManager::GetInputPins(const ezDocumentObject* pObject) const
{
  EZ_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  auto it = m_ObjectToNode.Find(pObject->GetGuid());
  EZ_ASSERT_DEV(it.IsValid(), "Can't get input pins of objects that aren't nodes!");
  return ezMakeArrayPtr((ezUniquePtr<const ezPin>*)it.Value().m_Inputs.GetData(), it.Value().m_Inputs.GetCount());
}

ezArrayPtr<const ezUniquePtr<const ezPin>> ezDocumentNodeManager::GetOutputPins(const ezDocumentObject* pObject) const
{
  EZ_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  auto it = m_ObjectToNode.Find(pObject->GetGuid());
  EZ_ASSERT_DEV(it.IsValid(), "Can't get input pins of objects that aren't nodes!");
  return ezMakeArrayPtr((ezUniquePtr<const ezPin>*)it.Value().m_Outputs.GetData(), it.Value().m_Outputs.GetCount());
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

bool ezDocumentNodeManager::IsConnection(const ezDocumentObject* pObject) const
{
  EZ_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  if (pObject == nullptr)
    return false;
  if (pObject == GetRootObject())
    return false;

  return InternalIsConnection(pObject);
}

ezArrayPtr<const ezConnection* const> ezDocumentNodeManager::GetConnections(const ezPin& pin) const
{
  auto it = m_Connections.Find(&pin);
  if (it.IsValid())
  {
    return it.Value();
  }

  return ezArrayPtr<const ezConnection* const>();
}

bool ezDocumentNodeManager::HasConnections(const ezPin& pin) const
{
  auto it = m_Connections.Find(&pin);
  return it.IsValid() && it.Value().IsEmpty() == false;
}

bool ezDocumentNodeManager::IsConnected(const ezPin& source, const ezPin& target) const
{
  auto it = m_Connections.Find(&source);
  if (it.IsValid())
  {
    for (auto pConnection : it.Value())
    {
      if (&pConnection->GetTargetPin() == &target)
        return true;
    }
  }

  return false;
}

ezStatus ezDocumentNodeManager::CanConnect(const ezRTTI* pObjectType, const ezPin& source, const ezPin& target, CanConnectResult& out_result) const
{
  out_result = CanConnectResult::ConnectNever;

  if (pObjectType == nullptr || pObjectType->IsDerivedFrom(GetConnectionType()) == false)
    return ezStatus("Invalid connection object type");

  if (source.m_Type != ezPin::Type::Output)
    return ezStatus("Source pin is not an output pin.");
  if (target.m_Type != ezPin::Type::Input)
    return ezStatus("Target pin is not an input pin.");

  if (source.m_pParent == target.m_pParent)
    return ezStatus("Nodes cannot be connect with themselves.");

  if (IsConnected(source, target))
    return ezStatus("Pins already connected.");

  return InternalCanConnect(source, target, out_result);
}

ezStatus ezDocumentNodeManager::CanDisconnect(const ezConnection* pConnection) const
{
  if (pConnection == nullptr)
    return ezStatus("Invalid connection");

  return InternalCanDisconnect(pConnection->GetSourcePin(), pConnection->GetTargetPin());
}

ezStatus ezDocumentNodeManager::CanDisconnect(const ezDocumentObject* pObject) const
{
  if (!IsConnection(pObject))
    return ezStatus("Invalid connection object");

  const ezConnection& connection = GetConnection(pObject);
  return InternalCanDisconnect(connection.GetSourcePin(), connection.GetTargetPin());
}

ezStatus ezDocumentNodeManager::CanMoveNode(const ezDocumentObject* pObject, const ezVec2& vPos) const
{
  EZ_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  if (!IsNode(pObject))
    return ezStatus("The given object is not a node!");

  return InternalCanMoveNode(pObject, vPos);
}

void ezDocumentNodeManager::Connect(const ezDocumentObject* pObject, const ezPin& source, const ezPin& target)
{
  ezDocumentNodeManager::CanConnectResult res = CanConnectResult::ConnectNever;
  EZ_IGNORE_UNUSED(res);
  EZ_ASSERT_DEBUG(CanConnect(pObject->GetType(), source, target, res).m_Result.Succeeded(), "Connect: Sanity check failed!");

  auto pConnection = EZ_DEFAULT_NEW(ezConnection, source, target, pObject);
  m_ObjectToConnection.Insert(pObject->GetGuid(), pConnection);

  m_Connections[&source].PushBack(pConnection);
  m_Connections[&target].PushBack(pConnection);

  {
    ezDocumentNodeManagerEvent e(ezDocumentNodeManagerEvent::Type::AfterPinsConnected, pObject);
    m_NodeEvents.Broadcast(e);
  }
}

void ezDocumentNodeManager::Disconnect(const ezDocumentObject* pObject)
{
  auto it = m_ObjectToConnection.Find(pObject->GetGuid());
  EZ_ASSERT_DEBUG(it.IsValid(), "Sanity check failed!");
  EZ_ASSERT_DEBUG(CanDisconnect(pObject).m_Result.Succeeded(), "Disconnect: Sanity check failed!");

  {
    ezDocumentNodeManagerEvent e(ezDocumentNodeManagerEvent::Type::BeforePinsDisonnected, pObject);
    m_NodeEvents.Broadcast(e);
  }

  auto& pConnection = it.Value();
  const ezPin& source = pConnection->GetSourcePin();
  const ezPin& target = pConnection->GetTargetPin();
  m_Connections[&source].RemoveAndCopy(pConnection.Borrow());
  m_Connections[&target].RemoveAndCopy(pConnection.Borrow());

  m_ObjectToConnection.Remove(it);
}

void ezDocumentNodeManager::MoveNode(const ezDocumentObject* pObject, const ezVec2& vPos)
{
  EZ_ASSERT_DEBUG(CanMoveNode(pObject, vPos).m_Result.Succeeded(), "MoveNode: Sanity check failed!");

  auto it = m_ObjectToNode.Find(pObject->GetGuid());
  EZ_ASSERT_DEBUG(it.IsValid(), "Moveable node does not exist, CanMoveNode impl invalid!");
  it.Value().m_vPos = vPos;

  ezDocumentNodeManagerEvent e(ezDocumentNodeManagerEvent::Type::NodeMoved, pObject);
  m_NodeEvents.Broadcast(e);
}

void ezDocumentNodeManager::AttachMetaDataBeforeSaving(ezAbstractObjectGraph& ref_graph) const
{
  auto pNodeMetaDataType = ezGetStaticRTTI<DocumentNodeManager_NodeMetaData>();
  auto pConnectionMetaDataType = ezGetStaticRTTI<DocumentNodeManager_ConnectionMetaData>();

  ezRttiConverterContext context;
  ezRttiConverterWriter rttiConverter(&ref_graph, &context, true, true);

  for (auto it = ref_graph.GetAllNodes().GetIterator(); it.IsValid(); ++it)
  {
    auto* pAbstractObject = it.Value();
    const ezUuid& guid = pAbstractObject->GetGuid();

    {
      auto it2 = m_ObjectToNode.Find(guid);
      if (it2.IsValid())
      {
        const NodeInternal& node = it2.Value();

        DocumentNodeManager_NodeMetaData nodeMetaData;
        nodeMetaData.m_Pos = node.m_vPos;
        rttiConverter.AddProperties(pAbstractObject, pNodeMetaDataType, &nodeMetaData);
      }
    }

    {
      auto it2 = m_ObjectToConnection.Find(guid);
      if (it2.IsValid())
      {
        const ezConnection& connection = *it2.Value();
        const ezPin& sourcePin = connection.GetSourcePin();
        const ezPin& targetPin = connection.GetTargetPin();

        DocumentNodeManager_ConnectionMetaData connectionMetaData;
        connectionMetaData.m_Source = sourcePin.GetParent()->GetGuid();
        connectionMetaData.m_Target = targetPin.GetParent()->GetGuid();
        connectionMetaData.m_SourcePin = sourcePin.GetName();
        connectionMetaData.m_TargetPin = targetPin.GetName();
        rttiConverter.AddProperties(pAbstractObject, pConnectionMetaDataType, &connectionMetaData);
      }
    }
  }
}

void ezDocumentNodeManager::RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph, bool bUndoable)
{
  ezCommandHistory* history = GetDocument()->GetCommandHistory();

  auto pNodeMetaDataType = ezGetStaticRTTI<DocumentNodeManager_NodeMetaData>();
  auto pConnectionMetaDataType = ezGetStaticRTTI<DocumentNodeManager_ConnectionMetaData>();

  ezRttiConverterContext context;
  ezRttiConverterReader rttiConverter(&graph, &context);

  for (auto it : graph.GetAllNodes())
  {
    auto pAbstractObject = it.Value();
    ezDocumentObject* pObject = GetObject(pAbstractObject->GetGuid());
    if (pObject == nullptr)
      continue;

    if (IsNode(pObject))
    {
      DocumentNodeManager_NodeMetaData nodeMetaData;
      rttiConverter.ApplyPropertiesToObject(pAbstractObject, pNodeMetaDataType, &nodeMetaData);

      if (CanMoveNode(pObject, nodeMetaData.m_Pos).m_Result.Succeeded())
      {
        if (bUndoable)
        {
          ezMoveNodeCommand move;
          move.m_Object = pObject->GetGuid();
          move.m_NewPos = nodeMetaData.m_Pos;
          history->AddCommand(move);
        }
        else
        {
          MoveNode(pObject, nodeMetaData.m_Pos);
        }
      }

      // Backwards compatibility to old file format
      if (auto pOldConnections = pAbstractObject->FindProperty("Node::Connections"))
      {
        EZ_ASSERT_DEV(bUndoable == false, "Undo not supported for old file format");
        RestoreOldMetaDataAfterLoading(graph, *pOldConnections, pObject);
      }
    }
    else if (IsConnection(pObject))
    {
      DocumentNodeManager_ConnectionMetaData connectionMetaData;
      rttiConverter.ApplyPropertiesToObject(pAbstractObject, pConnectionMetaDataType, &connectionMetaData);

      ezDocumentObject* pSource = GetObject(connectionMetaData.m_Source);
      ezDocumentObject* pTarget = GetObject(connectionMetaData.m_Target);
      if (pSource == nullptr || pTarget == nullptr)
        continue;

      const ezPin* pSourcePin = GetOutputPinByName(pSource, connectionMetaData.m_SourcePin);
      const ezPin* pTargetPin = GetInputPinByName(pTarget, connectionMetaData.m_TargetPin);
      if (pSourcePin == nullptr || pTargetPin == nullptr)
        continue;

      ezDocumentNodeManager::CanConnectResult res;
      if (CanConnect(pObject->GetType(), *pSourcePin, *pTargetPin, res).m_Result.Succeeded())
      {
        if (bUndoable)
        {
          ezConnectNodePinsCommand cmd;
          cmd.m_ConnectionObject = pObject->GetGuid();
          cmd.m_ObjectSource = connectionMetaData.m_Source;
          cmd.m_ObjectTarget = connectionMetaData.m_Target;
          cmd.m_sSourcePin = connectionMetaData.m_SourcePin;
          cmd.m_sTargetPin = connectionMetaData.m_TargetPin;
          history->AddCommand(cmd);
        }
        else
        {
          Connect(pObject, *pSourcePin, *pTargetPin);
        }
      }
    }
  }
}

void ezDocumentNodeManager::GetMetaDataHash(const ezDocumentObject* pObject, ezUInt64& inout_uiHash) const
{
  if (IsNode(pObject))
  {
    // The node position is not hashed here since the hash is only used for asset transform
    // and for that the node position is irrelevant.
  }
  else if (IsConnection(pObject))
  {
    const ezConnection& connection = GetConnection(pObject);
    const ezPin& sourcePin = connection.GetSourcePin();
    const ezPin& targetPin = connection.GetTargetPin();

    inout_uiHash = ezHashingUtils::xxHash64(&sourcePin.GetParent()->GetGuid(), sizeof(ezUuid), inout_uiHash);
    inout_uiHash = ezHashingUtils::xxHash64(&targetPin.GetParent()->GetGuid(), sizeof(ezUuid), inout_uiHash);
    inout_uiHash = ezHashingUtils::xxHash64String(sourcePin.GetName(), inout_uiHash);
    inout_uiHash = ezHashingUtils::xxHash64String(targetPin.GetName(), inout_uiHash);
  }
}

bool ezDocumentNodeManager::CopySelectedObjects(ezAbstractObjectGraph& out_objectGraph) const
{
  const auto& selection = GetDocument()->GetSelectionManager()->GetSelection();

  if (selection.IsEmpty())
    return false;

  ezDocumentObjectConverterWriter writer(&out_objectGraph, this);

  ezHashSet<const ezDocumentObject*> copiedNodes;
  for (const ezDocumentObject* pObject : selection)
  {
    // Only add nodes here, connections are then collected below to ensure
    // that we always include only valid connections within the copied subgraph no matter if they are selected or not.
    if (IsNode(pObject))
    {
      // objects are required to be named root but this is not enforced or obvious by the interface.
      writer.AddObjectToGraph(pObject, "root");
      copiedNodes.Insert(pObject);
    }
  }

  ezHashSet<const ezDocumentObject*> copiedConnections;
  for (const ezDocumentObject* pNodeObject : selection)
  {
    if (IsNode(pNodeObject) == false)
      continue;

    auto outputs = GetOutputPins(pNodeObject);
    for (auto& pSourcePin : outputs)
    {
      auto connections = GetConnections(*pSourcePin);
      for (const ezConnection* pConnection : connections)
      {
        const ezDocumentObject* pConnectionObject = pConnection->GetParent();

        EZ_ASSERT_DEV(pSourcePin == &pConnection->GetSourcePin(), "");
        if (copiedConnections.Contains(pConnectionObject) == false && copiedNodes.Contains(pConnection->GetTargetPin().GetParent()))
        {
          writer.AddObjectToGraph(pConnectionObject, "root");
          copiedConnections.Insert(pConnectionObject);
        }
      }
    }
  }

  AttachMetaDataBeforeSaving(out_objectGraph);

  return true;
}

bool ezDocumentNodeManager::PasteObjects(const ezArrayPtr<ezDocument::PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, const ezVec2& vPickedPosition, bool bAllowPickedPosition)
{
  bool bAddedAll = true;
  ezDeque<const ezDocumentObject*> AddedObjects;

  for (const auto& pi : info)
  {
    // only add nodes that are allowed to be added
    if (CanAdd(pi.m_pObject->GetTypeAccessor().GetType(), nullptr, "Children", pi.m_Index).m_Result.Succeeded())
    {
      AddedObjects.PushBack(pi.m_pObject);
      AddObject(pi.m_pObject, nullptr, "Children", pi.m_Index);
    }
    else
    {
      bAddedAll = false;
    }
  }

  RestoreMetaDataAfterLoading(objectGraph, true);

  if (!AddedObjects.IsEmpty() && bAllowPickedPosition)
  {
    ezCommandHistory* history = GetDocument()->GetCommandHistory();

    ezVec2 vAvgPos(0);
    ezUInt32 nodeCount = 0;
    for (const ezDocumentObject* pObject : AddedObjects)
    {
      if (IsNode(pObject))
      {
        vAvgPos += GetNodePos(pObject);
        ++nodeCount;
      }
    }

    vAvgPos /= (float)nodeCount;
    const ezVec2 vMoveNode = -vAvgPos + vPickedPosition;

    for (const ezDocumentObject* pObject : AddedObjects)
    {
      if (IsNode(pObject))
      {
        ezMoveNodeCommand move;
        move.m_Object = pObject->GetGuid();
        move.m_NewPos = GetNodePos(pObject) + vMoveNode;
        history->AddCommand(move);
      }
    }

    if (!bAddedAll)
    {
      ezLog::Info("[EditorStatus]Not all nodes were allowed to be added to the document");
    }
  }

  GetDocument()->GetSelectionManager()->SetSelection(AddedObjects);
  return true;
}

bool ezDocumentNodeManager::CanReachNode(const ezDocumentObject* pSource, const ezDocumentObject* pTarget, ezSet<const ezDocumentObject*>& Visited) const
{
  if (pSource == pTarget)
    return true;

  if (Visited.Contains(pSource))
    return false;

  Visited.Insert(pSource);

  auto outputs = GetOutputPins(pSource);
  for (auto& pSourcePin : outputs)
  {
    auto connections = GetConnections(*pSourcePin);
    for (const ezConnection* pConnection : connections)
    {
      if (CanReachNode(pConnection->GetTargetPin().GetParent(), pTarget, Visited))
        return true;
    }
  }

  return false;
}


bool ezDocumentNodeManager::WouldConnectionCreateCircle(const ezPin& source, const ezPin& target) const
{
  const ezDocumentObject* pSourceNode = source.GetParent();
  const ezDocumentObject* pTargetNode = target.GetParent();
  ezSet<const ezDocumentObject*> Visited;

  return CanReachNode(pTargetNode, pSourceNode, Visited);
}

bool ezDocumentNodeManager::InternalIsNode(const ezDocumentObject* pObject) const
{
  return true;
}

bool ezDocumentNodeManager::InternalIsConnection(const ezDocumentObject* pObject) const
{
  auto pType = pObject->GetTypeAccessor().GetType();
  return pType->IsDerivedFrom(GetConnectionType());
}

ezStatus ezDocumentNodeManager::InternalCanConnect(const ezPin& source, const ezPin& target, CanConnectResult& out_Result) const
{
  out_Result = CanConnectResult::ConnectNtoN;
  return ezStatus(EZ_SUCCESS);
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
      else if (IsConnection(e.m_pObject))
      {
        // Nothing to do here: Map entries are created in Connect method.
      }
    }
    break;
    case ezDocumentObjectEvent::Type::BeforeObjectDestroyed:
    {
      if (IsNode(e.m_pObject))
      {
        auto it = m_ObjectToNode.Find(e.m_pObject->GetGuid());
        EZ_ASSERT_DEBUG(it.IsValid(), "Sanity check failed!");

        m_ObjectToNode.Remove(it);
      }
      else if (IsConnection(e.m_pObject))
      {
        // Nothing to do here: Map entries are removed in Disconnect method.
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

    default:
      break;
  }
}

void ezDocumentNodeManager::RestoreOldMetaDataAfterLoading(const ezAbstractObjectGraph& graph, const ezAbstractObjectNode::Property& connectionsProperty, const ezDocumentObject* pSourceObject)
{
  if (connectionsProperty.m_Value.IsA<ezVariantArray>() == false)
    return;

  const ezVariantArray& array = connectionsProperty.m_Value.Get<ezVariantArray>();
  for (const ezVariant& var : array)
  {
    if (var.IsA<ezUuid>() == false)
      continue;

    auto pOldConnectionAbstractObject = graph.GetNode(var.Get<ezUuid>());
    auto pTargetProperty = pOldConnectionAbstractObject->FindProperty("Target");
    if (pTargetProperty == nullptr || pTargetProperty->m_Value.IsA<ezUuid>() == false)
      continue;

    ezDocumentObject* pTargetObject = GetObject(pTargetProperty->m_Value.Get<ezUuid>());
    if (pTargetObject == nullptr)
      continue;

    auto pSourcePinProperty = pOldConnectionAbstractObject->FindProperty("SourcePin");
    if (pSourcePinProperty == nullptr || pSourcePinProperty->m_Value.IsA<ezString>() == false)
      continue;

    auto pTargetPinProperty = pOldConnectionAbstractObject->FindProperty("TargetPin");
    if (pTargetPinProperty == nullptr || pTargetPinProperty->m_Value.IsA<ezString>() == false)
      continue;

    const ezPin* pSourcePin = GetOutputPinByName(pSourceObject, pSourcePinProperty->m_Value.Get<ezString>());
    const ezPin* pTargetPin = GetInputPinByName(pTargetObject, pTargetPinProperty->m_Value.Get<ezString>());
    if (pSourcePin == nullptr || pTargetPin == nullptr)
      continue;

    const ezRTTI* pConnectionType = GetConnectionType();
    ezDocumentNodeManager::CanConnectResult res;
    if (CanConnect(pConnectionType, *pSourcePin, *pTargetPin, res).m_Result.Succeeded())
    {
      ezUuid ObjectGuid;
      ObjectGuid.CreateNewUuid();
      ezDocumentObject* pConnectionObject = CreateObject(pConnectionType, ObjectGuid);

      AddObject(pConnectionObject, nullptr, "", -1);

      Connect(pConnectionObject, *pSourcePin, *pTargetPin);
    }
  }
}
