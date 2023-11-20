#include <EditorPluginAi/EditorPluginAiPCH.h>

#include <AiPlugin/UtilityAI/Framework/AiControllerDesc.h>
#include <AiPlugin/UtilityAI/Framework/AiInputPin.h>
#include <EditorPluginAi/AiControllerAsset/AiControllerAsset.h>
#include <EditorPluginAi/AiControllerAsset/AiControllerGraphQt.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Math/ColorScheme.h>
#include <GuiFoundation/VisualGraph/Scene.moc.h>
#include <ToolsFoundation/Serialization/ToolsSerializationUtils.h>
#include <ToolsFoundation/VisualGraph/VisualGraphCommandAccessor.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiControllerAssetDocument, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiControllerPin, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_SUBSYSTEM_DECLARATION(EditorPluginAi, AiControllerGraph)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    ezQtVisualGraphScene::GetNodeFactory().RegisterCreator(ezGetStaticRTTI<ezAiSensor>(), [](const ezRTTI* pRtti)->ezQtVisualGraphNode* { return new ezQtAiControllerNode(); });
    ezQtVisualGraphScene::GetNodeFactory().RegisterCreator(ezGetStaticRTTI<ezAiPerceptionGenerator>(), [](const ezRTTI* pRtti)->ezQtVisualGraphNode* { return new ezQtAiControllerNode(); });
    ezQtVisualGraphScene::GetNodeFactory().RegisterCreator(ezGetStaticRTTI<ezAiBehavior>(), [](const ezRTTI* pRtti)->ezQtVisualGraphNode* { return new ezQtAiControllerNode(); });
    ezQtVisualGraphScene::GetNodeFactory().RegisterCreator(ezGetStaticRTTI<ezAiConsideration>(), [](const ezRTTI* pRtti)->ezQtVisualGraphNode* { return new ezQtAiControllerNode(); });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    ezQtVisualGraphScene::GetNodeFactory().UnregisterCreator(ezGetStaticRTTI<ezAiSensor>());
    ezQtVisualGraphScene::GetNodeFactory().UnregisterCreator(ezGetStaticRTTI<ezAiPerceptionGenerator>());
    ezQtVisualGraphScene::GetNodeFactory().UnregisterCreator(ezGetStaticRTTI<ezAiBehavior>());
    ezQtVisualGraphScene::GetNodeFactory().UnregisterCreator(ezGetStaticRTTI<ezAiConsideration>());
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

ezAiControllerPin::ezAiControllerPin(Type type, ezStringView sName, const ezColorGammaUB& color, const ezDocumentObject* pObject, const ezRTTI* pDataType)
  : ezVisualGraphPin(type, sName, color, pObject)
  , m_pDataType(pDataType)
{
}

ezStringView ezAiControllerPin::GetDataTypeName() const
{
  return m_pDataType ? m_pDataType->GetTypeName() : ezStringView();
}

//////////////////////////////////////////////////////////////////////////

namespace
{
  ezColorGammaUB GetPinColor(const ezRTTI* pDataType)
  {
    if (pDataType->IsDerivedFrom<ezAiPerception>())
      return ezColorScheme::DarkUI(ezColorScheme::Violet);

    switch (ezAiControllerNodeManager::GetNodeKind(pDataType))
    {
      case ezAiControllerNodeManager::NodeKind::Sensor:
        return ezColorScheme::DarkUI(ezColorScheme::Cyan);
      case ezAiControllerNodeManager::NodeKind::PerceptionGenerator:
        return ezColorScheme::DarkUI(ezColorScheme::Violet);
      case ezAiControllerNodeManager::NodeKind::Consideration:
        return ezColorScheme::DarkUI(ezColorScheme::Lime);
      default:
        return ezColorScheme::DarkUI(ezColorScheme::Gray);
    }
  }
} // namespace

