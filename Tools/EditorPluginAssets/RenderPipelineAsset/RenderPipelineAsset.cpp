#include <PCH.h>
#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAsset.h>
#include <EditorPluginAssets/RenderPipelineAsset/RenderPipelineAssetManager.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <ToolsFoundation/Command/NodeCommands.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRenderPipelineAssetDocument, 3, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

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

    if (!pProp->GetSpecificType()->IsDerivedFrom<ezNodePin>())
      continue;

    ezColor pinColor = ezColor::Grey;
    if (const ezColorAttribute* pAttr = pProp->GetAttributeByType<ezColorAttribute>())
    {
      pinColor = pAttr->GetColor();
    }

    if (pProp->GetSpecificType()->IsDerivedFrom<ezInputNodePin>())
    {
      ezPin* pPin = EZ_DEFAULT_NEW(ezPin, ezPin::Type::Input, pProp->GetPropertyName(), pinColor, pObject);
      node.m_Inputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<ezOutputNodePin>())
    {
      ezPin* pPin = EZ_DEFAULT_NEW(ezPin, ezPin::Type::Output, pProp->GetPropertyName(), pinColor, pObject);
      node.m_Outputs.PushBack(pPin);
    }
    else if (pProp->GetSpecificType()->IsDerivedFrom<ezPassThroughNodePin>())
    {
      ezPin* pPinIn = EZ_DEFAULT_NEW(ezPin, ezPin::Type::Input, pProp->GetPropertyName(), pinColor, pObject);
      node.m_Inputs.PushBack(pPinIn);
      ezPin* pPinOut = EZ_DEFAULT_NEW(ezPin, ezPin::Type::Output, pProp->GetPropertyName(), pinColor, pObject);
      node.m_Outputs.PushBack(pPinOut);
    }
  }
}

void ezRenderPipelineNodeManager::InternalDestroyPins(const ezDocumentObject* pObject, NodeInternal& node)
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

ezStatus ezRenderPipelineNodeManager::InternalCanConnect(const ezPin* pSource, const ezPin* pTarget, CanConnectResult& out_Result) const
{
  out_Result = CanConnectResult::ConnectNto1;

  if (!pTarget->GetConnections().IsEmpty())
    return ezStatus("Only one connection can be made to in input pin!");

  return ezStatus(EZ_SUCCESS);
}

ezRenderPipelineAssetDocument::ezRenderPipelineAssetDocument(const char* szDocumentPath)
  : ezAssetDocument(szDocumentPath, EZ_DEFAULT_NEW(ezRenderPipelineNodeManager), false, false)
{
}

ezStatus ezRenderPipelineAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually)
{
  const ezUInt8 uiVersion = 1;
  stream << uiVersion;

  ezAbstractObjectGraph graph;
  ezRttiConverterContext context;
  ezRttiConverterWriter rttiConverter(&graph, &context, true, true);
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
  }

  ezDocumentNodeManager* pManager = static_cast<ezDocumentNodeManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(graph);

  ezMemoryStreamStorage storage;
  ezMemoryStreamWriter writer(&storage);
  ezAbstractGraphBinarySerializer::Write(writer, &graph);

  ezUInt32 uiSize = storage.GetStorageSize();
  stream << uiSize;
  stream.WriteBytes(storage.GetData(), uiSize);
  return ezStatus(EZ_SUCCESS);
}

void ezRenderPipelineAssetDocument::InternalGetMetaDataHash(const ezDocumentObject* pObject, ezUInt64& inout_uiHash) const
{
  const ezDocumentNodeManager* pManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());
  if (pManager->IsNode(pObject))
  {
    auto outputs = pManager->GetOutputPins(pObject);
    for (const ezPin* pPinSource : outputs)
    {
      auto inputs = pPinSource->GetConnections();
      for (const ezConnection* pConnection : inputs)
      {
        const ezPin* pPinTarget = pConnection->GetTargetPin();

        inout_uiHash = ezHashing::xxHash64(&pPinSource->GetParent()->GetGuid(), sizeof(ezUuid), inout_uiHash);
        inout_uiHash = ezHashing::xxHash64(&pPinTarget->GetParent()->GetGuid(), sizeof(ezUuid), inout_uiHash);
        inout_uiHash = ezHashing::xxHash64(pPinSource->GetName(), ezStringUtils::GetStringElementCount(pPinSource->GetName()), inout_uiHash);
        inout_uiHash = ezHashing::xxHash64(pPinTarget->GetName(), ezStringUtils::GetStringElementCount(pPinTarget->GetName()), inout_uiHash);
      }
    }
  }
}

void ezRenderPipelineAssetDocument::AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const
{
  const ezDocumentNodeManager* pManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(graph);

}

void ezRenderPipelineAssetDocument::RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph, bool bUndoable)
{
  ezDocumentNodeManager* pManager = static_cast<ezDocumentNodeManager*>(GetObjectManager());
  pManager->RestoreMetaDataAfterLoading(graph, bUndoable);
}



void ezRenderPipelineAssetDocument::GetSupportedMimeTypesForPasting(ezHybridArray<ezString, 4>& out_MimeTypes) const
{
  out_MimeTypes.PushBack("application/ezEditor.RenderPipelineGraph");
}

bool ezRenderPipelineAssetDocument::Copy(ezAbstractObjectGraph& out_objectGraph, ezStringBuilder& out_MimeType) const
{
  out_MimeType = "application/ezEditor.RenderPipelineGraph";

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

bool ezRenderPipelineAssetDocument::Paste(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, const char* szMimeType)
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
