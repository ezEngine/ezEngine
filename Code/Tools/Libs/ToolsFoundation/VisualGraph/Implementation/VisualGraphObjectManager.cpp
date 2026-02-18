#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <ToolsFoundation/Command/VisualGraphCommands.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <ToolsFoundation/VisualGraph/VisualGraphObjectManager.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualGraphPin, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

////////////////////////////////////////////////////////////////////////
// ezVisualGraphObjectManager Internal
////////////////////////////////////////////////////////////////////////

struct DocumentNodeManager_NodeMetaData
{
  ezVec2 m_Pos = ezVec2::MakeZero();
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

  bool IsValid() const { return m_Source.IsValid() && m_Target.IsValid(); }
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
// ezDocumentObject_ConnectionBase
////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDocumentObject_ConnectionBase, 1, ezRTTIDefaultAllocator<ezDocumentObject_ConnectionBase>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Source", m_Source)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("Target", m_Target)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("SourcePin", m_SourcePin)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("TargetPin", m_TargetPin)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

////////////////////////////////////////////////////////////////////////
// ezVisualGraphObjectManager
////////////////////////////////////////////////////////////////////////

ezVisualGraphObjectManager::ezVisualGraphObjectManager()
{
  m_ObjectEvents.AddEventHandler(ezMakeDelegate(&ezVisualGraphObjectManager::ObjectHandler, this));
  m_StructureEvents.AddEventHandler(ezMakeDelegate(&ezVisualGraphObjectManager::StructureEventHandler, this));
  m_PropertyEvents.AddEventHandler(ezMakeDelegate(&ezVisualGraphObjectManager::PropertyEventsHandler, this));
}

ezVisualGraphObjectManager::~ezVisualGraphObjectManager()
{
  m_ObjectEvents.RemoveEventHandler(ezMakeDelegate(&ezVisualGraphObjectManager::ObjectHandler, this));
  m_StructureEvents.RemoveEventHandler(ezMakeDelegate(&ezVisualGraphObjectManager::StructureEventHandler, this));
  m_PropertyEvents.RemoveEventHandler(ezMakeDelegate(&ezVisualGraphObjectManager::PropertyEventsHandler, this));
}

void ezVisualGraphObjectManager::GetNodeCreationTemplates(ezDynamicArray<ezVisualGraphNodeDesc>& out_templates) const
{
  ezHybridArray<const ezRTTI*, 32> types;
  GetCreateableTypes(types);

  for (auto pType : types)
  {
    auto& nodeTemplate = out_templates.ExpandAndGetRef();
    nodeTemplate.m_pType = pType;
  }
}

const ezRTTI* ezVisualGraphObjectManager::GetConnectionType() const
{
  return ezGetStaticRTTI<ezDocumentObject_ConnectionBase>();
}

ezVec2 ezVisualGraphObjectManager::GetNodePos(const ezDocumentObject* pObject) const
{
  EZ_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  auto it = m_ObjectToNode.Find(pObject->GetGuid());
  EZ_ASSERT_DEV(it.IsValid(), "Can't get pos of objects that aren't nodes!");
  return it.Value().m_vPos;
}

const ezVisualGraphConnection& ezVisualGraphObjectManager::GetConnection(const ezDocumentObject* pObject) const
{
  EZ_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  auto it = m_ObjectToConnection.Find(pObject->GetGuid());
  EZ_ASSERT_DEV(it.IsValid(), "Can't get connection for objects that aren't connections!");
  return *it.Value();
}

const ezVisualGraphConnection* ezVisualGraphObjectManager::GetConnectionIfExists(const ezDocumentObject* pObject) const
{
  EZ_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  auto it = m_ObjectToConnection.Find(pObject->GetGuid());
  return it.IsValid() ? it.Value().Borrow() : nullptr;
}

const ezVisualGraphPin* ezVisualGraphObjectManager::GetInputPinByName(const ezDocumentObject* pObject, ezStringView sName) const
{
  EZ_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  auto it = m_ObjectToNode.Find(pObject->GetGuid());
  EZ_ASSERT_DEV(it.IsValid(), "Can't get input pins of objects that aren't nodes!");
  for (auto& pPin : it.Value().m_Inputs)
  {
    if (pPin->GetName() == sName)
      return pPin.Borrow();
  }
  return nullptr;
}

const ezVisualGraphPin* ezVisualGraphObjectManager::GetOutputPinByName(const ezDocumentObject* pObject, ezStringView sName) const
{
  EZ_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  auto it = m_ObjectToNode.Find(pObject->GetGuid());
  EZ_ASSERT_DEV(it.IsValid(), "Can't get input pins of objects that aren't nodes!");
  for (auto& pPin : it.Value().m_Outputs)
  {
    if (pPin->GetName() == sName)
      return pPin.Borrow();
  }
  return nullptr;
}

ezArrayPtr<const ezUniquePtr<const ezVisualGraphPin>> ezVisualGraphObjectManager::GetInputPins(const ezDocumentObject* pObject) const
{
  EZ_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  auto it = m_ObjectToNode.Find(pObject->GetGuid());
  EZ_ASSERT_DEV(it.IsValid(), "Can't get input pins of objects that aren't nodes!");
  return ezMakeArrayPtr((ezUniquePtr<const ezVisualGraphPin>*)it.Value().m_Inputs.GetData(), it.Value().m_Inputs.GetCount());
}

ezArrayPtr<const ezUniquePtr<const ezVisualGraphPin>> ezVisualGraphObjectManager::GetOutputPins(const ezDocumentObject* pObject) const
{
  EZ_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  auto it = m_ObjectToNode.Find(pObject->GetGuid());
  EZ_ASSERT_DEV(it.IsValid(), "Can't get input pins of objects that aren't nodes!");
  return ezMakeArrayPtr((ezUniquePtr<const ezVisualGraphPin>*)it.Value().m_Outputs.GetData(), it.Value().m_Outputs.GetCount());
}

bool ezVisualGraphObjectManager::IsNode(const ezDocumentObject* pObject) const
{
  EZ_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  if (pObject == nullptr)
    return false;
  if (pObject == GetRootObject())
    return false;

  return InternalIsNode(pObject);
}

bool ezVisualGraphObjectManager::IsConnection(const ezDocumentObject* pObject) const
{
  EZ_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  if (pObject == nullptr)
    return false;
  if (pObject == GetRootObject())
    return false;

  return InternalIsConnection(pObject);
}

bool ezVisualGraphObjectManager::IsDynamicPinProperty(const ezDocumentObject* pObject, const ezAbstractProperty* pProp) const
{
  if (IsNode(pObject) == false)
    return false;

  if (pProp == nullptr)
    return false;

  return InternalIsDynamicPinProperty(pObject, pProp);
}

ezArrayPtr<const ezVisualGraphConnection* const> ezVisualGraphObjectManager::GetConnections(const ezVisualGraphPin& pin) const
{
  auto it = m_Connections.Find(&pin);
  if (it.IsValid())
  {
    return it.Value();
  }

  return ezArrayPtr<const ezVisualGraphConnection* const>();
}

bool ezVisualGraphObjectManager::HasConnections(const ezVisualGraphPin& pin) const
{
  auto it = m_Connections.Find(&pin);
  return it.IsValid() && it.Value().IsEmpty() == false;
}

bool ezVisualGraphObjectManager::IsConnected(const ezVisualGraphPin& source, const ezVisualGraphPin& target) const
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

ezStatus ezVisualGraphObjectManager::CanConnect(const ezRTTI* pObjectType, const ezVisualGraphPin& source, const ezVisualGraphPin& target, CanConnectResult& out_result) const
{
  out_result = CanConnectResult::ConnectNever;

  if (pObjectType == nullptr || pObjectType->IsDerivedFrom(GetConnectionType()) == false)
    return ezStatus("Invalid connection object type");

  if (source.m_Type != ezVisualGraphPin::Type::Output)
    return ezStatus("Source pin is not an output pin.");
  if (target.m_Type != ezVisualGraphPin::Type::Input)
    return ezStatus("Target pin is not an input pin.");

  if (source.m_pParent == target.m_pParent)
    return ezStatus("Nodes cannot be connect with themselves.");

  if (IsConnected(source, target))
    return ezStatus("Pins already connected.");

  return InternalCanConnect(source, target, out_result);
}

ezStatus ezVisualGraphObjectManager::CanDisconnect(const ezVisualGraphConnection* pConnection) const
{
  if (pConnection == nullptr)
    return ezStatus("Invalid connection");

  return InternalCanDisconnect(pConnection->GetSourcePin(), pConnection->GetTargetPin());
}

ezStatus ezVisualGraphObjectManager::CanDisconnect(const ezDocumentObject* pObject) const
{
  if (!IsConnection(pObject))
    return ezStatus("Invalid connection object");

  const ezVisualGraphConnection& connection = GetConnection(pObject);
  return InternalCanDisconnect(connection.GetSourcePin(), connection.GetTargetPin());
}

ezStatus ezVisualGraphObjectManager::CanMoveNode(const ezDocumentObject* pObject, const ezVec2& vPos) const
{
  EZ_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  if (!IsNode(pObject))
    return ezStatus("The given object is not a node!");

  return InternalCanMoveNode(pObject, vPos);
}

void ezVisualGraphObjectManager::Connect(const ezDocumentObject* pObject, const ezVisualGraphPin& source, const ezVisualGraphPin& target)
{
  ezVisualGraphObjectManager::CanConnectResult res = CanConnectResult::ConnectNever;
  EZ_IGNORE_UNUSED(res);
  EZ_ASSERT_DEBUG(CanConnect(pObject->GetType(), source, target, res).Succeeded(), "Connect: Sanity check failed!");

  EZ_ASSERT_DEBUG(pObject->GetTypeAccessor().GetValue("Source") == source.GetParent()->GetGuid(), "Property should have been set at this point already");
  EZ_ASSERT_DEBUG(pObject->GetTypeAccessor().GetValue("Target") == target.GetParent()->GetGuid(), "Property should have been set at this point already");
  EZ_ASSERT_DEBUG(pObject->GetTypeAccessor().GetValue("SourcePin") == source.GetName(), "Property should have been set at this point already");
  EZ_ASSERT_DEBUG(pObject->GetTypeAccessor().GetValue("TargetPin") == target.GetName(), "Property should have been set at this point already");

  auto pConnection = EZ_DEFAULT_NEW(ezVisualGraphConnection, source, target, pObject);
  m_ObjectToConnection.Insert(pObject->GetGuid(), pConnection);

  m_Connections[&source].PushBack(pConnection);
  m_Connections[&target].PushBack(pConnection);

  {
    ezVisualGraphObjectManagerEvent e(ezVisualGraphObjectManagerEvent::Type::AfterPinsConnected, pObject);
    m_NodeEvents.Broadcast(e);
  }
}

void ezVisualGraphObjectManager::Disconnect(const ezDocumentObject* pObject)
{
  auto it = m_ObjectToConnection.Find(pObject->GetGuid());
  EZ_ASSERT_DEBUG(it.IsValid(), "Sanity check failed!");
  EZ_ASSERT_DEBUG(CanDisconnect(pObject).Succeeded(), "Disconnect: Sanity check failed!");

  {
    ezVisualGraphObjectManagerEvent e(ezVisualGraphObjectManagerEvent::Type::BeforePinsDisonnected, pObject);
    m_NodeEvents.Broadcast(e);
  }

  auto& pConnection = it.Value();
  const ezVisualGraphPin& source = pConnection->GetSourcePin();
  const ezVisualGraphPin& target = pConnection->GetTargetPin();
  m_Connections[&source].RemoveAndCopy(pConnection.Borrow());
  m_Connections[&target].RemoveAndCopy(pConnection.Borrow());

  m_ObjectToConnection.Remove(it);
}

void ezVisualGraphObjectManager::MoveNode(const ezDocumentObject* pObject, const ezVec2& vPos)
{
  EZ_ASSERT_DEBUG(CanMoveNode(pObject, vPos).Succeeded(), "MoveNode: Sanity check failed!");

  auto it = m_ObjectToNode.Find(pObject->GetGuid());
  EZ_ASSERT_DEBUG(it.IsValid(), "Moveable node does not exist, CanMoveNode impl invalid!");
  it.Value().m_vPos = vPos;

  ezVisualGraphObjectManagerEvent e(ezVisualGraphObjectManagerEvent::Type::NodeMoved, pObject);
  m_NodeEvents.Broadcast(e);
}

void ezVisualGraphObjectManager::AttachMetaDataBeforeSaving(ezAbstractObjectGraph& ref_graph) const
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
  }
}