ezAiControllerNodeManager::NodeKind ezAiControllerNodeManager::GetNodeKind(const ezRTTI* pType)
{
  if (pType->IsDerivedFrom<ezAiSensor>())
    return NodeKind::Sensor;
  if (pType->IsDerivedFrom<ezAiPerceptionGenerator>())
    return NodeKind::PerceptionGenerator;
  if (pType->IsDerivedFrom<ezAiBehavior>())
    return NodeKind::Behavior;
  if (pType->IsDerivedFrom<ezAiConsideration>())
    return NodeKind::Consideration;

  return NodeKind::None;
}

ezStringView ezAiControllerNodeManager::GetShortTypeName(const ezRTTI* pType)
{
  ezStringView sName = pType->GetTypeName();

  for (ezStringView sPrefix : {"ezAiSensor"_ezsv, "ezAiPerceptionGen"_ezsv, "ezAiBehavior"_ezsv, "ezAiConsideration"_ezsv})
  {
    if (sName.StartsWith(sPrefix) && sName.GetElementCount() > sPrefix.GetElementCount())
    {
      sName.Shrink(sPrefix.GetElementCount(), 0);
      break;
    }
  }

  return sName;
}

void ezAiControllerNodeManager::GetNodeCreationTemplates(ezDynamicArray<ezVisualGraphNodeDesc>& out_templates) const
{
  static const ezHashedString s_sSensors = ezMakeHashedString("Sensors");
  static const ezHashedString s_sGenerators = ezMakeHashedString("Perception Generators");
  static const ezHashedString s_sBehaviors = ezMakeHashedString("Behaviors");
  static const ezHashedString s_sConsiderations = ezMakeHashedString("Considerations");

  ezTempHybridArray<const ezRTTI*, 32> types;
  GetCreateableTypes(types);

  for (const ezRTTI* pType : types)
  {
    auto& nodeTemplate = out_templates.ExpandAndGetRef();
    nodeTemplate.m_pType = pType;
    nodeTemplate.m_sTypeName = GetShortTypeName(pType);

    switch (GetNodeKind(pType))
    {
      case NodeKind::Sensor:
        nodeTemplate.m_sCategory = s_sSensors;
        break;
      case NodeKind::PerceptionGenerator:
        nodeTemplate.m_sCategory = s_sGenerators;
        break;
      case NodeKind::Behavior:
        nodeTemplate.m_sCategory = s_sBehaviors;
        break;
      case NodeKind::Consideration:
        nodeTemplate.m_sCategory = s_sConsiderations;
        break;
      default:
        break;
    }
  }
}

bool ezAiControllerNodeManager::InternalIsNode(const ezDocumentObject* pObject) const
{
  return GetNodeKind(pObject->GetTypeAccessor().GetType()) != NodeKind::None;
}

void ezAiControllerNodeManager::InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& ref_node)
{
  const ezRTTI* pType = pObject->GetType();
  const NodeKind kind = GetNodeKind(pType);

  ezTempHybridArray<const ezAbstractProperty*, 32> properties;
  pType->GetAllProperties(properties);

  for (const ezAbstractProperty* pProp : properties)
  {
    if (pProp->GetCategory() != ezPropertyCategory::Member || !pProp->GetSpecificType()->IsDerivedFrom<ezAiInputPin>())
      continue;

    const ezAiInputPinAttribute* pAttr = pProp->GetAttributeByType<ezAiInputPinAttribute>();
    const ezRTTI* pAccepted = pAttr ? pAttr->GetNodeType() : nullptr;

    if (pAccepted == nullptr)
    {
      ezLog::Warning("{}::{}: the input pin has no valid ezAiInputPinAttribute.", pType->GetTypeName(), pProp->GetPropertyName());
      continue;
    }

    auto pPin = EZ_DEFAULT_NEW(ezAiControllerPin, ezVisualGraphPin::Type::Input, pProp->GetPropertyName(), GetPinColor(pAccepted), pObject, pAccepted);
    pPin->m_pInputAttribute = pAttr;
    ref_node.m_Inputs.PushBack(pPin);
  }

  if (kind == NodeKind::Sensor || kind == NodeKind::PerceptionGenerator || kind == NodeKind::Consideration)
  {
    const ezRTTI* pDataType = pType;
    if (kind == NodeKind::PerceptionGenerator)
    {
      if (const ezRTTI* pPerceptionType = ezAiPerceptionGenerator::GetPerceptionType(pType))
        pDataType = pPerceptionType;
      else
        ezLog::Warning("{}: the type has no valid ezAiPerceptionTypeAttribute, behaviors can't connect to it.", pType->GetTypeName());
    }

    auto pPin = EZ_DEFAULT_NEW(ezAiControllerPin, ezVisualGraphPin::Type::Output, "Out", GetPinColor(pDataType), pObject, pDataType);
    pPin->m_pNodeType = pType;
    ref_node.m_Outputs.PushBack(pPin);
  }
}

