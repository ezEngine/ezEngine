#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimationGraphAsset/AnimationGraphAsset.h>
#include <EditorPluginAssets/AnimationGraphAsset/AnimationGraphQt.h>
#include <Foundation/Math/ColorScheme.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes2/PoseResultAnimNode.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes2/SampleFrameAnimNode.h>
#include <ToolsFoundation/NodeObject/NodeCommandAccessor.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <ToolsFoundation/Serialization/ToolsSerializationUtils.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimationGraphAssetDocument, 5, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimationGraphNodePin, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimationGraphAssetProperties, 1, ezRTTIDefaultAllocator<ezAnimationGraphAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_MEMBER_PROPERTY("IncludeGraphs", m_IncludeGraphs)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Keyframe_Graph")),
    EZ_ARRAY_MEMBER_PROPERTY("AnimationClipMapping", m_AnimationClipMapping),
  }
    EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_SUBSYSTEM_DECLARATION(EditorPluginAssets, AnimationGraph)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezQtNodeScene::GetNodeFactory().RegisterCreator(ezGetStaticRTTI<ezAnimGraphNode>(), [](const ezRTTI* pRtti)->ezQtNode* { return new ezQtAnimationGraphNode(); });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezQtNodeScene::GetNodeFactory().UnregisterCreator(ezGetStaticRTTI<ezAnimGraphNode>());
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

bool ezAnimationGraphNodeManager::InternalIsNode(const ezDocumentObject* pObject) const
{
  auto pType = pObject->GetTypeAccessor().GetType();
  return pType->IsDerivedFrom<ezAnimGraphNode>();
}