void ezVisualGraphObjectManager::RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph, bool bUndoable)
{
  ezCommandHistory* history = GetDocument()->GetCommandHistory();

  auto pNodeMetaDataType = ezGetStaticRTTI<DocumentNodeManager_NodeMetaData>();
  auto pConnectionMetaDataType = ezGetStaticRTTI<DocumentNodeManager_ConnectionMetaData>();

  ezRttiConverterContext context;
  ezRttiConverterReader rttiConverter(&graph, &context);

  // Ensure that all nodes have their pins created
  for (auto it : graph.GetAllNodes())
  {
    auto pAbstractObject = it.Value();
    ezDocumentObject* pObject = GetObject(pAbstractObject->GetGuid());
    if (pObject != nullptr && IsNode(pObject))
    {
      auto& nodeInternal = m_ObjectToNode[pObject->GetGuid()];
      if (nodeInternal.m_Inputs.IsEmpty() && nodeInternal.m_Outputs.IsEmpty())
      {
        InternalCreatePins(pObject, nodeInternal);
      }
    }
  }

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

      if (CanMoveNode(pObject, nodeMetaData.m_Pos).Succeeded())
      {
        if (bUndoable)
        {
          ezMoveNodeCommand move;
          move.m_Object = pObject->GetGuid();
          move.m_NewPos = nodeMetaData.m_Pos;
          history->AddCommand(move).LogFailure();
        }
        else
        {
          MoveNode(pObject, nodeMetaData.m_Pos);
        }
      }

      EZ_ASSERT_DEV(pAbstractObject->FindProperty("Node::Connections") == nullptr, "Old file format detected that is not supported anymore. Re-save the document with a previous version of ez. ({})", GetDocument()->GetDocumentPath());
    }
    else if (IsConnection(pObject))
    {
      ezVariant sourceVar = pObject->GetTypeAccessor().GetValue("Source");
      ezVariant targetVar = pObject->GetTypeAccessor().GetValue("Target");
      ezVariant sourcePinVar = pObject->GetTypeAccessor().GetValue("SourcePin");
      ezVariant targetPinVar = pObject->GetTypeAccessor().GetValue("TargetPin");
      EZ_ASSERT_DEV(sourceVar.IsA<ezUuid>() && targetVar.IsA<ezUuid>() && sourcePinVar.IsA<ezString>() && targetPinVar.IsA<ezString>(), "Invalid connection object");

      ezUuid source = sourceVar.Get<ezUuid>();
      ezUuid target = targetVar.Get<ezUuid>();
      ezStringView sourcePin = sourcePinVar.Get<ezString>();
      ezStringView targetPin = targetPinVar.Get<ezString>();

      const ezVisualGraphPin* pSourcePin = nullptr;
      const ezVisualGraphPin* pTargetPin = nullptr;
      if (ResolveConnection(source, target, sourcePin, targetPin, pSourcePin, pTargetPin).Failed())
      {
        // Try to restore from metadata
        DocumentNodeManager_ConnectionMetaData connectionMetaData;
        rttiConverter.ApplyPropertiesToObject(pAbstractObject, pConnectionMetaDataType, &connectionMetaData);
        if (connectionMetaData.IsValid())
        {
          pObject->GetTypeAccessor().SetValue("Source", connectionMetaData.m_Source);
          pObject->GetTypeAccessor().SetValue("Target", connectionMetaData.m_Target);
          pObject->GetTypeAccessor().SetValue("SourcePin", connectionMetaData.m_SourcePin);
          pObject->GetTypeAccessor().SetValue("TargetPin", connectionMetaData.m_TargetPin);

          source = connectionMetaData.m_Source;
          target = connectionMetaData.m_Target;
          sourcePin = connectionMetaData.m_SourcePin;
          targetPin = connectionMetaData.m_TargetPin;
        }
      }

      if (ResolveConnection(source, target, sourcePin, targetPin, pSourcePin, pTargetPin).Succeeded())
      {
        if (bUndoable)
        {
          ezConnectNodePinsCommand cmd;
          cmd.m_ConnectionObject = pObject->GetGuid();
          cmd.m_ObjectSource = pSourcePin->GetParent()->GetGuid();
          cmd.m_ObjectTarget = pTargetPin->GetParent()->GetGuid();
          cmd.m_sSourcePin = pSourcePin->GetName();
          cmd.m_sTargetPin = pTargetPin->GetName();
          history->AddCommand(cmd).LogFailure();
        }
        else
        {
          Connect(pObject, *pSourcePin, *pTargetPin);
        }
      }
      else
      {
        RemoveObject(pObject);
        DestroyObject(pObject);
      }
    }
    else
    {
      DocumentNodeManager_ConnectionMetaData connectionMetaData;
      rttiConverter.ApplyPropertiesToObject(pAbstractObject, pConnectionMetaDataType, &connectionMetaData);

      if (connectionMetaData.IsValid() == false)
        continue;

      const ezVisualGraphPin* pSourcePin = nullptr;
      const ezVisualGraphPin* pTargetPin = nullptr;
      if (ResolveConnection(connectionMetaData.m_Source, connectionMetaData.m_Target, connectionMetaData.m_SourcePin, connectionMetaData.m_TargetPin, pSourcePin, pTargetPin).Succeeded())
      {
        ezDocumentObject* pNewConnectionObject = CreateObject(GetConnectionType());
        pNewConnectionObject->GetTypeAccessor().SetValue("Source", connectionMetaData.m_Source);
        pNewConnectionObject->GetTypeAccessor().SetValue("Target", connectionMetaData.m_Target);
        pNewConnectionObject->GetTypeAccessor().SetValue("SourcePin", connectionMetaData.m_SourcePin);
        pNewConnectionObject->GetTypeAccessor().SetValue("TargetPin", connectionMetaData.m_TargetPin);
        AddObject(pNewConnectionObject, nullptr, "", -1);

        EZ_ASSERT_DEV(bUndoable == false, "This code path should only be taken by document loading code");
        Connect(pNewConnectionObject, *pSourcePin, *pTargetPin);
      }

      RemoveObject(pObject);
      DestroyObject(pObject);
    }
  }
}

