#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimationControllerAsset/AnimationControllerAsset.h>
#include <EditorPluginAssets/AnimationControllerAsset/AnimationControllerGraphQt.h>
#include <Foundation/Math/ColorScheme.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <ToolsFoundation/Serialization/ToolsSerializationUtils.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimationControllerAssetDocument, 3, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimationControllerNodePin, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_SUBSYSTEM_DECLARATION(EditorPluginAssets, AnimationController)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezQtNodeScene::GetNodeFactory().RegisterCreator(ezGetStaticRTTI<ezAnimGraphNode>(), [](const ezRTTI* pRtti)->ezQtNode* { return new ezQtAnimationControllerNode(); });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezQtNodeScene::GetNodeFactory().UnregisterCreator(ezGetStaticRTTI<ezAnimGraphNode>());
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

bool ezAnimationControllerNodeManager::InternalIsNode(const ezDocumentObject* pObject) const
{
  auto pType = pObject->GetTypeAccessor().GetType();
  return pType->IsDerivedFrom<ezAnimGraphNode>();
}

void ezAnimationControllerNodeManager::InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& ref_node)
{
  auto pType = pObject->GetTypeAccessor().GetType();
  if (!pType->IsDerivedFrom<ezAnimGraphNode>())
    return;

  ezHybridArray<ezAbstractProperty*, 32> properties;
  pType->GetAllProperties(properties);

  const ezColor triggerPinColor = ezColorScheme::DarkUI(ezColorScheme::Grape);
  const ezColor numberPinColor = ezColorScheme::DarkUI(ezColorScheme::Lime);
  const ezColor weightPinColor = ezColorScheme::DarkUI(ezColorScheme::Teal);
  const ezColor localPosePinColor = ezColorScheme::DarkUI(ezColorScheme::Blue);
  const ezColor modelPosePinColor = ezColorScheme::DarkUI(ezColorScheme::Violet);
  // EXTEND THIS if a new type is introduced

  for (ezAbstractProperty* pProp : properties)
  {
    if (pProp->GetCategory() != ezPropertyCategory::Member || !pProp->GetSpecificType()->IsDerivedFrom<ezAnimGraphPin>())
      continue;

    if (pProp->GetSpecificType()->IsDerivedFrom<ezAnimGraphTriggerInputPin>())
    {
      auto pPin = EZ_DEFAULT_NEW(ezAnimationControllerNodePin, ezPin::Type::Input, pProp->GetPropertyName(), triggerPinColor, pObject);
      pPin->m_DataType = ezAnimGraphPin::Trigger;
      ref_node.m_Inputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<ezAnimGraphTriggerOutputPin>())
    {
      auto pPin = EZ_DEFAULT_NEW(ezAnimationControllerNodePin, ezPin::Type::Output, pProp->GetPropertyName(), triggerPinColor, pObject);
      pPin->m_DataType = ezAnimGraphPin::Trigger;
      ref_node.m_Outputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<ezAnimGraphNumberInputPin>())
    {
      auto pPin = EZ_DEFAULT_NEW(ezAnimationControllerNodePin, ezPin::Type::Input, pProp->GetPropertyName(), numberPinColor, pObject);
      pPin->m_DataType = ezAnimGraphPin::Number;
      ref_node.m_Inputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<ezAnimGraphNumberOutputPin>())
    {
      auto pPin = EZ_DEFAULT_NEW(ezAnimationControllerNodePin, ezPin::Type::Output, pProp->GetPropertyName(), numberPinColor, pObject);
      pPin->m_DataType = ezAnimGraphPin::Number;
      ref_node.m_Outputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<ezAnimGraphBoneWeightsInputPin>())
    {
      auto pPin = EZ_DEFAULT_NEW(ezAnimationControllerNodePin, ezPin::Type::Input, pProp->GetPropertyName(), weightPinColor, pObject);
      pPin->m_DataType = ezAnimGraphPin::BoneWeights;
      ref_node.m_Inputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<ezAnimGraphBoneWeightsOutputPin>())
    {
      auto pPin = EZ_DEFAULT_NEW(ezAnimationControllerNodePin, ezPin::Type::Output, pProp->GetPropertyName(), weightPinColor, pObject);
      pPin->m_DataType = ezAnimGraphPin::BoneWeights;
      ref_node.m_Outputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<ezAnimGraphLocalPoseInputPin>())
    {
      auto pPin = EZ_DEFAULT_NEW(ezAnimationControllerNodePin, ezPin::Type::Input, pProp->GetPropertyName(), localPosePinColor, pObject);
      pPin->m_DataType = ezAnimGraphPin::LocalPose;
      ref_node.m_Inputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<ezAnimGraphLocalPoseMultiInputPin>())
    {
      auto pPin = EZ_DEFAULT_NEW(ezAnimationControllerNodePin, ezPin::Type::Input, pProp->GetPropertyName(), localPosePinColor, pObject);
      pPin->m_DataType = ezAnimGraphPin::LocalPose;
      pPin->m_bMultiInputPin = true;
      pPin->m_Shape = ezPin::Shape::RoundRect;
      ref_node.m_Inputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<ezAnimGraphLocalPoseOutputPin>())
    {
      auto pPin = EZ_DEFAULT_NEW(ezAnimationControllerNodePin, ezPin::Type::Output, pProp->GetPropertyName(), localPosePinColor, pObject);
      pPin->m_DataType = ezAnimGraphPin::LocalPose;
      ref_node.m_Outputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<ezAnimGraphModelPoseInputPin>())
    {
      auto pPin = EZ_DEFAULT_NEW(ezAnimationControllerNodePin, ezPin::Type::Input, pProp->GetPropertyName(), modelPosePinColor, pObject);
      pPin->m_DataType = ezAnimGraphPin::ModelPose;
      ref_node.m_Inputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<ezAnimGraphModelPoseOutputPin>())
    {
      auto pPin = EZ_DEFAULT_NEW(ezAnimationControllerNodePin, ezPin::Type::Output, pProp->GetPropertyName(), modelPosePinColor, pObject);
      pPin->m_DataType = ezAnimGraphPin::ModelPose;
      ref_node.m_Outputs.PushBack(pPin);
    }
    else
    {
      // EXTEND THIS if a new type is introduced
      EZ_ASSERT_NOT_IMPLEMENTED;
    }
  }
}

