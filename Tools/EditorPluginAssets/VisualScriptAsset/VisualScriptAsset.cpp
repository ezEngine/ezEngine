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
#include <GameEngine/VisualScript/VisualScriptInstance.h>

//////////////////////////////////////////////////////////////////////////
// ezVisualScriptAssetDocument
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptAssetDocument, 3, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezVisualScriptAssetDocument::ezVisualScriptAssetDocument(const char* szDocumentPath)
  : ezAssetDocument(szDocumentPath, EZ_DEFAULT_NEW(ezVisualScriptNodeManager), false, false)
{
  ezVisualScriptTypeRegistry::GetSingleton()->UpdateNodeTypes();
}


void ezVisualScriptAssetDocument::OnInterDocumentMessage(ezReflectedClass* pMessage, ezDocument* pSender)
{
  if (pMessage->GetDynamicRTTI()->IsDerivedFrom<ezVisualScriptActivityMsgToEditor>())
  {
    HandleVsActivityMsg(static_cast<ezVisualScriptActivityMsgToEditor*>(pMessage));
  }
}

void ezVisualScriptAssetDocument::HandleVsActivityMsg(const ezVisualScriptActivityMsgToEditor* pActivityMsg)
{
  const auto& db = pActivityMsg->m_Activity;
  const ezUInt8* pData = db.GetData();

  ezUInt32* pNumExecCon = (ezUInt32*)pData;
  ezUInt32* pNumDataCon = (ezUInt32*)ezMemoryUtils::AddByteOffsetConst(pData, 4);

  ezUInt32* pExecCon = (ezUInt32*)ezMemoryUtils::AddByteOffsetConst(pData, 8);
  ezUInt32* pDataCon = pExecCon + (*pNumExecCon);

  ezVisualScriptInstanceActivity act;
  act.m_ActiveExecutionConnections.SetCountUninitialized(*pNumExecCon);
  act.m_ActiveDataConnections.SetCountUninitialized(*pNumDataCon);

  ezMemoryUtils::Copy<ezUInt32>(act.m_ActiveExecutionConnections.GetData(), pExecCon, act.m_ActiveExecutionConnections.GetCount());
  ezMemoryUtils::Copy<ezUInt32>(act.m_ActiveDataConnections.GetData(), pDataCon, act.m_ActiveDataConnections.GetCount());

  ezVisualScriptActivityEvent ae;
  ae.m_pActivityData = &act;
  ae.m_ObjectGuid = pActivityMsg->m_ComponentGuid;

  m_ActivityEvents.Broadcast(ae);
}

ezStatus ezVisualScriptAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually)
{
  ezVisualScriptResourceDescriptor desc;
  if ( GenerateVisualScriptDescriptor( desc ).Failed() )
  {
    ezLog::Warning( "Couldn't generate visual script descriptor!" );
    return ezStatus( EZ_FAILURE );
  }

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

ezResult ezVisualScriptAssetDocument::GenerateVisualScriptDescriptor(ezVisualScriptResourceDescriptor& desc)
{
  ezVisualScriptNodeManager* pNodeManager = static_cast<ezVisualScriptNodeManager*>(GetObjectManager());
  ezVisualScriptTypeRegistry* pTypeRegistry = ezVisualScriptTypeRegistry::GetSingleton();

  ezDynamicArray<const ezDocumentObject*> allNodes;
  GetAllVsNodes(allNodes);

  ezMap<const ezDocumentObject*, ezUInt16> ObjectToIndex;
  desc.m_Nodes.Reserve(allNodes.GetCount());

  for (ezUInt32 i = 0; i < allNodes.GetCount(); ++i)
  {
    const ezDocumentObject* pObject = allNodes[i];
    const ezVisualScriptNodeDescriptor* pDesc = pTypeRegistry->GetDescriptorForType(pObject->GetType());

    if ( !pDesc )
    {
      ezLog::SeriousWarning( "Couldn't get descriptor from type registry. Are all plugins loaded?" );
      return EZ_FAILURE;
    }

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
          path.m_uiOutputPinType = pVsPinSource->GetDescriptor()->m_DataType;
          path.m_uiTargetNode = ObjectToIndex[pVsPinTarget->GetParent()];
          path.m_uiInputPin = pVsPinTarget->GetDescriptor()->m_uiPinIndex;
          path.m_uiInputPinType = pVsPinTarget->GetDescriptor()->m_DataType;
        }
      }
    }
  }

  return EZ_SUCCESS;
}


void ezVisualScriptAssetDocument::GetAllVsNodes(ezDynamicArray<const ezDocumentObject *>& allNodes) const
{
  ezVisualScriptTypeRegistry* pTypeRegistry = ezVisualScriptTypeRegistry::GetSingleton();
  const ezRTTI* pNodeBaseRtti = pTypeRegistry->GetNodeBaseType();

  allNodes.Clear();
  allNodes.Reserve(64);

  const auto& children = GetObjectManager()->GetRootObject()->GetChildren();
  for (const ezDocumentObject* pObject : children)
  {
    auto pType = pObject->GetTypeAccessor().GetType();
    if (!pType->IsDerivedFrom(pNodeBaseRtti))
      continue;

    allNodes.PushBack(pObject);
  }
}

void ezVisualScriptAssetDocument::GetSupportedMimeTypesForPasting(ezHybridArray<ezString, 4>& out_MimeTypes) const
{
  out_MimeTypes.PushBack("application/ezEditor.VisualScriptGraph");
}

bool ezVisualScriptAssetDocument::Copy(ezAbstractObjectGraph& out_objectGraph, ezStringBuilder& out_MimeType) const
{
  out_MimeType = "application/ezEditor.VisualScriptGraph";

  const auto& selection = GetSelectionManager()->GetSelection();

  if (selection.IsEmpty())
    return false;

  const ezDocumentNodeManager* pManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());

  ezDocumentObjectConverterWriter writer(&out_objectGraph, pManager, true, true);

  for (const ezDocumentObject* pNode : selection)
  {
    // objects are required to be named root but this is not enforced or obvious by the interface.
    writer.AddObjectToGraph(pNode, "root");
  }

  pManager->AttachMetaDataBeforeSaving(out_objectGraph);

  return true;
}

bool ezVisualScriptAssetDocument::Paste(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, const char* szMimeType)
{
  bool bAddedAll = true;

  ezDeque<const ezDocumentObject*> AddedNodes;

  for (const PasteInfo& pi : info)
  {
    // only add nodes that are allowed to be added
    if (GetObjectManager()->CanAdd(pi.m_pObject->GetTypeAccessor().GetType(), nullptr, "Children", -1).m_Result.Succeeded())
    {
      AddedNodes.PushBack(pi.m_pObject);
      GetObjectManager()->AddObject(pi.m_pObject, nullptr, "Children", -1);
    }
    else
    {
      bAddedAll = false;
    }
  }

  m_DocumentObjectMetaData.RestoreMetaDataFromAbstractGraph(objectGraph);

  RestoreMetaDataAfterLoading(objectGraph);

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
      pManager->MoveNode(pNode, pManager->GetNodePos(pNode) + vMoveNode);
    }

    if (!bAddedAll)
    {
      ezLog::Info("[EditorStatus]Not all nodes were allowed to be added to the document");
    }

  }

  GetSelectionManager()->SetSelection(AddedNodes);
  return true;
}