ezStatus ezAiControllerNodeManager::InternalCanConnect(const ezVisualGraphPin& source, const ezVisualGraphPin& target, CanConnectResult& out_result) const
{
  out_result = CanConnectResult::ConnectNever;

  if (source.GetType() == target.GetType())
    return ezStatus("Can only connect input pins with output pins.");

  const ezAiControllerPin& sourcePin = static_cast<const ezAiControllerPin&>(source);
  const ezAiControllerPin& targetPin = static_cast<const ezAiControllerPin&>(target);

  if (targetPin.m_pInputAttribute == nullptr || !targetPin.m_pInputAttribute->Accepts(sourcePin.m_pNodeType))
    return ezStatus(ezFmt("The pin only accepts '{}'.", targetPin.GetDataTypeName()));

  out_result = CanConnectResult::ConnectNtoN;
  return ezStatus(EZ_SUCCESS);
}

void ezAiControllerNodeManager::GetCreateableTypes(ezDynamicArray<const ezRTTI*>& out_types) const
{
  const ezBitflags<ezRTTI::ForEachOptions> options = ezRTTI::ForEachOptions::ExcludeAbstract | ezRTTI::ForEachOptions::ExcludeNonAllocatable;

  for (const ezRTTI* pBase : {ezGetStaticRTTI<ezAiSensor>(), ezGetStaticRTTI<ezAiPerceptionGenerator>(), ezGetStaticRTTI<ezAiBehavior>(), ezGetStaticRTTI<ezAiConsideration>()})
  {
    ezRTTI::ForEachDerivedType(pBase, [&](const ezRTTI* pRtti)
      { out_types.PushBack(pRtti); }, options);
  }
}

//////////////////////////////////////////////////////////////////////////

ezAiControllerAssetDocument::ezAiControllerAssetDocument(ezStringView sDocumentPath)
  : ezAssetDocument(sDocumentPath, EZ_DEFAULT_NEW(ezAiControllerNodeManager), ezAssetDocEngineConnection::None)
{
  m_pObjectAccessor = EZ_DEFAULT_NEW(ezVisualGraphCommandAccessor, GetCommandHistory());
}