void ezAnimationControllerNodeManager::GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& ref_types) const
{
  ezSet<const ezRTTI*> typeSet;
  ezReflectionUtils::GatherTypesDerivedFromClass(ezGetStaticRTTI<ezAnimGraphNode>(), typeSet, false);

  ref_types.Clear();
  for (auto pType : typeSet)
  {
    if (pType->GetTypeFlags().IsAnySet(ezTypeFlags::Abstract))
      continue;

    ref_types.PushBack(pType);
  }
}

ezStatus ezAnimationControllerNodeManager::InternalCanConnect(const ezPin& source, const ezPin& target, CanConnectResult& out_result) const
{
  const ezAnimationControllerNodePin& sourcePin = ezStaticCast<const ezAnimationControllerNodePin&>(source);
  const ezAnimationControllerNodePin& targetPin = ezStaticCast<const ezAnimationControllerNodePin&>(target);

  out_result = CanConnectResult::ConnectNever;

  if (sourcePin.m_DataType != targetPin.m_DataType)
    return ezStatus("Can't connect pins of different data types");

  if (sourcePin.GetType() == targetPin.GetType())
    return ezStatus("Can only connect input pins with output pins.");

  switch (sourcePin.m_DataType)
  {
    case ezAnimGraphPin::Trigger:
      out_result = CanConnectResult::ConnectNtoN;
      break;

    case ezAnimGraphPin::Number:
      out_result = CanConnectResult::ConnectNto1;
      break;

    case ezAnimGraphPin::BoneWeights:
      out_result = CanConnectResult::ConnectNto1;
      break;

    case ezAnimGraphPin::LocalPose:
      if (targetPin.m_bMultiInputPin)
        out_result = CanConnectResult::ConnectNtoN;
      else
        out_result = CanConnectResult::ConnectNto1;
      break;

    case ezAnimGraphPin::ModelPose:
      out_result = CanConnectResult::ConnectNto1;
      break;

      // EXTEND THIS if a new type is introduced
      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return ezStatus(EZ_SUCCESS);
}

ezAnimationControllerAssetDocument::ezAnimationControllerAssetDocument(const char* szDocumentPath)
  : ezAssetDocument(szDocumentPath, EZ_DEFAULT_NEW(ezAnimationControllerNodeManager), ezAssetDocEngineConnection::None)
{
}

ezTransformStatus ezAnimationControllerAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  const auto* pNodeManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());

  ezDynamicArray<const ezDocumentObject*> allNodes;
  ezMap<ezUInt8, PinCount> pinCounts;
  CountPinTypes(pNodeManager, allNodes, pinCounts);

  // if the asset is entirely empty, don't complain
  if (allNodes.IsEmpty())
    return ezStatus(EZ_SUCCESS);

  SortNodesByPriority(allNodes);

  if (allNodes.IsEmpty())
  {
    return ezStatus("Animation controller graph doesn't have any output nodes.");
  }

  ezAnimGraph animController;
  animController.m_TriggerInputPinStates.SetCount(pinCounts[ezAnimGraphPin::Trigger].m_uiInputCount);
  animController.m_NumberInputPinStates.SetCount(pinCounts[ezAnimGraphPin::Number].m_uiInputCount);
  animController.m_BoneWeightInputPinStates.SetCount(pinCounts[ezAnimGraphPin::BoneWeights].m_uiInputCount);
  animController.m_LocalPoseInputPinStates.SetCount(pinCounts[ezAnimGraphPin::LocalPose].m_uiInputCount);
  animController.m_ModelPoseInputPinStates.SetCount(pinCounts[ezAnimGraphPin::ModelPose].m_uiInputCount);
  // EXTEND THIS if a new type is introduced

  for (ezUInt32 i = 0; i < ezAnimGraphPin::ENUM_COUNT; ++i)
  {
    animController.m_OutputPinToInputPinMapping[i].SetCount(pinCounts[i].m_uiOutputCount);
  }

  auto pIdxProperty = static_cast<ezAbstractMemberProperty*>(ezAnimGraphPin::GetStaticRTTI()->FindPropertyByName("PinIdx", false));
  EZ_ASSERT_DEBUG(pIdxProperty, "Missing PinIdx property");
  auto pNumProperty = static_cast<ezAbstractMemberProperty*>(ezAnimGraphPin::GetStaticRTTI()->FindPropertyByName("NumConnections", false));
  EZ_ASSERT_DEBUG(pNumProperty, "Missing NumConnections property");

  ezDynamicArray<ezAnimGraphNode*> newNodes;
  newNodes.Reserve(allNodes.GetCount());

  CreateOutputGraphNodes(allNodes, animController, newNodes);

  ezMap<const ezPin*, ezUInt16> inputPinIndices;
  SetInputPinIndices(newNodes, allNodes, pNodeManager, pinCounts, inputPinIndices, pIdxProperty, pNumProperty);
  SetOutputPinIndices(newNodes, allNodes, pNodeManager, pinCounts, animController, pIdxProperty, inputPinIndices);

  ezDefaultMemoryStreamStorage storage;
  ezMemoryStreamWriter writer(&storage);
  EZ_SUCCEED_OR_RETURN(animController.Serialize(writer));

  stream << storage.GetStorageSize32();
  return storage.CopyToStream(stream);
}

