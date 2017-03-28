#include <PCH.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptAsset.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptAssetManager.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptTypeRegistry.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptGraph.h>
#include <VisualShader/VisualShaderTypeRegistry.h>
#include <GameEngine/VisualScript/VisualScriptResource.h>
#include <Core/WorldSerializer/ResourceHandleWriter.h>

//////////////////////////////////////////////////////////////////////////
// ezVisualScriptAssetDocument
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptAssetDocument, 2, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptAssetDocument::ezVisualScriptAssetDocument(const char* szDocumentPath)
  : ezAssetDocument(szDocumentPath, EZ_DEFAULT_NEW(ezVisualScriptNodeManager), false, false)
{
}

ezStatus ezVisualScriptAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually)
{
  ezVisualScriptResourceDescriptor desc;
  GenerateVisualScriptDescriptor(desc);

  ezResourceHandleWriteContext context;
  context.BeginWritingToStream(&stream);

  desc.Save(stream);

  context.EndWritingToStream(&stream);

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

void ezVisualScriptAssetDocument::GenerateVisualScriptDescriptor(ezVisualScriptResourceDescriptor& desc)
{
  ezVisualScriptNodeManager* pNodeManager = static_cast<ezVisualScriptNodeManager*>(GetObjectManager());
  ezVisualScriptTypeRegistry* pTypeRegistry = ezVisualScriptTypeRegistry::GetSingleton();
  const ezRTTI* pNodeBaseRtti = pTypeRegistry->GetNodeBaseType();

  ezDynamicArray<const ezDocumentObject*> allNodes;
  allNodes.Reserve(64);

  const auto& children = GetObjectManager()->GetRootObject()->GetChildren();
  for (const ezDocumentObject* pObject : children)
  {
    auto pType = pObject->GetTypeAccessor().GetType();
    if (!pType->IsDerivedFrom(pNodeBaseRtti))
      continue;

    allNodes.PushBack(pObject);
  }

  ezMap<const ezDocumentObject*, ezUInt16> ObjectToIndex;
  desc.m_Nodes.Reserve(allNodes.GetCount());

  for (ezUInt32 i = 0; i < allNodes.GetCount(); ++i)
  {
    const ezDocumentObject* pObject = allNodes[i];
    const ezVisualScriptNodeDescriptor* pDesc = pTypeRegistry->GetDescriptorForType(pObject->GetType());

    auto& node = desc.m_Nodes.ExpandAndGetRef();
    node.m_pType = nullptr;
    node.m_sTypeName = pDesc->m_sTypeName;
    node.m_uiFirstProperty = desc.m_Properties.GetCount();
    node.m_uiNumProperties = 0;

    ObjectToIndex[pObject] = i;

    ezHybridArray<ezAbstractProperty*, 32> properties;
    pObject->GetType()->GetAllProperties(properties);

    for (const ezAbstractProperty* pProp : properties)
    {
      if (pProp->GetCategory() == ezPropertyCategory::Member)
      {
        auto& ref = desc.m_Properties.ExpandAndGetRef();
        ref.m_sName = pProp->GetPropertyName();
        ref.m_Value = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName());

        node.m_uiNumProperties++;
      }
    }
  }

  for (ezUInt32 srcNodeIdx = 0; srcNodeIdx < allNodes.GetCount(); ++srcNodeIdx)
  {
    const ezDocumentObject* pSrcObject = allNodes[srcNodeIdx];

    const ezArrayPtr<ezPin* const> outputPins = pNodeManager->GetOutputPins(pSrcObject);

    for (const ezPin* pPin : outputPins)
    {
      const auto connections = pPin->GetConnections();

      for (const ezConnection* pCon : connections)
      {
        const ezVisualScriptPin* pVsPinSource = static_cast<const ezVisualScriptPin*>(pCon->GetSourcePin());
        const ezVisualScriptPin* pVsPinTarget = static_cast<const ezVisualScriptPin*>(pCon->GetTargetPin());

        if (pVsPinSource->GetDescriptor()->m_PinType == ezVisualScriptPinDescriptor::Execution)
        {
          auto& path = desc.m_ExecutionPaths.ExpandAndGetRef();
          path.m_uiSourceNode = srcNodeIdx;
          path.m_uiOutputPin = pVsPinSource->GetDescriptor()->m_uiPinIndex;
          path.m_uiTargetNode = ObjectToIndex[pVsPinTarget->GetParent()];
          path.m_uiInputPin = pVsPinTarget->GetDescriptor()->m_uiPinIndex;
        }
        else if (pVsPinSource->GetDescriptor()->m_PinType == ezVisualScriptPinDescriptor::Data)
        {
          auto& path = desc.m_DataPaths.ExpandAndGetRef();
          path.m_uiSourceNode = srcNodeIdx;
          path.m_uiOutputPin = pVsPinSource->GetDescriptor()->m_uiPinIndex;
          path.m_uiTargetNode = ObjectToIndex[pVsPinTarget->GetParent()];
          path.m_uiInputPin = pVsPinTarget->GetDescriptor()->m_uiPinIndex;
        }
      }
    }
  }
}