void ezAnimationGraphNodeManager::InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& ref_node)
{
  auto pType = pObject->GetTypeAccessor().GetType();
  if (!pType->IsDerivedFrom<ezAnimGraphNode>())
    return;

  ezHybridArray<const ezAbstractProperty*, 32> properties;
  pType->GetAllProperties(properties);

  const ezColor triggerPinColor = ezColorScheme::DarkUI(ezColorScheme::Yellow);
  const ezColor numberPinColor = ezColorScheme::DarkUI(ezColorScheme::Lime);
  const ezColor boolPinColor = ezColorScheme::LightUI(ezColorScheme::Lime);
  const ezColor weightPinColor = ezColorScheme::DarkUI(ezColorScheme::Teal);
  const ezColor localPosePinColor = ezColorScheme::DarkUI(ezColorScheme::Blue);
  const ezColor modelPosePinColor = ezColorScheme::DarkUI(ezColorScheme::Grape);
  // EXTEND THIS if a new type is introduced

  ezHybridArray<ezString, 16> pinNames;

  for (auto pProp : properties)
  {
    if (!pProp->GetSpecificType()->IsDerivedFrom<ezAnimGraphPin>())
      continue;

    pinNames.Clear();

    if (pProp->GetCategory() == ezPropertyCategory::Array)
    {
      if (const ezDynamicPinAttribute* pDynPin = pProp->GetAttributeByType<ezDynamicPinAttribute>())
      {
        GetDynamicPinNames(pObject, pDynPin->GetProperty(), pProp->GetPropertyName(), pinNames);
      }
    }
    else if (pProp->GetCategory() == ezPropertyCategory::Member)
    {
      pinNames.PushBack(pProp->GetPropertyName());
    }

    for (ezUInt32 i = 0; i < pinNames.GetCount(); ++i)
    {
      const auto& pinName = pinNames[i];

      if (pProp->GetSpecificType()->IsDerivedFrom<ezAnimGraphTriggerInputPin>())
      {
        auto pPin = EZ_DEFAULT_NEW(ezAnimationGraphNodePin, ezPin::Type::Input, pinName, triggerPinColor, pObject);
        pPin->m_DataType = ezAnimGraphPin::Trigger;
        ref_node.m_Inputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<ezAnimGraphTriggerOutputPin>())
      {
        auto pPin = EZ_DEFAULT_NEW(ezAnimationGraphNodePin, ezPin::Type::Output, pinName, triggerPinColor, pObject);
        pPin->m_DataType = ezAnimGraphPin::Trigger;
        ref_node.m_Outputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<ezAnimGraphNumberInputPin>())
      {
        auto pPin = EZ_DEFAULT_NEW(ezAnimationGraphNodePin, ezPin::Type::Input, pinName, numberPinColor, pObject);
        pPin->m_DataType = ezAnimGraphPin::Number;
        ref_node.m_Inputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<ezAnimGraphNumberOutputPin>())
      {
        auto pPin = EZ_DEFAULT_NEW(ezAnimationGraphNodePin, ezPin::Type::Output, pinName, numberPinColor, pObject);
        pPin->m_DataType = ezAnimGraphPin::Number;
        ref_node.m_Outputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<ezAnimGraphBoolInputPin>())
      {
        auto pPin = EZ_DEFAULT_NEW(ezAnimationGraphNodePin, ezPin::Type::Input, pinName, boolPinColor, pObject);
        pPin->m_DataType = ezAnimGraphPin::Bool;
        ref_node.m_Inputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<ezAnimGraphBoolOutputPin>())
      {
        auto pPin = EZ_DEFAULT_NEW(ezAnimationGraphNodePin, ezPin::Type::Output, pinName, boolPinColor, pObject);
        pPin->m_DataType = ezAnimGraphPin::Bool;
        ref_node.m_Outputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<ezAnimGraphBoneWeightsInputPin>())
      {
        auto pPin = EZ_DEFAULT_NEW(ezAnimationGraphNodePin, ezPin::Type::Input, pinName, weightPinColor, pObject);
        pPin->m_DataType = ezAnimGraphPin::BoneWeights;
        ref_node.m_Inputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<ezAnimGraphBoneWeightsOutputPin>())
      {
        auto pPin = EZ_DEFAULT_NEW(ezAnimationGraphNodePin, ezPin::Type::Output, pinName, weightPinColor, pObject);
        pPin->m_DataType = ezAnimGraphPin::BoneWeights;
        ref_node.m_Outputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<ezAnimGraphLocalPoseInputPin>())
      {
        auto pPin = EZ_DEFAULT_NEW(ezAnimationGraphNodePin, ezPin::Type::Input, pinName, localPosePinColor, pObject);
        pPin->m_DataType = ezAnimGraphPin::LocalPose;
        ref_node.m_Inputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<ezAnimGraphLocalPoseMultiInputPin>())
      {
        auto pPin = EZ_DEFAULT_NEW(ezAnimationGraphNodePin, ezPin::Type::Input, pinName, localPosePinColor, pObject);
        pPin->m_DataType = ezAnimGraphPin::LocalPose;
        pPin->m_bMultiInputPin = true;
        pPin->m_Shape = ezPin::Shape::RoundRect;
        ref_node.m_Inputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<ezAnimGraphLocalPoseOutputPin>())
      {
        auto pPin = EZ_DEFAULT_NEW(ezAnimationGraphNodePin, ezPin::Type::Output, pinName, localPosePinColor, pObject);
        pPin->m_DataType = ezAnimGraphPin::LocalPose;
        ref_node.m_Outputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<ezAnimGraphModelPoseInputPin>())
      {
        auto pPin = EZ_DEFAULT_NEW(ezAnimationGraphNodePin, ezPin::Type::Input, pinName, modelPosePinColor, pObject);
        pPin->m_DataType = ezAnimGraphPin::ModelPose;
        ref_node.m_Inputs.PushBack(pPin);
      }
      else if (pProp->GetSpecificType()->IsDerivedFrom<ezAnimGraphModelPoseOutputPin>())
      {
        auto pPin = EZ_DEFAULT_NEW(ezAnimationGraphNodePin, ezPin::Type::Output, pinName, modelPosePinColor, pObject);
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
}

void ezAnimationGraphNodeManager::GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& ref_types) const
{
  ezSet<const ezRTTI*> typeSet;
  ezReflectionUtils::GatherTypesDerivedFromClass(ezGetStaticRTTI<ezAnimGraphNode>(), typeSet);

  ref_types.Clear();
  for (auto pType : typeSet)
  {
    if (pType->GetTypeFlags().IsAnySet(ezTypeFlags::Abstract))
      continue;

    ref_types.PushBack(pType);
  }
}

ezStatus ezAnimationGraphNodeManager::InternalCanConnect(const ezPin& source, const ezPin& target, CanConnectResult& out_result) const
{
  const ezAnimationGraphNodePin& sourcePin = ezStaticCast<const ezAnimationGraphNodePin&>(source);
  const ezAnimationGraphNodePin& targetPin = ezStaticCast<const ezAnimationGraphNodePin&>(target);

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

  if (out_result != CanConnectResult::ConnectNever && WouldConnectionCreateCircle(source, target))
  {
    out_result = CanConnectResult::ConnectNever;
    return ezStatus("Connecting these pins would create a circle in the graph.");
  }

  return ezStatus(EZ_SUCCESS);
}

bool ezAnimationGraphNodeManager::InternalIsDynamicPinProperty(const ezDocumentObject* pObject, const ezAbstractProperty* pProp) const
{
  return pProp->GetAttributeByType<ezDynamicPinAttribute>() != nullptr;
}

ezAnimationGraphAssetDocument::ezAnimationGraphAssetDocument(ezStringView sDocumentPath)
  : ezSimpleAssetDocument<ezAnimationGraphAssetProperties>(EZ_DEFAULT_NEW(ezAnimationGraphNodeManager), sDocumentPath, ezAssetDocEngineConnection::None)
{
  m_pObjectAccessor = EZ_DEFAULT_NEW(ezNodeCommandAccessor, GetCommandHistory());
}

ezTransformStatus ezAnimationGraphAssetDocument::InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  const auto* pNodeManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());

  auto pProp = GetProperties();

  {
    stream.WriteVersion(2);
    stream.WriteArray(pProp->m_IncludeGraphs).AssertSuccess();

    const ezUInt32 uiNum = pProp->m_AnimationClipMapping.GetCount();
    stream << uiNum;

    for (ezUInt32 i = 0; i < uiNum; ++i)
    {
      stream << pProp->m_AnimationClipMapping[i].m_sClipName;
      stream << pProp->m_AnimationClipMapping[i].m_hClip;
    }
  }

  // find all 'nodes'
  ezDynamicArray<const ezDocumentObject*> allNodes;
  for (auto pNode : pNodeManager->GetRootObject()->GetChildren())
  {
    if (!pNodeManager->IsNode(pNode))
      continue;

    allNodes.PushBack(pNode);
  }

  ezAnimGraph animGraph;

  ezMap<const ezDocumentObject*, ezAnimGraphNode*> docNodeToRuntimeNode;

  // create all nodes in the ezAnimGraph
  {
    for (const ezDocumentObject* pNode : allNodes)
    {
      ezAnimGraphNode* pNewNode = animGraph.AddNode(pNode->GetType()->GetAllocator()->Allocate<ezAnimGraphNode>());

      // copy all the non-hidden properties
      ezToolsSerializationUtils::CopyProperties(pNode, GetObjectManager(), pNewNode, pNewNode->GetDynamicRTTI(), [](const ezAbstractProperty* p)
        { return p->GetAttributeByType<ezHiddenAttribute>() == nullptr; });

      docNodeToRuntimeNode[pNode] = pNewNode;
    }
  }

  // add all node connections to the ezAnimGraph
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
          ezAnimGraphNode* pDstNode = docNodeToRuntimeNode[pCon->GetTargetPin().GetParent()];

          animGraph.AddConnection(pSrcNode, pCon->GetSourcePin().GetName(), pDstNode, pCon->GetTargetPin().GetName());
        }
      }
    }
  }

  EZ_SUCCEED_OR_RETURN(animGraph.Serialize(stream));

  return ezTransformStatus(EZ_SUCCESS);
}

