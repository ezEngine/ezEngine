#include <PCH.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptAsset.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptAssetManager.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Serialization/BinarySerializer.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptAssetDocument, 2, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

bool ezVisualScriptNodeManager::InternalIsNode(const ezDocumentObject* pObject) const
{
  auto pType = pObject->GetTypeAccessor().GetType();
  //return pType->IsDerivedFrom<ezVisualScriptPass>() || pType->IsDerivedFrom<ezExtractor>();
  return false;
}

void ezVisualScriptNodeManager::InternalCreatePins(const ezDocumentObject* pObject, NodeInternal& node)
{
  auto pType = pObject->GetTypeAccessor().GetType();
  //if (!pType->IsDerivedFrom<ezVisualScriptPass>())
    return;

}

void ezVisualScriptNodeManager::InternalDestroyPins(const ezDocumentObject* pObject, NodeInternal& node)
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


void ezVisualScriptNodeManager::GetCreateableTypes(ezHybridArray<const ezRTTI*, 32>& Types) const
{
  ezSet<const ezRTTI*> typeSet;
  //ezReflectionUtils::GatherTypesDerivedFromClass(ezGetStaticRTTI<ezExtractor>(), typeSet, false);

  Types.Clear();

  for (auto pType : typeSet)
  {
    if (pType->GetTypeFlags().IsAnySet(ezTypeFlags::Abstract))
      continue;

    Types.PushBack(pType);
  }
}

ezStatus ezVisualScriptNodeManager::InternalCanConnect(const ezPin* pSource, const ezPin* pTarget) const
{
  //if (!pTarget->GetConnections().IsEmpty())
  //  return ezStatus("Only one connection can be made to in input pin!");

  return ezStatus(EZ_SUCCESS);
}

ezVisualScriptAssetDocument::ezVisualScriptAssetDocument(const char* szDocumentPath)
  : ezAssetDocument(szDocumentPath, EZ_DEFAULT_NEW(ezVisualScriptNodeManager), false, false)
{
}

ezStatus ezVisualScriptAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually)
{
  const ezUInt8 uiVersion = 1;
  stream << uiVersion;

  ezAbstractObjectGraph graph;
  ezRttiConverterContext context;
  ezRttiConverterWriter rttiConverter(&graph, &context, true, true);
  ezDocumentObjectConverterWriter objectConverter(&graph, GetObjectManager(), true, true);

  auto& children = GetObjectManager()->GetRootObject()->GetChildren();
  for (ezDocumentObject* pObject : children)
  {
    auto pType = pObject->GetTypeAccessor().GetType();
    //if (pType->IsDerivedFrom<ezVisualScriptPass>())
    //{
    //  objectConverter.AddObjectToGraph(pObject, "Pass");
    //}
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

void ezVisualScriptAssetDocument::InternalGetMetaDataHash(const ezDocumentObject* pObject, ezUInt64& inout_uiHash) const
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

        inout_uiHash = ezHashing::MurmurHash64(&pPinSource->GetParent()->GetGuid(), sizeof(ezUuid), inout_uiHash);
        inout_uiHash = ezHashing::MurmurHash64(&pPinTarget->GetParent()->GetGuid(), sizeof(ezUuid), inout_uiHash);
        inout_uiHash = ezHashing::MurmurHash64(pPinSource->GetName(), ezStringUtils::GetStringElementCount(pPinSource->GetName()), inout_uiHash);
        inout_uiHash = ezHashing::MurmurHash64(pPinTarget->GetName(), ezStringUtils::GetStringElementCount(pPinTarget->GetName()), inout_uiHash);
      }
    }
  }
}

void ezVisualScriptAssetDocument::AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const
{
  const ezDocumentNodeManager* pManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(graph);

}

void ezVisualScriptAssetDocument::RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph)
{
  ezDocumentNodeManager* pManager = static_cast<ezDocumentNodeManager*>(GetObjectManager());
  pManager->RestoreMetaDataAfterLoading(graph);
}