void ezVisualGraphObjectManager::GetMetaDataHash(const ezDocumentObject* pObject, ezUInt64& inout_uiHash) const
{
  if (IsNode(pObject))
  {
    // The node position is not hashed here since the hash is only used for asset transform
    // and for that the node position is irrelevant.
  }
  else if (IsConnection(pObject))
  {
    const ezVisualGraphConnection& connection = GetConnection(pObject);
    const ezVisualGraphPin& sourcePin = connection.GetSourcePin();
    const ezVisualGraphPin& targetPin = connection.GetTargetPin();

    inout_uiHash = ezHashingUtils::xxHash64(&sourcePin.GetParent()->GetGuid(), sizeof(ezUuid), inout_uiHash);
    inout_uiHash = ezHashingUtils::xxHash64(&targetPin.GetParent()->GetGuid(), sizeof(ezUuid), inout_uiHash);
    inout_uiHash = ezHashingUtils::xxHash64String(sourcePin.GetName(), inout_uiHash);
    inout_uiHash = ezHashingUtils::xxHash64String(targetPin.GetName(), inout_uiHash);
  }
}

bool ezVisualGraphObjectManager::CopySelectedObjects(ezAbstractObjectGraph& out_objectGraph) const
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
      for (const ezVisualGraphConnection* pConnection : connections)
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