ezTransformStatus ezAiControllerAssetDocument::InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  const auto* pManager = static_cast<const ezAiControllerNodeManager*>(GetObjectManager());

  ezAiControllerDesc desc;

  // the index of every node within its array in the desc (sensors, generators, behaviors, considerations), these are what input pins store
  ezHashTable<const ezDocumentObject*, ezUInt32> nodeIndices;
  {
    ezUInt32 uiCounts[5] = {};
    for (const ezDocumentObject* pNode : pManager->GetRootObject()->GetChildren())
    {
      if (pManager->IsNode(pNode))
      {
        nodeIndices[pNode] = uiCounts[(int)ezAiControllerNodeManager::GetNodeKind(pNode->GetType())]++;
      }
    }
  }

  for (const ezDocumentObject* pNode : pManager->GetRootObject()->GetChildren())
  {
    if (!pManager->IsNode(pNode))
      continue;

    const ezRTTI* pType = pNode->GetType();
    ezInternal::NewInstance<ezReflectedClass> pObject = pType->GetAllocator()->Allocate<ezReflectedClass>();
    ezToolsSerializationUtils::CopyProperties(pNode, pManager, pObject, pType, [](const ezAbstractProperty* pProp)
      { return pProp->GetAttributeByType<ezHiddenAttribute>() == nullptr; });

    // fill the input pins from the graph connections
    for (const auto& pPin : pManager->GetInputPins(pNode))
    {
      const ezAbstractProperty* pProp = pType->FindPropertyByName(pPin->GetName());
      if (pProp == nullptr || pProp->GetCategory() != ezPropertyCategory::Member || !pProp->GetSpecificType()->IsDerivedFrom<ezAiInputPin>())
        continue;

      ezAiInputPin* pInputPin = static_cast<ezAiInputPin*>(static_cast<const ezAbstractMemberProperty*>(pProp)->GetPropertyPointer(pObject.m_pInstance));

      for (const ezVisualGraphConnection* pConnection : pManager->GetConnections(*pPin))
      {
        pInputPin->m_Connections.PushBack(nodeIndices[pConnection->GetSourcePin().GetParent()]);
      }
    }

    switch (ezAiControllerNodeManager::GetNodeKind(pType))
    {
      case ezAiControllerNodeManager::NodeKind::Sensor:
        desc.AddSensor(ezUniquePtr<ezAiSensor>(pObject.Cast<ezAiSensor>()));
        break;

      case ezAiControllerNodeManager::NodeKind::PerceptionGenerator:
        desc.AddPerceptionGenerator(ezUniquePtr<ezAiPerceptionGenerator>(pObject.Cast<ezAiPerceptionGenerator>()));
        break;

      case ezAiControllerNodeManager::NodeKind::Behavior:
        desc.AddBehavior(ezUniquePtr<ezAiBehavior>(pObject.Cast<ezAiBehavior>()));
        break;

      case ezAiControllerNodeManager::NodeKind::Consideration:
        desc.AddConsideration(ezUniquePtr<ezAiConsideration>(pObject.Cast<ezAiConsideration>()));
        break;

      default:
        EZ_ASSERT_NOT_IMPLEMENTED;
    }
  }

  EZ_SUCCEED_OR_RETURN(desc.Serialize(stream));

  // reports invalid pin connections etc. in the log
  desc.Finalize();

  return ezTransformStatus(EZ_SUCCESS);
}

void ezAiControllerAssetDocument::GetSupportedMimeTypesForPasting(ezDynamicArray<ezString>& out_mimeTypes) const
{
  out_mimeTypes.PushBack("application/ezEditor.AiControllerGraph");
}

bool ezAiControllerAssetDocument::CopySelectedObjects(ezAbstractObjectGraph& out_objectGraph, ezStringBuilder& out_MimeType) const
{
  out_MimeType = "application/ezEditor.AiControllerGraph";

  const auto* pManager = static_cast<const ezVisualGraphObjectManager*>(GetObjectManager());
  return pManager->CopySelectedObjects(out_objectGraph);
}

bool ezAiControllerAssetDocument::Paste(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, ezStringView sMimeType)
{
  auto* pManager = static_cast<ezVisualGraphObjectManager*>(GetObjectManager());
  return pManager->PasteObjects(info, objectGraph, ezQtVisualGraphScene::GetLastMouseInteractionPos(), bAllowPickedPosition);
}

void ezAiControllerAssetDocument::InternalGetMetaDataHash(const ezDocumentObject* pObject, ezUInt64& inout_uiHash) const
{
  const auto* pManager = static_cast<const ezVisualGraphObjectManager*>(GetObjectManager());
  pManager->GetMetaDataHash(pObject, inout_uiHash);
}

void ezAiControllerAssetDocument::AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const
{
  SUPER::AttachMetaDataBeforeSaving(graph);
  const auto* pManager = static_cast<const ezVisualGraphObjectManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(graph);
}

void ezAiControllerAssetDocument::RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph, bool bUndoable)
{
  SUPER::RestoreMetaDataAfterLoading(graph, bUndoable);
  auto* pManager = static_cast<ezVisualGraphObjectManager*>(GetObjectManager());
  pManager->RestoreMetaDataAfterLoading(graph, bUndoable);
}