static void AssignNodePriority(const ezDocumentObject* pNode, ezUInt16 uiCurPrio, ezMap<const ezDocumentObject*, ezUInt16>& ref_prios, const ezDocumentNodeManager* pNodeManager)
{
  ref_prios[pNode] = ezMath::Min(ref_prios[pNode], uiCurPrio);

  const auto inputPins = pNodeManager->GetInputPins(pNode);

  for (auto& pPin : inputPins)
  {
    for (auto pConnection : pNodeManager->GetConnections(*pPin))
    {
      AssignNodePriority(pConnection->GetSourcePin().GetParent(), uiCurPrio - 1, ref_prios, pNodeManager);
    }
  }
}

void ezAnimationControllerAssetDocument::SortNodesByPriority(ezDynamicArray<const ezDocumentObject*>& allNodes)
{
  // starts at output nodes (which have no output pins) and walks back recursively over the connections on their input nodes
  // until it reaches the end of the graph
  // assigns decreasing priorities to the nodes that it finds
  // thus it generates a weak order in which the nodes should be stepped at runtime

  const auto* pNodeManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());

  ezMap<const ezDocumentObject*, ezUInt16> prios;
  for (const ezDocumentObject* pNode : allNodes)
  {
    prios[pNode] = 0xFFFF;
  }

  for (const ezDocumentObject* pNode : allNodes)
  {
    // only look at the final nodes in the graph
    if (pNodeManager->GetOutputPins(pNode).IsEmpty())
    {
      AssignNodePriority(pNode, 0xFFFE, prios, pNodeManager);
    }
  }

  // remove unreachable nodes
  for (ezUInt32 i = allNodes.GetCount(); i > 0; --i)
  {
    if (prios[allNodes[i - 1]] == 0xFFFF)
    {
      allNodes.RemoveAtAndSwap(i - 1);
    }
  }

  allNodes.Sort([&](auto lhs, auto rhs) -> bool { return prios[lhs] < prios[rhs]; });
}