bool ezVisualGraphObjectManager::PasteObjects(const ezArrayPtr<ezDocument::PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, const ezVec2& vPickedPosition, bool bAllowPickedPosition)
{
  bool bAddedAll = true;
  ezDeque<const ezDocumentObject*> AddedObjects;

  for (const auto& pi : info)
  {
    // only add nodes that are allowed to be added
    if (CanAdd(pi.m_pObject->GetTypeAccessor().GetType(), nullptr, "Children", pi.m_Index).Succeeded())
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
        history->AddCommand(move).LogFailure();
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

bool ezVisualGraphObjectManager::CanReachNode(const ezDocumentObject* pSource, const ezDocumentObject* pTarget, ezSet<const ezDocumentObject*>& Visited) const
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
    for (const ezVisualGraphConnection* pConnection : connections)
    {
      if (CanReachNode(pConnection->GetTargetPin().GetParent(), pTarget, Visited))
        return true;
    }
  }

  return false;
}


bool ezVisualGraphObjectManager::WouldConnectionCreateCircle(const ezVisualGraphPin& source, const ezVisualGraphPin& target) const
{
  const ezDocumentObject* pSourceNode = source.GetParent();
  const ezDocumentObject* pTargetNode = target.GetParent();
  ezSet<const ezDocumentObject*> Visited;

  return CanReachNode(pTargetNode, pSourceNode, Visited);
}

