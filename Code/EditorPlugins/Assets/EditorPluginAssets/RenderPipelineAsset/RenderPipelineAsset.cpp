#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAsset.h>

#include <ToolsFoundation/VisualGraph/VisualGraphCommandAccessor.h>

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GuiFoundation/VisualGraph/Scene.moc.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/Implementation/RenderPipelinePassGraph.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezRenderPipelineResourceType, 1)
  EZ_ENUM_CONSTANTS(ezRenderPipelineResourceType::Texture, ezRenderPipelineResourceType::Buffer)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezRenderPipelineAssetPinInfo, ezNoBase, 2, ezRTTIDefaultAllocator<ezRenderPipelineAssetPinInfo>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ENUM_MEMBER_PROPERTY("ResourceType", ezRenderPipelineResourceType, m_ResourceType),
    EZ_MEMBER_PROPERTY("Name", m_sName),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderPipelineAssetMetaData, 1, ezRTTIDefaultAllocator<ezRenderPipelineAssetMetaData>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_MEMBER_PROPERTY("Inputs", m_Inputs),
    EZ_ARRAY_MEMBER_PROPERTY("Outputs", m_Outputs),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderPipelineNodeGraphPin, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderPipelineAssetDocument, 6, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  ezUInt64 ComputeRenderPipelineMetaDataHash(const ezRenderPipelineAssetMetaData* pMetaData)
  {
    auto HashPin = [](const ezRenderPipelineAssetPinInfo& pin, ezUInt64& ref_uiHash)
    {
      const ezUInt8 uiResourceType = pin.m_ResourceType.GetValue();
      ref_uiHash = ezHashingUtils::xxHash64(&uiResourceType, sizeof(uiResourceType), ref_uiHash);
      ref_uiHash = ezHashingUtils::xxHash64String(pin.m_sName, ref_uiHash);
    };

    if (pMetaData == nullptr)
      return 0;

    ezUInt64 uiHash = 0;
    const ezUInt32 uiInputCount = pMetaData->m_Inputs.GetCount();
    uiHash = ezHashingUtils::xxHash64(&uiInputCount, sizeof(uiInputCount), uiHash);
    for (const ezRenderPipelineAssetPinInfo& pin : pMetaData->m_Inputs)
    {
      HashPin(pin, uiHash);
    }

    const ezUInt32 uiOutputCount = pMetaData->m_Outputs.GetCount();
    uiHash = ezHashingUtils::xxHash64(&uiOutputCount, sizeof(uiOutputCount), uiHash);
    for (const ezRenderPipelineAssetPinInfo& pin : pMetaData->m_Outputs)
    {
      HashPin(pin, uiHash);
    }

    return uiHash;
  }

  ezColor GetPinColor(bool bIsBuffer, ezStringView sName)
  {
    if (bIsBuffer)
      return ezColorScheme::DarkUI(ezColorScheme::Teal);

    if (sName == "DepthStencil")
      return ezColorScheme::DarkUI(ezColorScheme::Pink);

    return ezColorScheme::DarkUI(ezColorScheme::Blue);
  }
} // namespace

ezRenderPipelineNodeGraphPin::ezRenderPipelineNodeGraphPin(ezVisualGraphPin::Type type, const char* szName, const ezColorGammaUB& color, const ezDocumentObject* pObject, ezRenderPipelineResourceType::Enum resourceType)
  : ezVisualGraphPin(type, szName, color, pObject)
  , m_ResourceType(resourceType)
{
}

ezRenderPipelineNodeGraphPin::~ezRenderPipelineNodeGraphPin() = default;

//////////////////////////////////////////////////////////////////////////

ezRenderPipelineNodeManager::ezRenderPipelineNodeManager()
{
  ezAssetCurator::GetSingleton()->m_Events.AddEventHandler(ezMakeDelegate(&ezRenderPipelineNodeManager::AssetCuratorEventHandler, this));
  m_NodeEvents.AddEventHandler(ezMakeDelegate(&ezRenderPipelineNodeManager::NodeEventHandler, this));
}

ezRenderPipelineNodeManager::~ezRenderPipelineNodeManager()
{
  m_NodeEvents.RemoveEventHandler(ezMakeDelegate(&ezRenderPipelineNodeManager::NodeEventHandler, this));
  ezAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezRenderPipelineNodeManager::AssetCuratorEventHandler, this));
}