void ezAnimationGraphAssetDocument::InternalGetMetaDataHash(const ezDocumentObject* pObject, ezUInt64& inout_uiHash) const
{
  // without this, changing connections only (no property value) may not result in a different asset document hash and therefore no transform

  const ezDocumentNodeManager* pManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());
  pManager->GetMetaDataHash(pObject, inout_uiHash);
}

void ezAnimationGraphAssetDocument::AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const
{
  SUPER::AttachMetaDataBeforeSaving(graph);
  const ezDocumentNodeManager* pManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(graph);
}

void ezAnimationGraphAssetDocument::RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph, bool bUndoable)
{
  SUPER::RestoreMetaDataAfterLoading(graph, bUndoable);
  ezDocumentNodeManager* pManager = static_cast<ezDocumentNodeManager*>(GetObjectManager());
  pManager->RestoreMetaDataAfterLoading(graph, bUndoable);
}



void ezAnimationGraphAssetDocument::GetSupportedMimeTypesForPasting(ezHybridArray<ezString, 4>& out_MimeTypes) const
{
  out_MimeTypes.PushBack("application/ezEditor.AnimationGraphGraph");
}

bool ezAnimationGraphAssetDocument::CopySelectedObjects(ezAbstractObjectGraph& out_objectGraph, ezStringBuilder& out_MimeType) const
{
  out_MimeType = "application/ezEditor.AnimationGraphGraph";

  const ezDocumentNodeManager* pManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());
  return pManager->CopySelectedObjects(out_objectGraph);
}

bool ezAnimationGraphAssetDocument::Paste(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, ezStringView sMimeType)
{
  ezDocumentNodeManager* pManager = static_cast<ezDocumentNodeManager*>(GetObjectManager());
  return pManager->PasteObjects(info, objectGraph, ezQtNodeScene::GetLastMouseInteractionPos(), bAllowPickedPosition);
}

ezAnimationGraphNodePin::ezAnimationGraphNodePin(Type type, const char* szName, const ezColorGammaUB& color, const ezDocumentObject* pObject)
  : ezPin(type, szName, color, pObject)
{
}

ezAnimationGraphNodePin::~ezAnimationGraphNodePin() = default;