ezResult ezVisualGraphObjectManager::ResolveConnection(const ezUuid& sourceObject, const ezUuid& targetObject, ezStringView sourcePin, ezStringView targetPin, const ezVisualGraphPin*& out_pSourcePin, const ezVisualGraphPin*& out_pTargetPin) const
{
  const ezDocumentObject* pSource = GetObject(sourceObject);
  const ezDocumentObject* pTarget = GetObject(targetObject);
  if (pSource == nullptr || pTarget == nullptr)
  {
    return EZ_FAILURE;
  }

  const ezVisualGraphPin* pSourcePin = GetOutputPinByName(pSource, sourcePin);
  if (pSourcePin == nullptr)
  {
    ezLog::Error("Unknown output pin '{}' on '{}'. The connection has been removed.", sourcePin, pSource->GetType()->GetTypeName());
    return EZ_FAILURE;
  }

  const ezVisualGraphPin* pTargetPin = GetInputPinByName(pTarget, targetPin);
  if (pTargetPin == nullptr)
  {
    ezLog::Error("Unknown input pin '{}' on '{}'. The connection has been removed.", targetPin, pTarget->GetType()->GetTypeName());
    return EZ_FAILURE;
  }

  out_pSourcePin = pSourcePin;
  out_pTargetPin = pTargetPin;
  return EZ_SUCCESS;
}