void ezRenderPipelineNodeManager::AssetCuratorEventHandler(const ezAssetCuratorEvent& e)
{
  if (e.m_Type != ezAssetCuratorEvent::Type::AssetUpdated || e.m_pInfo == nullptr || e.m_pInfo->m_pAssetInfo->GetManager() != GetDocument()->GetDocumentManager())
    return;

  const ezRenderPipelineAssetMetaData* pMetaData = e.m_pInfo->m_pAssetInfo->m_Info->GetMetaInfo<ezRenderPipelineAssetMetaData>();
  const ezUInt64 uiMetaDataHash = ComputeRenderPipelineMetaDataHash(pMetaData);

  ezHybridArray<const ezDocumentObject*, 4> subGraphs;
  // Find sub-graphs that match the changed asset and have a different meta-data hash.
  for (auto it = m_SubGraphs.GetIterator(); it.IsValid(); ++it)
  {
    const SubGraphCache& cache = it.Value();
    if (cache.m_SourceAssetGuid == e.m_AssetGuid && cache.m_uiMetaDataHash != uiMetaDataHash)
      subGraphs.PushBack(cache.m_pObject);
  }

  if (!subGraphs.IsEmpty())
  {
    // As we have to modify the document, we need to create a transaction. Undoing this one might fail though but still better than clearing the undo stack.
    auto pAccessor = static_cast<ezVisualGraphCommandAccessor*>(GetDocument()->GetObjectAccessor());
    pAccessor->StartTransaction("Update Sub-Graph");
    for (const ezDocumentObject* pObject : subGraphs)
    {
      ezTempHybridArray<ezVisualGraphCommandAccessor::ConnectionInfo, 16> oldConnections;
      ezStatus res = pAccessor->DisconnectAllPins(pObject, oldConnections);
      if (res.Failed())
      {
        ezLog::Warning("Failed to DisconnectAllPins while a pipeline sub-graph was changed: {}", res.GetMessageString());
      }

      TryRecreatePins(pObject);

      res = pAccessor->TryReconnectAllPins(pObject, oldConnections);
      if (res.Failed())
      {
        ezLog::Warning("Failed to TryReconnectAllPins while a pipeline sub-graph was changed: {}", res.GetMessageString());
      }
    }
    pAccessor->FinishTransaction();
  }
}

void ezRenderPipelineNodeManager::NodeEventHandler(const ezVisualGraphObjectManagerEvent& e)
{
  if (e.m_EventType == ezVisualGraphObjectManagerEvent::Type::BeforeNodeRemoved)
  {
    m_SubGraphs.Remove(e.m_pObject->GetGuid());
  }
}

bool ezRenderPipelineNodeManager::InternalIsNode(const ezDocumentObject* pObject) const
{
  auto pType = pObject->GetTypeAccessor().GetType();
  return pType->IsDerivedFrom<ezRenderPipelineNode>() || pType->IsDerivedFrom<ezExtractor>();
}

