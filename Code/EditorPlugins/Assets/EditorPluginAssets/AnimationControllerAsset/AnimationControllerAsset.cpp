#include <EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/AnimationControllerAsset/AnimationControllerAsset.h>
#include <EditorPluginAssets/AnimationControllerAsset/AnimationControllerAssetManager.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/Pipeline/RenderPipelineNode.h>
#include <ToolsFoundation/Command/NodeCommands.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimationControllerAssetDocument, 3, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimationControllerNodePin, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

bool ezAnimationControllerNodeManager::InternalIsNode(const ezDocumentObject* pObject) const
{
  auto pType = pObject->GetTypeAccessor().GetType();
  return pType->IsDerivedFrom<ezAnimGraphNode>();
}

void ezAnimationControllerNodeManager::InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& node)
{
  auto pType = pObject->GetTypeAccessor().GetType();
  if (!pType->IsDerivedFrom<ezAnimGraphNode>())
    return;

  ezHybridArray<ezAbstractProperty*, 32> properties;
  pType->GetAllProperties(properties);

  const ezColor triggerPinColor = ezColor::DarkOrange;
  const ezColor numberPinColor = ezColor::ForestGreen;

  for (ezAbstractProperty* pProp : properties)
  {
    if (pProp->GetCategory() != ezPropertyCategory::Member || !pProp->GetSpecificType()->IsDerivedFrom<ezAnimGraphPin>())
      continue;

    if (pProp->GetSpecificType()->IsDerivedFrom<ezAnimGraphTriggerInputPin>())
    {
      ezAnimationControllerNodePin* pPin = EZ_DEFAULT_NEW(ezAnimationControllerNodePin, ezPin::Type::Input, pProp->GetPropertyName(), triggerPinColor, pObject);
      pPin->m_DataType = ezAnimGraphPin::Trigger;
      node.m_Inputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<ezAnimGraphTriggerOutputPin>())
    {
      ezAnimationControllerNodePin* pPin = EZ_DEFAULT_NEW(ezAnimationControllerNodePin, ezPin::Type::Output, pProp->GetPropertyName(), triggerPinColor, pObject);
      pPin->m_DataType = ezAnimGraphPin::Trigger;
      node.m_Outputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<ezAnimGraphNumberInputPin>())
    {
      ezAnimationControllerNodePin* pPin = EZ_DEFAULT_NEW(ezAnimationControllerNodePin, ezPin::Type::Input, pProp->GetPropertyName(), numberPinColor, pObject);
      pPin->m_DataType = ezAnimGraphPin::Number;
      node.m_Inputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<ezAnimGraphNumberOutputPin>())
    {
      ezAnimationControllerNodePin* pPin = EZ_DEFAULT_NEW(ezAnimationControllerNodePin, ezPin::Type::Output, pProp->GetPropertyName(), numberPinColor, pObject);
      pPin->m_DataType = ezAnimGraphPin::Number;
      node.m_Outputs.PushBack(pPin);
    }

    // EXTEND THIS if a new type is introduced
  }
}

void ezAnimationControllerNodeManager::InternalDestroyPins(const ezDocumentObject* pObject, NodeInternal& node)
{
  for (ezPin* pPin : node.m_Inputs)
  {
    EZ_DEFAULT_DELETE(pPin);
  }
  node.m_Inputs.Clear();
  for (ezPin* pPin : node.m_Outputs)
  {
    EZ_DEFAULT_DELETE(pPin);
  }
  node.m_Outputs.Clear();
}


void ezAnimationControllerNodeManager::GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& Types) const
{
  ezSet<const ezRTTI*> typeSet;
  ezReflectionUtils::GatherTypesDerivedFromClass(ezGetStaticRTTI<ezAnimGraphNode>(), typeSet, false);

  Types.Clear();
  for (auto pType : typeSet)
  {
    if (pType->GetTypeFlags().IsAnySet(ezTypeFlags::Abstract))
      continue;

    Types.PushBack(pType);
  }
}