void ezVisualGraphObjectManager::GetDynamicPinNames(const ezDocumentObject* pObject, ezStringView sPropertyName, ezStringView sPinName, ezDynamicArray<ezString>& out_Names) const
{
  out_Names.Clear();

  const ezAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sPropertyName);
  if (pProp == nullptr)
  {
    ezLog::Warning("Property '{0}' not found in type '{1}'", sPropertyName, pObject->GetType()->GetTypeName());
    return;
  }

  ezStringBuilder sTemp;
  ezVariant value = pObject->GetTypeAccessor().GetValue(sPropertyName);

  if (pProp->GetCategory() == ezPropertyCategory::Member)
  {
    if (value.CanConvertTo<ezUInt32>())
    {
      ezUInt32 uiCount = value.ConvertTo<ezUInt32>();
      for (ezUInt32 i = 0; i < uiCount; ++i)
      {
        sTemp.SetFormat("{}[{}]", sPinName, i);
        out_Names.PushBack(sTemp);
      }
    }
  }
  else if (pProp->GetCategory() == ezPropertyCategory::Array)
  {
    auto pArrayProp = static_cast<const ezAbstractArrayProperty*>(pProp);

    auto& a = value.Get<ezVariantArray>();
    const ezUInt32 uiCount = a.GetCount();

    auto variantType = pArrayProp->GetSpecificType()->GetVariantType();
    if (variantType >= ezVariantType::Int8 && variantType <= ezVariantType::UInt64)
    {
      for (ezUInt32 i = 0; i < uiCount; ++i)
      {
        sTemp.SetFormat("{}", a[i]);
        out_Names.PushBack(sTemp);
      }
    }
    else if (variantType == ezVariantType::String || variantType == ezVariantType::HashedString)
    {
      for (ezUInt32 i = 0; i < uiCount; ++i)
      {
        out_Names.PushBack(a[i].ConvertTo<ezString>());
      }
    }
    else if (pArrayProp->GetSpecificType()->GetTypeFlags().IsSet(ezTypeFlags::Class))
    {
      for (ezUInt32 i = 0; i < uiCount; ++i)
      {
        auto pInnerObject = GetObject(a[i].Get<ezUuid>());
        if (pInnerObject == nullptr)
          continue;

        ezVariant nameVar = pInnerObject->GetTypeAccessor().GetValue("Name");
        if (nameVar.IsString() || nameVar.IsHashedString())
        {
          out_Names.PushBack(nameVar.ConvertTo<ezString>());
        }
        else
        {
          sTemp.SetFormat("{}[{}]", sPinName, i);
          out_Names.PushBack(sTemp);
        }
      }
    }
    else
    {
      for (ezUInt32 i = 0; i < uiCount; ++i)
      {
        sTemp.SetFormat("{}[{}]", sPinName, i);
        out_Names.PushBack(sTemp);
      }
    }
  }
}

