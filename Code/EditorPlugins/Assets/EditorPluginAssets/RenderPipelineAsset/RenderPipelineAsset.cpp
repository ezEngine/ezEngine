#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAsset.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderPipelineAssetDocument, 4, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

bool ezRenderPipelineNodeManager::InternalIsNode(const ezDocumentObject* pObject) const
{
  auto pType = pObject->GetTypeAccessor().GetType();
  return pType->IsDerivedFrom<ezRenderPipelinePass>() || pType->IsDerivedFrom<ezExtractor>();
}

void ezRenderPipelineNodeManager::InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& node)
{
  auto pType = pObject->GetTypeAccessor().GetType();
  if (!pType->IsDerivedFrom<ezRenderPipelinePass>())
    return;

  ezHybridArray<ezAbstractProperty*, 32> properties;
  pType->GetAllProperties(properties);

  for (ezAbstractProperty* pProp : properties)
  {
    if (pProp->GetCategory() != ezPropertyCategory::Member)
      continue;

    if (!pProp->GetSpecificType()->IsDerivedFrom<ezRenderPipelineNodePin>())
      continue;

    ezColor pinColor;
    if (const ezColorAttribute* pAttr = pProp->GetAttributeByType<ezColorAttribute>())
    {
      pinColor = pAttr->GetColor();
    }
    else
    {
      ezColorScheme::Enum color = ezColorScheme::Gray;
      if (ezStringUtils::IsEqual(pProp->GetPropertyName(), "DepthStencil"))
        color = ezColorScheme::Pink;

      pinColor = ezColorScheme::DarkUI(color);
    }

    if (pProp->GetSpecificType()->IsDerivedFrom<ezRenderPipelineNodeInputPin>())
    {
      auto pPin = EZ_DEFAULT_NEW(ezPin, ezPin::Type::Input, pProp->GetPropertyName(), pinColor, pObject);
      node.m_Inputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<ezRenderPipelineNodeOutputPin>())
    {
      auto pPin = EZ_DEFAULT_NEW(ezPin, ezPin::Type::Output, pProp->GetPropertyName(), pinColor, pObject);
      node.m_Outputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<ezRenderPipelineNodePassThrougPin>())
    {
      auto pPinIn = EZ_DEFAULT_NEW(ezPin, ezPin::Type::Input, pProp->GetPropertyName(), pinColor, pObject);
      node.m_Inputs.PushBack(pPinIn);
      auto pPinOut = EZ_DEFAULT_NEW(ezPin, ezPin::Type::Output, pProp->GetPropertyName(), pinColor, pObject);
      node.m_Outputs.PushBack(pPinOut);
    }
  }
}

void ezRenderPipelineNodeManager::GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& Types) const
{
  ezSet<const ezRTTI*> typeSet;
  ezReflectionUtils::GatherTypesDerivedFromClass(ezGetStaticRTTI<ezRenderPipelinePass>(), typeSet, false);
  ezReflectionUtils::GatherTypesDerivedFromClass(ezGetStaticRTTI<ezExtractor>(), typeSet, false);
  Types.Clear();
  for (auto pType : typeSet)
  {
    if (pType->GetTypeFlags().IsAnySet(ezTypeFlags::Abstract))
      continue;

    Types.PushBack(pType);
  }
}

ezStatus ezRenderPipelineNodeManager::InternalCanConnect(const ezPin& source, const ezPin& target, CanConnectResult& out_Result) const
{
  out_Result = CanConnectResult::ConnectNto1;
  return ezStatus(EZ_SUCCESS);
}

ezRenderPipelineAssetDocument::ezRenderPipelineAssetDocument(const char* szDocumentPath)
  : ezAssetDocument(szDocumentPath, EZ_DEFAULT_NEW(ezRenderPipelineNodeManager), ezAssetDocEngineConnection::None)
{
}

ezStatus ezRenderPipelineAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  ezDocumentNodeManager* pManager = static_cast<ezDocumentNodeManager*>(GetObjectManager());

  const ezUInt8 uiVersion = 1;
  stream << uiVersion;

  ezAbstractObjectGraph graph;
  ezDocumentObjectConverterWriter objectConverter(&graph, GetObjectManager());

  auto& children = GetObjectManager()->GetRootObject()->GetChildren();
  for (ezDocumentObject* pObject : children)
  {
    auto pType = pObject->GetTypeAccessor().GetType();
    if (pType->IsDerivedFrom<ezRenderPipelinePass>())
    {
      objectConverter.AddObjectToGraph(pObject, "Pass");
    }
    else if (pType->IsDerivedFrom<ezExtractor>())
    {
      objectConverter.AddObjectToGraph(pObject, "Extractor");
    }
    else if (pManager->IsConnection(pObject))
    {
      objectConverter.AddObjectToGraph(pObject, "Connection");
    }
  }

  pManager->AttachMetaDataBeforeSaving(graph);

  ezDefaultMemoryStreamStorage storage;
  ezMemoryStreamWriter writer(&storage);
  ezAbstractGraphBinarySerializer::Write(writer, &graph);

  ezUInt32 uiSize = storage.GetStorageSize32();
  stream << uiSize;
  return storage.CopyToStream(stream);
}

void ezRenderPipelineAssetDocument::InternalGetMetaDataHash(const ezDocumentObject* pObject, ezUInt64& inout_uiHash) const
{
  const ezDocumentNodeManager* pManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());
  pManager->GetMetaDataHash(pObject, inout_uiHash);
}

void ezRenderPipelineAssetDocument::AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const
{
  SUPER::AttachMetaDataBeforeSaving(graph);
  const ezDocumentNodeManager* pManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(graph);
}

void ezRenderPipelineAssetDocument::RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph, bool bUndoable)
{
  SUPER::RestoreMetaDataAfterLoading(graph, bUndoable);
  ezDocumentNodeManager* pManager = static_cast<ezDocumentNodeManager*>(GetObjectManager());
  pManager->RestoreMetaDataAfterLoading(graph, bUndoable);
}



void ezRenderPipelineAssetDocument::GetSupportedMimeTypesForPasting(ezHybridArray<ezString, 4>& out_MimeTypes) const
{
  out_MimeTypes.PushBack("application/ezEditor.RenderPipelineGraph");
}

bool ezRenderPipelineAssetDocument::CopySelectedObjects(ezAbstractObjectGraph& out_objectGraph, ezStringBuilder& out_MimeType) const
{
  out_MimeType = "application/ezEditor.RenderPipelineGraph";

  const ezDocumentNodeManager* pManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());
  return pManager->CopySelectedObjects(out_objectGraph);
}

bool ezRenderPipelineAssetDocument::Paste(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, const char* szMimeType)
{
  ezDocumentNodeManager* pManager = static_cast<ezDocumentNodeManager*>(GetObjectManager());
  return pManager->PasteObjects(info, objectGraph, ezQtNodeScene::GetLastMouseInteractionPos(), bAllowPickedPosition);
}