void ezRenderPipelineNodeManager::InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& ref_node)
{
  auto pType = pObject->GetTypeAccessor().GetType();
  if (!pType->IsDerivedFrom<ezRenderPipelineNode>())
    return;

  // SubGraph nodes get their pins from the referenced asset's meta data.
  if (pType == ezGetStaticRTTI<ezSubGraphNode>())
  {
    SubGraphCache& cache = m_SubGraphs[pObject->GetGuid()];
    cache.m_pObject = pObject;
    cache.m_SourceAssetGuid = ezUuid();
    cache.m_uiMetaDataHash = 0;

    const ezString sPipeline = pObject->GetTypeAccessor().GetValue("Pipeline").ConvertTo<ezString>();

    auto pSubAsset = ezAssetCurator::GetSingleton()->FindSubAsset(sPipeline);
    if (!pSubAsset.isValid())
      return;

    const ezRenderPipelineAssetMetaData* pMeta = pSubAsset->m_pAssetInfo->m_Info->GetMetaInfo<ezRenderPipelineAssetMetaData>();

    cache.m_SourceAssetGuid = pSubAsset->m_Data.m_Guid;
    cache.m_uiMetaDataHash = ComputeRenderPipelineMetaDataHash(pMeta);

    if (pMeta == nullptr)
      return;

    for (const ezRenderPipelineAssetPinInfo& pinInfo : pMeta->m_Inputs)
    {
      const ezColor pinColor = GetPinColor(pinInfo.m_ResourceType == ezRenderPipelineResourceType::Buffer, pinInfo.m_sName);

      auto pPin = EZ_DEFAULT_NEW(ezRenderPipelineNodeGraphPin, ezVisualGraphPin::Type::Input, pinInfo.m_sName, pinColor, pObject, pinInfo.m_ResourceType);
      ref_node.m_Inputs.PushBack(std::move(pPin));
    }

    for (const ezRenderPipelineAssetPinInfo& pinInfo : pMeta->m_Outputs)
    {
      const ezColor pinColor = GetPinColor(pinInfo.m_ResourceType == ezRenderPipelineResourceType::Buffer, pinInfo.m_sName);

      auto pPin = EZ_DEFAULT_NEW(ezRenderPipelineNodeGraphPin, ezVisualGraphPin::Type::Output, pinInfo.m_sName, pinColor, pObject, pinInfo.m_ResourceType);
      ref_node.m_Outputs.PushBack(std::move(pPin));
    }

    return;
  }

  if (pType->IsDerivedFrom<ezSwitchBasePass>())
  {
    ezDynamicArray<ezString> inputNames;
    GetDynamicPinNames(pObject, "Values", "", inputNames);

    const bool bBuffer = pType->IsDerivedFrom<ezBufferSwitchPass>();
    const ezRenderPipelineResourceType::Enum resourceType = bBuffer ? ezRenderPipelineResourceType::Buffer : ezRenderPipelineResourceType::Texture;

    for (const ezString& sInputName : inputNames)
    {
      const ezColor pinColor = GetPinColor(bBuffer, sInputName);

      auto pPin = EZ_DEFAULT_NEW(ezRenderPipelineNodeGraphPin, ezVisualGraphPin::Type::Input, sInputName, pinColor, pObject, resourceType);
      ref_node.m_Inputs.PushBack(std::move(pPin));
    }
  }

  ezTempHybridArray<const ezAbstractProperty*, 32> properties;
  pType->GetAllProperties(properties);

  for (auto pProp : properties)
  {
    if (pProp->GetCategory() != ezPropertyCategory::Member)
      continue;

    if (pProp->GetAttributeByType<ezHiddenAttribute>() != nullptr)
      continue;

    if (!pProp->GetSpecificType()->IsDerivedFrom<ezRenderPipelineNodePin>())
      continue;

    const ezRTTI* pPinType = pProp->GetSpecificType();
    const bool bBuffer = pPinType->IsDerivedFrom<ezRenderPipelineNodeBufferInputPin>() ||
                         pPinType->IsDerivedFrom<ezRenderPipelineNodeBufferOutputPin>() ||
                         pPinType->IsDerivedFrom<ezRenderPipelineNodeBufferPassThroughPin>();
    const ezRenderPipelineResourceType::Enum resourceType = bBuffer ? ezRenderPipelineResourceType::Buffer : ezRenderPipelineResourceType::Texture;

    ezColor pinColor;
    if (const ezColorAttribute* pAttr = pProp->GetAttributeByType<ezColorAttribute>())
    {
      pinColor = pAttr->GetColor();
    }
    else
    {
      pinColor = GetPinColor(bBuffer, pProp->GetPropertyName());
    }

    if (pPinType->IsDerivedFrom<ezRenderPipelineNodeInputPin>() || pPinType->IsDerivedFrom<ezRenderPipelineNodeBufferInputPin>())
    {
      auto pPin = EZ_DEFAULT_NEW(ezRenderPipelineNodeGraphPin, ezVisualGraphPin::Type::Input, pProp->GetPropertyName(), pinColor, pObject, resourceType);
      ref_node.m_Inputs.PushBack(std::move(pPin));
    }
    else if (pPinType->IsDerivedFrom<ezRenderPipelineNodeOutputPin>() || pPinType->IsDerivedFrom<ezRenderPipelineNodeBufferOutputPin>())
    {
      auto pPin = EZ_DEFAULT_NEW(ezRenderPipelineNodeGraphPin, ezVisualGraphPin::Type::Output, pProp->GetPropertyName(), pinColor, pObject, resourceType);
      ref_node.m_Outputs.PushBack(std::move(pPin));
    }
    else if (pPinType->IsDerivedFrom<ezRenderPipelineNodePassThroughPin>() || pPinType->IsDerivedFrom<ezRenderPipelineNodeBufferPassThroughPin>())
    {
      auto pInputPin = EZ_DEFAULT_NEW(ezRenderPipelineNodeGraphPin, ezVisualGraphPin::Type::Input, pProp->GetPropertyName(), pinColor, pObject, resourceType);
      ref_node.m_Inputs.PushBack(std::move(pInputPin));

      auto pOutputPin = EZ_DEFAULT_NEW(ezRenderPipelineNodeGraphPin, ezVisualGraphPin::Type::Output, pProp->GetPropertyName(), pinColor, pObject, resourceType);
      ref_node.m_Outputs.PushBack(std::move(pOutputPin));
    }
  }
}