bool ezVisualGraphObjectManager::TryRecreatePins(const ezDocumentObject* pObject)
{
  if (!IsNode(pObject))
    return false;

  auto& nodeInternal = m_ObjectToNode[pObject->GetGuid()];

  for (auto& pPin : nodeInternal.m_Inputs)
  {
    if (HasConnections(*pPin))
    {
      ezLog::Error("Can't re-create pins if they are still connected");
      return false;
    }
  }

  for (auto& pPin : nodeInternal.m_Outputs)
  {
    if (HasConnections(*pPin))
    {
      ezLog::Error("Can't re-create pins if they are still connected");
      return false;
    }
  }

  {
    ezVisualGraphObjectManagerEvent e(ezVisualGraphObjectManagerEvent::Type::BeforePinsChanged, pObject);
    m_NodeEvents.Broadcast(e);
  }

  nodeInternal.m_Inputs.Clear();
  nodeInternal.m_Outputs.Clear();
  InternalCreatePins(pObject, nodeInternal);

  {
    ezVisualGraphObjectManagerEvent e(ezVisualGraphObjectManagerEvent::Type::AfterPinsChanged, pObject);
    m_NodeEvents.Broadcast(e);
  }

  return true;
}

bool ezVisualGraphObjectManager::InternalIsNode(const ezDocumentObject* pObject) const
{
  return true;
}

bool ezVisualGraphObjectManager::InternalIsConnection(const ezDocumentObject* pObject) const
{
  auto pType = pObject->GetTypeAccessor().GetType();
  return pType->IsDerivedFrom(GetConnectionType());
}

