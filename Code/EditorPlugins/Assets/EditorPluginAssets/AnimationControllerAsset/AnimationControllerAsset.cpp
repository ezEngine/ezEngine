#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimationControllerAsset/AnimationControllerAsset.h>
#include <EditorPluginAssets/AnimationControllerAsset/AnimationControllerGraphQt.h>
#include <Foundation/Math/ColorScheme.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes2/PoseResultAnimNode.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes2/SampleFrameAnimNode.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <ToolsFoundation/Serialization/ToolsSerializationUtils.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimationControllerAssetDocument, 4, ezRTTINoAllocator)
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

  const ezColor triggerPinColor = ezColorScheme::DarkUI(ezColorScheme::Yellow);
  const ezColor numberPinColor = ezColorScheme::DarkUI(ezColorScheme::Lime);
  const ezColor boolPinColor = ezColorScheme::LightUI(ezColorScheme::Lime);
  const ezColor weightPinColor = ezColorScheme::DarkUI(ezColorScheme::Teal);
  const ezColor localPosePinColor = ezColorScheme::DarkUI(ezColorScheme::Blue);
  const ezColor modelPosePinColor = ezColorScheme::DarkUI(ezColorScheme::Grape);
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
    else if (pProp->GetSpecificType()->IsDerivedFrom<ezAnimGraphBoolInputPin>())
    {
      auto pPin = EZ_DEFAULT_NEW(ezAnimationControllerNodePin, ezPin::Type::Input, pProp->GetPropertyName(), boolPinColor, pObject);
      pPin->m_DataType = ezAnimGraphPin::Bool;
      ref_node.m_Inputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<ezAnimGraphBoolOutputPin>())
    {
      auto pPin = EZ_DEFAULT_NEW(ezAnimationControllerNodePin, ezPin::Type::Output, pProp->GetPropertyName(), boolPinColor, pObject);
      pPin->m_DataType = ezAnimGraphPin::Bool;
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

    case ezAnimGraphPin::Bool:
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

  // find all 'nodes'
  ezDynamicArray<const ezDocumentObject*> allNodes;
  for (auto pNode : pNodeManager->GetRootObject()->GetChildren())
  {
    if (!pNodeManager->IsNode(pNode))
      continue;

    allNodes.PushBack(pNode);
  }

  // if the asset is entirely empty, don't complain
  if (allNodes.IsEmpty())
    return ezStatus(EZ_SUCCESS);

  ezAnimGraphBuilder builder;

  ezMap<const ezDocumentObject*, const ezAnimGraphNode*> docNodeToRuntimeNode;

  // create all nodes in the ezAnimGraphBuilder
  {
    for (const ezDocumentObject* pNode : allNodes)
    {
      ezAnimGraphNode* pNewNode = builder.AddNode(pNode->GetType()->GetAllocator()->Allocate<ezAnimGraphNode>());

      // copy all the non-hidden properties
      ezToolsSerializationUtils::CopyProperties(pNode, GetObjectManager(), pNewNode, pNewNode->GetDynamicRTTI(), [](const ezAbstractProperty* p) { return p->GetAttributeByType<ezHiddenAttribute>() == nullptr; });

      docNodeToRuntimeNode[pNode] = pNewNode;
    }
  }

  // add all node connections to the ezAnimGraphBuilder
  {
    for (ezUInt32 nodeIdx = 0; nodeIdx < allNodes.GetCount(); ++nodeIdx)
    {
      const ezDocumentObject* pNode = allNodes[nodeIdx];

      const auto outputPins = pNodeManager->GetOutputPins(pNode);

      for (auto& pPin : outputPins)
      {
        for (const ezConnection* pCon : pNodeManager->GetConnections(*pPin))
        {
          const ezAnimGraphNode* pSrcNode = docNodeToRuntimeNode[pCon->GetSourcePin().GetParent()];
          const ezAnimGraphNode* pDstNode = docNodeToRuntimeNode[pCon->GetTargetPin().GetParent()];

          builder.AddConnection(pSrcNode, pCon->GetSourcePin().GetName(), pDstNode, pCon->GetTargetPin().GetName());
        }
      }
    }
  }

  ezDefaultMemoryStreamStorage storage;
  ezMemoryStreamWriter writer(&storage);

  EZ_SUCCEED_OR_RETURN(builder.Serialize(writer));

  stream << storage.GetStorageSize32();
  return storage.CopyToStream(stream);
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