void ezRenderPipelineNodeManager::GetCreateableTypes(ezDynamicArray<const ezRTTI*>& out_types) const
{
  ezRTTI::ForEachDerivedType<ezRenderPipelineNode>(
    [&](const ezRTTI* pRtti)
    { out_types.PushBack(pRtti); },
    ezRTTI::ForEachOptions::ExcludeAbstract);

  ezRTTI::ForEachDerivedType<ezExtractor>(
    [&](const ezRTTI* pRtti)
    { out_types.PushBack(pRtti); },
    ezRTTI::ForEachOptions::ExcludeAbstract);
}

ezStatus ezRenderPipelineNodeManager::InternalCanConnect(const ezVisualGraphPin& source, const ezVisualGraphPin& target, CanConnectResult& out_result) const
{
  const ezRenderPipelineNodeGraphPin& sourcePin = ezStaticCast<const ezRenderPipelineNodeGraphPin&>(source);
  const ezRenderPipelineNodeGraphPin& targetPin = ezStaticCast<const ezRenderPipelineNodeGraphPin&>(target);

  out_result = CanConnectResult::ConnectNever;

  if (sourcePin.m_ResourceType != targetPin.m_ResourceType)
    return ezStatus("Can't connect texture and buffer pins");

  if (WouldConnectionCreateCircle(source, target))
    return ezStatus("Connecting these pins would create a circle in the graph.");

  out_result = CanConnectResult::ConnectNto1;
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezRenderPipelineNodeManager::InternalCanAdd(const ezRTTI* pRtti, const ezDocumentObject* pParent, ezStringView sParentProperty, const ezVariant& index) const
{
  if (pRtti->IsDerivedFrom<ezExtractor>())
  {
    for (const ezDocumentObject* pObject : GetRootObject()->GetChildren())
    {
      if (pObject->GetType() == pRtti)
        return ezStatus(ezFmt("The pipeline may only contain one extractor of type '{}'.", pRtti->GetTypeName()));
    }
  }

  return ezStatus(EZ_SUCCESS);
}

bool ezRenderPipelineNodeManager::InternalIsDynamicPinProperty(const ezDocumentObject* pObject, const ezAbstractProperty* pProp) const
{
  if (pObject->GetTypeAccessor().GetType()->IsDerivedFrom<ezSwitchBasePass>())
  {
    return ezStringUtils::IsEqual(pProp->GetPropertyName(), "Values");
  }

  if (pObject->GetTypeAccessor().GetType() == ezGetStaticRTTI<ezSubGraphNode>())
  {
    return ezStringUtils::IsEqual(pProp->GetPropertyName(), "Pipeline");
  }

  return false;
}

//////////////////////////////////////////////////////////////////////////

ezRenderPipelineAssetDocument::ezRenderPipelineAssetDocument(ezStringView sDocumentPath)
  : ezAssetDocument(sDocumentPath, EZ_DEFAULT_NEW(ezRenderPipelineNodeManager), ezAssetDocEngineConnection::FullObjectMirroring)
{
  m_pObjectAccessor = EZ_DEFAULT_NEW(ezVisualGraphCommandAccessor, GetCommandHistory());
}

ezRenderPipelineAssetDocument::~ezRenderPipelineAssetDocument() = default;

ezStatus ezRenderPipelineAssetDocument::Validate() const
{
  const ezRTTI* pInputTypes[] = {ezGetStaticRTTI<ezSubGraphTextureInputNode>(), ezGetStaticRTTI<ezSubGraphBufferInputNode>()};
  const ezRTTI* pOutputTypes[] = {ezGetStaticRTTI<ezSubGraphTextureOutputNode>(), ezGetStaticRTTI<ezSubGraphBufferOutputNode>()};
  const ezRenderPipelineNodeManager* pManager = static_cast<const ezRenderPipelineNodeManager*>(GetObjectManager());

  ezSet<ezString> inputNames, outputNames;

  for (const ezDocumentObject* pObject : GetObjectManager()->GetRootObject()->GetChildren())
  {
    const ezRTTI* pType = pObject->GetTypeAccessor().GetType();

    if (pType->IsDerivedFrom<ezSwitchBasePass>())
    {
      ezSet<ezInt32> uniqueValues;
      const ezVariantArray& values = pObject->GetTypeAccessor().GetValue("Values").Get<ezVariantArray>();
      for (const ezVariant& value : values)
      {
        const ezInt32 iValue = value.ConvertTo<ezInt32>();
        if (uniqueValues.Contains(iValue))
          return ezStatus(ezFmt("Switch '{}' contains duplicate value '{}'.", pObject->GetTypeAccessor().GetValue("Name"), iValue));

        uniqueValues.Insert(iValue);
      }
    }

    bool bIsInput = (pType == pInputTypes[0] || pType == pInputTypes[1]);
    bool bIsOutput = (pType == pOutputTypes[0] || pType == pOutputTypes[1]);

    if (!bIsInput && !bIsOutput)
      continue;

    const ezString sName = pObject->GetTypeAccessor().GetValue("Name").ConvertTo<ezString>();

    if (sName.IsEmpty())
    {
      return ezStatus(ezFmt("{} node '{}' has an empty name.", bIsInput ? "Input" : "Output", pType->GetTypeName()));
    }

    ezSet<ezString>& names = bIsInput ? inputNames : outputNames;
    if (names.Contains(sName))
    {
      return ezStatus(ezFmt("{} node '{}' has a duplicate name '{}'.", bIsInput ? "Input" : "Output", pType->GetTypeName(), sName));
    }
    names.Insert(sName);
  }

  for (const ezDocumentObject* pObject : GetObjectManager()->GetRootObject()->GetChildren())
  {
    const bool isNode = pManager->IsNode(pObject);
    if (!isNode)
      continue;

    for (const ezUniquePtr<const ezVisualGraphPin>& pOutputPin : pManager->GetOutputPins(pObject))
    {
      ezUInt32 uiPassThroughConnections = 0;
      for (const ezVisualGraphConnection* pConnection : pManager->GetConnections(*pOutputPin))
      {
        const ezVisualGraphPin& targetPin = pConnection->GetTargetPin();
        const ezRTTI* pTargetType = targetPin.GetParent()->GetType();

        bool bIsPassThrough = pTargetType->IsDerivedFrom<ezSwitchBasePass>();
        if (const ezAbstractProperty* pProperty = pTargetType->FindPropertyByName(targetPin.GetName()))
        {
          const ezRTTI* pPinType = pProperty->GetSpecificType();
          bIsPassThrough |= pPinType->IsDerivedFrom<ezRenderPipelineNodePassThroughPin>() || pPinType->IsDerivedFrom<ezRenderPipelineNodeBufferPassThroughPin>();
        }

        if (bIsPassThrough && ++uiPassThroughConnections > 1)
        {
          return ezStatus(ezFmt("Output pin '{}.{}' is connected to more than one pass-through input.", pObject->GetTypeAccessor().GetValue("Name"), pOutputPin->GetName()));
        }
      }
    }
  }

  return ezStatus(EZ_SUCCESS);
}

ezTransformStatus ezRenderPipelineAssetDocument::InternalTransformAsset(const char* szTargetFile, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile,
  const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  EZ_SUCCEED_OR_RETURN(Validate());

  if (!GetLoadingErrors().IsEmpty())
  {
    ezStringBuilder s("Cannot transform document because it had errors during loading:\n\n");
    for (const ezString& err : GetLoadingErrors())
    {
      s.Append(err, "\n");
    }
    return ezTransformStatus(s.GetView());
  }

  return ezAssetDocument::RemoteExport(AssetHeader, szTargetFile);
}

ezTransformStatus ezRenderPipelineAssetDocument::InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  EZ_REPORT_FAILURE("Should not be called");
  return ezTransformStatus();
}

void ezRenderPipelineAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  ezRenderPipelineAssetMetaData* pMeta = EZ_DEFAULT_NEW(ezRenderPipelineAssetMetaData);

  struct BoundaryNodeType
  {
    const ezRTTI* m_pType;
    bool m_bIsInput;
    ezRenderPipelineResourceType::Enum m_ResourceType;
  };

  const BoundaryNodeType boundaryTypes[] = {
    {ezGetStaticRTTI<ezSubGraphTextureInputNode>(), true, ezRenderPipelineResourceType::Texture},
    {ezGetStaticRTTI<ezSubGraphBufferInputNode>(), true, ezRenderPipelineResourceType::Buffer},
    {ezGetStaticRTTI<ezSubGraphTextureOutputNode>(), false, ezRenderPipelineResourceType::Texture},
    {ezGetStaticRTTI<ezSubGraphBufferOutputNode>(), false, ezRenderPipelineResourceType::Buffer},
  };

  for (const ezDocumentObject* pObject : GetObjectManager()->GetRootObject()->GetChildren())
  {
    const ezRTTI* pType = pObject->GetTypeAccessor().GetType();

    for (const BoundaryNodeType& boundary : boundaryTypes)
    {
      if (boundary.m_pType != pType)
        continue;

      auto& info = (boundary.m_bIsInput ? pMeta->m_Inputs : pMeta->m_Outputs).ExpandAndGetRef();
      info.m_ResourceType = boundary.m_ResourceType;
      info.m_sName = pObject->GetTypeAccessor().GetValue("Name").ConvertTo<ezString>();
      break;
    }
  }

  auto sortByTypeAndName = [](const ezRenderPipelineAssetPinInfo& a, const ezRenderPipelineAssetPinInfo& b)
  {
    if (a.m_ResourceType != b.m_ResourceType)
      return a.m_ResourceType < b.m_ResourceType;

    return a.m_sName.Compare(b.m_sName) < 0;
  };
  pMeta->m_Inputs.Sort(sortByTypeAndName);
  pMeta->m_Outputs.Sort(sortByTypeAndName);

  // pInfo takes ownership
  pInfo->m_MetaInfo.PushBack(pMeta);
}