ezStatus ezAnimationControllerNodeManager::InternalCanConnect(const ezPin* pSource0, const ezPin* pTarget0, CanConnectResult& out_Result) const
{
  const ezAnimationControllerNodePin* pSourcePin = ezStaticCast<const ezAnimationControllerNodePin*>(pSource0);
  const ezAnimationControllerNodePin* pTargetPin = ezStaticCast<const ezAnimationControllerNodePin*>(pTarget0);

  out_Result = CanConnectResult::ConnectNever;

  if (pSourcePin->m_DataType != pTargetPin->m_DataType)
    return ezStatus("Can't connect pins of different data types");

  if (pSourcePin->GetType() == pTargetPin->GetType())
    return ezStatus("Can only connect input pins with output pins.");

  switch (pSourcePin->m_DataType)
  {
    case ezAnimGraphPin::Trigger:
      out_Result = CanConnectResult::ConnectNtoN;
      break;

    case ezAnimGraphPin::Number:
      out_Result = CanConnectResult::Connect1toN;
      break;

      //case ezAnimGraphPin::SkeletonMask:
      //  out_Result = CanConnectResult::Connect1toN;
      //  break;

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return ezStatus(EZ_SUCCESS);
}

ezAnimationControllerAssetDocument::ezAnimationControllerAssetDocument(const char* szDocumentPath)
  : ezAssetDocument(szDocumentPath, EZ_DEFAULT_NEW(ezAnimationControllerNodeManager), ezAssetDocEngineConnection::None)
{
}

ezStatus ezAnimationControllerAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  const auto* pNodeManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());

  ezDynamicArray<const ezDocumentObject*> allNodes;
  ezMap<ezUInt8, PinCount> pinCounts;

  CountPinTypes(pNodeManager, allNodes, pinCounts);

  ezAnimGraph animController;
  animController.m_TriggerInputPinStates.SetCount(pinCounts[ezAnimGraphPin::Trigger].m_uiInputCount);
  animController.m_NumberInputPinStates.SetCount(pinCounts[ezAnimGraphPin::Number].m_uiInputCount);
  // EXTEND THIS if a new type is introduced

  for (ezUInt32 i = 0; i < ezAnimGraphPin::ENUM_COUNT; ++i)
  {
    animController.m_OutputPinToInputPinMapping[i].SetCount(pinCounts[i].m_uiOutputCount);
  }

  auto pIdxProperty = static_cast<ezAbstractMemberProperty*>(ezAnimGraphPin::GetStaticRTTI()->FindPropertyByName("PinIdx", false));
  EZ_ASSERT_DEBUG(pIdxProperty, "Missing PinIdx property");

  ezDynamicArray<ezAnimGraphNode*> newNodes;
  newNodes.Reserve(allNodes.GetCount());

  CreateOutputGraphNodes(allNodes, animController, newNodes);

  ezMap<const ezPin*, ezUInt16> inputPinIndices;
  SetInputPinIndices(newNodes, allNodes, pNodeManager, pinCounts, inputPinIndices, pIdxProperty);
  SetOutputPinIndices(newNodes, allNodes, pNodeManager, pinCounts, animController, pIdxProperty, inputPinIndices);

  ezMemoryStreamStorage storage;
  ezMemoryStreamWriter writer(&storage);
  EZ_SUCCEED_OR_RETURN(animController.Serialize(writer));

  stream << storage.GetStorageSize();
  return stream.WriteBytes(storage.GetData(), storage.GetStorageSize());
}

void ezAnimationControllerAssetDocument::SetOutputPinIndices(const ezDynamicArray<ezAnimGraphNode*>& newNodes, const ezDynamicArray<const ezDocumentObject*>& allNodes, const ezDocumentNodeManager* pNodeManager, ezMap<ezUInt8, PinCount>& pinCounts, ezAnimGraph& animController, ezAbstractMemberProperty* pIdxProperty, const ezMap<const ezPin*, ezUInt16>& inputPinIndices) const
{
  // this function is generic and doesn't need to be extended for new types

  for (ezUInt32 nodeIdx = 0; nodeIdx < newNodes.GetCount(); ++nodeIdx)
  {
    const ezDocumentObject* pNode = allNodes[nodeIdx];
    auto* pNewNode = newNodes[nodeIdx];
    const auto outputPins = pNodeManager->GetOutputPins(pNode);

    for (auto pPin : outputPins)
    {
      const ezAnimationControllerNodePin* pCtrlPin = ezStaticCast<const ezAnimationControllerNodePin*>(pPin);

      if (pCtrlPin->GetConnections().IsEmpty())
        continue;

      const ezUInt8 pinType = pCtrlPin->m_DataType;

      const ezUInt32 idx = pinCounts[pinType].m_uiOutputIdx++;

      animController.m_OutputPinToInputPinMapping[pinType][idx].Reserve(pCtrlPin->GetConnections().GetCount());

      auto pPinProp = static_cast<ezAbstractMemberProperty*>(pNewNode->GetDynamicRTTI()->FindPropertyByName(pPin->GetName()));
      EZ_ASSERT_DEBUG(pPinProp, "Pin with name '{}' has no equally named property", pPin->GetName());

      ezReflectionUtils::SetMemberPropertyValue(pIdxProperty, pPinProp->GetPropertyPointer(pNewNode), idx);

      // set output pin to input pin mapping

      for (const auto pCon : pCtrlPin->GetConnections())
      {
        const ezUInt16 uiTargetIdx = inputPinIndices.GetValueOrDefault(pCon->GetTargetPin(), 0xFFFF);
        EZ_ASSERT_DEBUG(uiTargetIdx != 0xFFFF, "invalid target pin");

        animController.m_OutputPinToInputPinMapping[pinType][idx].PushBack(uiTargetIdx);
      }
    }
  }
}