void ezAnimationControllerAssetDocument::SetOutputPinIndices(const ezDynamicArray<ezAnimGraphNode*>& newNodes, const ezDynamicArray<const ezDocumentObject*>& allNodes, const ezDocumentNodeManager* pNodeManager, ezMap<ezUInt8, PinCount>& pinCounts, ezAnimGraph& animController, ezAbstractMemberProperty* pIdxProperty, const ezMap<const ezPin*, ezUInt16>& inputPinIndices) const
{
  // this function is generic and doesn't need to be extended for new types

  for (ezUInt32 nodeIdx = 0; nodeIdx < newNodes.GetCount(); ++nodeIdx)
  {
    const ezDocumentObject* pNode = allNodes[nodeIdx];
    auto* pNewNode = newNodes[nodeIdx];
    const auto outputPins = pNodeManager->GetOutputPins(pNode);

    for (auto& pPin : outputPins)
    {
      auto connections = pNodeManager->GetConnections(*pPin);
      if (connections.IsEmpty())
        continue;

      const ezAnimationControllerNodePin& ctrlPin = ezStaticCast<const ezAnimationControllerNodePin&>(*pPin);
      const ezUInt8 pinType = ctrlPin.m_DataType;

      const ezUInt32 idx = pinCounts[pinType].m_uiOutputIdx++;

      animController.m_OutputPinToInputPinMapping[pinType][idx].Reserve(connections.GetCount());

      auto pPinProp = static_cast<ezAbstractMemberProperty*>(pNewNode->GetDynamicRTTI()->FindPropertyByName(pPin->GetName()));
      EZ_ASSERT_DEBUG(pPinProp, "Pin with name '{}' has no equally named property", pPin->GetName());

      // set the output index to use by this pin
      ezReflectionUtils::SetMemberPropertyValue(pIdxProperty, pPinProp->GetPropertyPointer(pNewNode), idx);

      // set output pin to input pin mapping

      for (const auto pCon : connections)
      {
        const ezUInt16 uiTargetIdx = inputPinIndices.GetValueOrDefault(&pCon->GetTargetPin(), 0xFFFF);

        if (uiTargetIdx != 0xFFFF)
        {
          animController.m_OutputPinToInputPinMapping[pinType][idx].PushBack(uiTargetIdx);
        }
      }
    }
  }
}

void ezAnimationControllerAssetDocument::SetInputPinIndices(const ezDynamicArray<ezAnimGraphNode*>& newNodes, const ezDynamicArray<const ezDocumentObject*>& allNodes, const ezDocumentNodeManager* pNodeManager, ezMap<ezUInt8, PinCount>& pinCounts, ezMap<const ezPin*, ezUInt16>& inputPinIndices, ezAbstractMemberProperty* pIdxProperty, ezAbstractMemberProperty* pNumProperty) const
{
  // this function is generic and doesn't need to be extended for new types

  for (ezUInt32 nodeIdx = 0; nodeIdx < newNodes.GetCount(); ++nodeIdx)
  {
    const ezDocumentObject* pNode = allNodes[nodeIdx];
    auto* pNewNode = newNodes[nodeIdx];

    const auto inputPins = pNodeManager->GetInputPins(pNode);

    for (auto& pPin : inputPins)
    {
      auto connections = pNodeManager->GetConnections(*pPin);
      if (connections.IsEmpty())
        continue;

      const ezAnimationControllerNodePin& ctrlPin = ezStaticCast<const ezAnimationControllerNodePin&>(*pPin);
      const ezUInt16 idx = pinCounts[(ezUInt8)ctrlPin.m_DataType].m_uiInputIdx++;
      inputPinIndices[&ctrlPin] = idx;

      auto pPinProp = static_cast<ezAbstractMemberProperty*>(pNewNode->GetDynamicRTTI()->FindPropertyByName(pPin->GetName()));
      EZ_ASSERT_DEBUG(pPinProp, "Pin with name '{}' has no equally named property", pPin->GetName());

      ezReflectionUtils::SetMemberPropertyValue(pIdxProperty, pPinProp->GetPropertyPointer(pNewNode), idx);
      ezReflectionUtils::SetMemberPropertyValue(pNumProperty, pPinProp->GetPropertyPointer(pNewNode), connections.GetCount());
    }
  }
}