void ezRenderPipelineAssetDocument::InternalGetMetaDataHash(const ezDocumentObject* pObject, ezUInt64& inout_uiHash) const
{
  const ezVisualGraphObjectManager* pManager = static_cast<const ezVisualGraphObjectManager*>(GetObjectManager());
  pManager->GetMetaDataHash(pObject, inout_uiHash);
}

void ezRenderPipelineAssetDocument::AttachMetaDataBeforeSaving(ezAbstractObjectGraph& ref_graph) const
{
  SUPER::AttachMetaDataBeforeSaving(ref_graph);
  const ezVisualGraphObjectManager* pManager = static_cast<const ezVisualGraphObjectManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(ref_graph);
}

void ezRenderPipelineAssetDocument::RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& ref_graph, bool bUndoable)
{
  SUPER::RestoreMetaDataAfterLoading(ref_graph, bUndoable);
  ezVisualGraphObjectManager* pManager = static_cast<ezVisualGraphObjectManager*>(GetObjectManager());
  pManager->RestoreMetaDataAfterLoading(ref_graph, bUndoable);
}

void ezRenderPipelineAssetDocument::GetSupportedMimeTypesForPasting(ezDynamicArray<ezString>& out_mimeTypes) const
{
  out_mimeTypes.PushBack("application/ezEditor.RenderPipelineGraph");
}

bool ezRenderPipelineAssetDocument::CopySelectedObjects(ezAbstractObjectGraph& out_objectGraph, ezStringBuilder& out_MimeType) const
{
  out_MimeType = "application/ezEditor.RenderPipelineGraph";

  const ezVisualGraphObjectManager* pManager = static_cast<const ezVisualGraphObjectManager*>(GetObjectManager());
  return pManager->CopySelectedObjects(out_objectGraph);
}

bool ezRenderPipelineAssetDocument::Paste(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, ezStringView sMimeType)
{
  ezVisualGraphObjectManager* pManager = static_cast<ezVisualGraphObjectManager*>(GetObjectManager());
  return pManager->PasteObjects(info, objectGraph, ezQtVisualGraphScene::GetLastMouseInteractionPos(), bAllowPickedPosition);
}