void ezAnimationControllerAssetDocument::SetInputPinIndices(const ezDynamicArray<ezAnimGraphNode*>& newNodes, const ezDynamicArray<const ezDocumentObject*>& allNodes, const ezDocumentNodeManager* pNodeManager, ezMap<ezUInt8, PinCount>& pinCounts, ezMap<const ezPin*, ezUInt16>& inputPinIndices, ezAbstractMemberProperty* pIdxProperty) const
{
  // this function is generic and doesn't need to be extended for new types

  for (ezUInt32 nodeIdx = 0; nodeIdx < newNodes.GetCount(); ++nodeIdx)
  {
    const ezDocumentObject* pNode = allNodes[nodeIdx];
    auto* pNewNode = newNodes[nodeIdx];

    const auto inputPins = pNodeManager->GetInputPins(pNode);

    for (auto pPin : inputPins)
    {
      const ezAnimationControllerNodePin* pCtrlPin = ezStaticCast<const ezAnimationControllerNodePin*>(pPin);

      if (pCtrlPin->GetConnections().IsEmpty())
        continue;

      const ezUInt16 idx = pinCounts[(ezUInt8)pCtrlPin->m_DataType].m_uiInputIdx++;
      inputPinIndices[pCtrlPin] = idx;

      auto pPinProp = static_cast<ezAbstractMemberProperty*>(pNewNode->GetDynamicRTTI()->FindPropertyByName(pPin->GetName()));
      EZ_ASSERT_DEBUG(pPinProp, "Pin with name '{}' has no equally named property", pPin->GetName());

      ezReflectionUtils::SetMemberPropertyValue(pIdxProperty, pPinProp->GetPropertyPointer(pNewNode), idx);
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

    // copy properties
    {
      ezHybridArray<ezAbstractProperty*, 32> properties;
      pNode->GetType()->GetAllProperties(properties);

      for (ezAbstractProperty* pProp : properties)
      {
        if (pProp->GetCategory() != ezPropertyCategory::Member)
          continue;

        if (pProp->GetSpecificType()->GetTypeFlags().IsAnySet(ezTypeFlags::StandardType | ezTypeFlags::IsEnum | ezTypeFlags::Bitflags))
        {
          ezReflectionUtils::SetMemberPropertyValue(static_cast<ezAbstractMemberProperty*>(pProp), pNewNode, pNode->GetTypeAccessor().GetValue(pProp->GetPropertyName()));
        }
      }
    }
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

      for (auto pPin : pins)
      {
        const ezAnimationControllerNodePin* pCtrlPin = ezStaticCast<const ezAnimationControllerNodePin*>(pPin);

        if (pCtrlPin->GetConnections().IsEmpty())
          continue;

        pinCounts[(ezUInt8)pCtrlPin->m_DataType].m_uiInputCount++;
      }
    }

    // output pins
    {
      const auto pins = pNodeManager->GetOutputPins(pNode);

      for (auto pPin : pins)
      {
        const ezAnimationControllerNodePin* pCtrlPin = ezStaticCast<const ezAnimationControllerNodePin*>(pPin);

        if (pCtrlPin->GetConnections().IsEmpty())
          continue;

        pinCounts[(ezUInt8)pCtrlPin->m_DataType].m_uiOutputCount++;
      }
    }
  }
}