void ezAnimationControllerAssetDocument::CreateOutputGraphNodes(const ezDynamicArray<const ezDocumentObject*>& allNodes, ezAnimGraph& animController, ezDynamicArray<ezAnimGraphNode*>& newNodes) const
{
  // this function is generic and doesn't need to be extended for new types

  for (const ezDocumentObject* pNode : allNodes)
  {
    animController.m_Nodes.PushBack(pNode->GetType()->GetAllocator()->Allocate<ezAnimGraphNode>());
    newNodes.PushBack(animController.m_Nodes.PeekBack().Borrow());
    auto pNewNode = animController.m_Nodes.PeekBack().Borrow();

    // copy all the non-hidden properties
    ezToolsSerializationUtils::CopyProperties(pNode, GetObjectManager(), pNewNode, pNewNode->GetDynamicRTTI(), [](const ezAbstractProperty* p) { return p->GetAttributeByType<ezHiddenAttribute>() == nullptr; });
  }
}

void ezAnimationControllerAssetDocument::CountPinTypes(const ezDocumentNodeManager* pNodeManager, ezDynamicArray<const ezDocumentObject*>& allNodes, ezMap<ezUInt8, PinCount>& pinCounts) const
{
  // this function is generic and doesn't need to be extended for new types

  for (auto pNode : pNodeManager->GetRootObject()->GetChildren())
  {
    if (!pNodeManager->IsNode(pNode))
      continue;

    allNodes.PushBack(pNode);

    // input pins
    {
      const auto pins = pNodeManager->GetInputPins(pNode);

      for (auto& pPin : pins)
      {
        if (pNodeManager->HasConnections(*pPin) == false)
          continue;

        const ezAnimationControllerNodePin& ctrlPin = ezStaticCast<const ezAnimationControllerNodePin&>(*pPin);
        pinCounts[(ezUInt8)ctrlPin.m_DataType].m_uiInputCount++;
      }
    }

    // output pins
    {
      const auto pins = pNodeManager->GetOutputPins(pNode);

      for (auto& pPin : pins)
      {
        if (pNodeManager->HasConnections(*pPin) == false)
          continue;

        const ezAnimationControllerNodePin& ctrlPin = ezStaticCast<const ezAnimationControllerNodePin&>(*pPin);
        pinCounts[(ezUInt8)ctrlPin.m_DataType].m_uiOutputCount++;
      }
    }
  }
}

void ezAnimationControllerAssetDocument::InternalGetMetaDataHash(const ezDocumentObject* pObject, ezUInt64& inout_uiHash) const
{
  // without this, changing connections only (no property value) may not result in a different asset document hash and therefore no transform

  const ezDocumentNodeManager* pManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());
  pManager->GetMetaDataHash(pObject, inout_uiHash);
}

void ezAnimationControllerAssetDocument::AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const
{
  SUPER::AttachMetaDataBeforeSaving(graph);
  const ezDocumentNodeManager* pManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(graph);
}

void ezAnimationControllerAssetDocument::RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph, bool bUndoable)
{
  SUPER::RestoreMetaDataAfterLoading(graph, bUndoable);
  ezDocumentNodeManager* pManager = static_cast<ezDocumentNodeManager*>(GetObjectManager());
  pManager->RestoreMetaDataAfterLoading(graph, bUndoable);
}



void ezAnimationControllerAssetDocument::GetSupportedMimeTypesForPasting(ezHybridArray<ezString, 4>& out_MimeTypes) const
{
  out_MimeTypes.PushBack("application/ezEditor.AnimationControllerGraph");
}

bool ezAnimationControllerAssetDocument::CopySelectedObjects(ezAbstractObjectGraph& out_objectGraph, ezStringBuilder& out_MimeType) const
{
  out_MimeType = "application/ezEditor.AnimationControllerGraph";

  const ezDocumentNodeManager* pManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());
  return pManager->CopySelectedObjects(out_objectGraph);
}

bool ezAnimationControllerAssetDocument::Paste(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, const char* szMimeType)
{
  ezDocumentNodeManager* pManager = static_cast<ezDocumentNodeManager*>(GetObjectManager());
  return pManager->PasteObjects(info, objectGraph, ezQtNodeScene::GetLastMouseInteractionPos(), bAllowPickedPosition);
}

ezAnimationControllerNodePin::ezAnimationControllerNodePin(Type type, const char* szName, const ezColorGammaUB& color, const ezDocumentObject* pObject)
  : ezPin(type, szName, color, pObject)
{
}

ezAnimationControllerNodePin::~ezAnimationControllerNodePin() = default;