ezStatus ezVisualGraphObjectManager::InternalCanConnect(const ezVisualGraphPin& source, const ezVisualGraphPin& target, CanConnectResult& out_Result) const
{
  out_Result = CanConnectResult::ConnectNtoN;
  return ezStatus(EZ_SUCCESS);
}

void ezVisualGraphObjectManager::ObjectHandler(const ezDocumentObjectEvent& e)
{
  switch (e.m_EventType)
  {
    case ezDocumentObjectEvent::Type::AfterObjectCreated:
    {
      if (IsNode(e.m_pObject))
      {
        EZ_ASSERT_DEBUG(!m_ObjectToNode.Contains(e.m_pObject->GetGuid()), "Sanity check failed!");
        m_ObjectToNode[e.m_pObject->GetGuid()] = NodeInternal();
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
    default:
      EZ_ASSERT_NOT_IMPLEMENTED
  }
}

void ezVisualGraphObjectManager::StructureEventHandler(const ezDocumentObjectStructureEvent& e)
{
  switch (e.m_EventType)
  {
    case ezDocumentObjectStructureEvent::Type::BeforeObjectAdded:
    {
      if (IsNode(e.m_pObject))
      {
        auto& nodeInternal = m_ObjectToNode[e.m_pObject->GetGuid()];
        if (nodeInternal.m_Inputs.IsEmpty() && nodeInternal.m_Outputs.IsEmpty())
        {
          InternalCreatePins(e.m_pObject, nodeInternal);
          // TODO: Sanity check pins (duplicate names etc).
        }

        ezVisualGraphObjectManagerEvent e2(ezVisualGraphObjectManagerEvent::Type::BeforeNodeAdded, e.m_pObject);
        m_NodeEvents.Broadcast(e2);
      }
    }
    break;
    case ezDocumentObjectStructureEvent::Type::AfterObjectAdded:
    {
      if (IsNode(e.m_pObject))
      {
        ezVisualGraphObjectManagerEvent e2(ezVisualGraphObjectManagerEvent::Type::AfterNodeAdded, e.m_pObject);
        m_NodeEvents.Broadcast(e2);
      }
      else
      {
        HandlePotentialDynamicPinPropertyChanged(e.m_pNewParent, e.m_sParentProperty);
      }
    }
    break;
    case ezDocumentObjectStructureEvent::Type::BeforeObjectRemoved:
    {
      if (IsNode(e.m_pObject))
      {
        ezVisualGraphObjectManagerEvent e2(ezVisualGraphObjectManagerEvent::Type::BeforeNodeRemoved, e.m_pObject);
        m_NodeEvents.Broadcast(e2);
      }
    }
    break;
    case ezDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    {
      if (IsNode(e.m_pObject))
      {
        ezVisualGraphObjectManagerEvent e2(ezVisualGraphObjectManagerEvent::Type::AfterNodeRemoved, e.m_pObject);
        m_NodeEvents.Broadcast(e2);
      }
      else
      {
        HandlePotentialDynamicPinPropertyChanged(e.m_pPreviousParent, e.m_sParentProperty);
      }
    }
    break;

    default:
      break;
  }
}

void ezVisualGraphObjectManager::PropertyEventsHandler(const ezDocumentObjectPropertyEvent& e)
{
  if (e.m_pObject == nullptr)
    return;

  HandlePotentialDynamicPinPropertyChanged(e.m_pObject, e.m_sProperty);

  if (const ezDocumentObject* pParent = e.m_pObject->GetParent())
  {
    HandlePotentialDynamicPinPropertyChanged(pParent, e.m_pObject->GetParentProperty());
  }
}

void ezVisualGraphObjectManager::HandlePotentialDynamicPinPropertyChanged(const ezDocumentObject* pObject, ezStringView sPropertyName)
{
  if (pObject == nullptr)
    return;

  const ezAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sPropertyName);
  if (pProp == nullptr)
    return;

  if (IsDynamicPinProperty(pObject, pProp))
  {
    TryRecreatePins(pObject);
  }
}