void ezAnimationControllerAssetDocument::InternalGetMetaDataHash(const ezDocumentObject* pObject, ezUInt64& inout_uiHash) const
{
  // const ezDocumentNodeManager* pManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());
  // if (pManager->IsNode(pObject))
  //{
  //  auto outputs = pManager->GetOutputPins(pObject);
  //  for (const ezPin* pPinSource : outputs)
  //  {
  //    auto inputs = pPinSource->GetConnections();
  //    for (const ezConnection* pConnection : inputs)
  //    {
  //      const ezPin* pPinTarget = pConnection->GetTargetPin();

  //      inout_uiHash = ezHashingUtils::xxHash64(&pPinSource->GetParent()->GetGuid(), sizeof(ezUuid), inout_uiHash);
  //      inout_uiHash = ezHashingUtils::xxHash64(&pPinTarget->GetParent()->GetGuid(), sizeof(ezUuid), inout_uiHash);
  //      inout_uiHash = ezHashingUtils::xxHash64(pPinSource->GetName(), ezStringUtils::GetStringElementCount(pPinSource->GetName()), inout_uiHash);
  //      inout_uiHash = ezHashingUtils::xxHash64(pPinTarget->GetName(), ezStringUtils::GetStringElementCount(pPinTarget->GetName()), inout_uiHash);
  //    }
  //  }
  //}
}

void ezAnimationControllerAssetDocument::AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const
{
  const ezDocumentNodeManager* pManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(graph);
}

void ezAnimationControllerAssetDocument::RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph, bool bUndoable)
{
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

  const auto& selection = GetSelectionManager()->GetSelection();

  if (selection.IsEmpty())
    return false;

  const ezDocumentNodeManager* pManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());

  ezDocumentObjectConverterWriter writer(&out_objectGraph, pManager);

  for (const ezDocumentObject* pNode : selection)
  {
    // objects are required to be named root but this is not enforced or obvious by the interface.
    writer.AddObjectToGraph(pNode, "root");
  }

  pManager->AttachMetaDataBeforeSaving(out_objectGraph);

  return true;
}

bool ezAnimationControllerAssetDocument::Paste(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, const char* szMimeType)
{
  bool bAddedAll = true;

  ezDeque<const ezDocumentObject*> AddedNodes;

  for (const PasteInfo& pi : info)
  {
    // only add nodes that are allowed to be added
    if (GetObjectManager()->CanAdd(pi.m_pObject->GetTypeAccessor().GetType(), nullptr, "Children", pi.m_Index).m_Result.Succeeded())
    {
      AddedNodes.PushBack(pi.m_pObject);
      GetObjectManager()->AddObject(pi.m_pObject, nullptr, "Children", pi.m_Index);
    }
    else
    {
      bAddedAll = false;
    }
  }

  m_DocumentObjectMetaData.RestoreMetaDataFromAbstractGraph(objectGraph);

  RestoreMetaDataAfterLoading(objectGraph, true);

  if (!AddedNodes.IsEmpty() && bAllowPickedPosition)
  {
    ezDocumentNodeManager* pManager = static_cast<ezDocumentNodeManager*>(GetObjectManager());

    ezVec2 vAvgPos(0);
    for (const ezDocumentObject* pNode : AddedNodes)
    {
      vAvgPos += pManager->GetNodePos(pNode);
    }

    vAvgPos /= AddedNodes.GetCount();

    const ezVec2 vMoveNode = -vAvgPos + ezQtNodeScene::GetLastMouseInteractionPos();

    for (const ezDocumentObject* pNode : AddedNodes)
    {
      ezMoveNodeCommand move;
      move.m_Object = pNode->GetGuid();
      move.m_NewPos = pManager->GetNodePos(pNode) + vMoveNode;
      GetCommandHistory()->AddCommand(move);
    }

    if (!bAddedAll)
    {
      ezLog::Info("[EditorStatus]Not all nodes were allowed to be added to the document");
    }
  }

  GetSelectionManager()->SetSelection(AddedNodes);
  return true;
}

ezAnimationControllerNodePin::ezAnimationControllerNodePin(Type type, const char* szName, const ezColorGammaUB& color, const ezDocumentObject* pObject)
  : ezPin(type, szName, color, pObject)
{
}

ezAnimationControllerNodePin::~ezAnimationControllerNodePin() = default;
