#include <EditorPluginAssetsPCH.h>

#include <Core/WorldSerializer/ResourceHandleWriter.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/GUI/ExposedParameters.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptAsset.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptAssetManager.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptGraph.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptTypeRegistry.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>
#include <GameEngine/VisualScript/VisualScriptResource.h>
#include <ToolsFoundation/Command/NodeCommands.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <VisualShader/VisualShaderTypeRegistry.h>

//////////////////////////////////////////////////////////////////////////
// ezVisualScriptAssetDocument
//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptParameter, 1, ezRTTINoAllocator);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Name", m_sName),
    EZ_MEMBER_PROPERTY("Expose", m_bExpose),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptParameterBool, 1, ezRTTIDefaultAllocator<ezVisualScriptParameterBool>);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Default", m_DefaultValue),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptParameterNumber, 1, ezRTTIDefaultAllocator<ezVisualScriptParameterNumber>);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Default", m_DefaultValue),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptAssetProperties, 1, ezRTTIDefaultAllocator<ezVisualScriptAssetProperties>);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_MEMBER_PROPERTY("BoolParameters", m_BoolParameters),
    EZ_ARRAY_MEMBER_PROPERTY("NumberParameters", m_NumberParameters)
  }
    EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////
// ezVisualScriptAssetDocument
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptAssetDocument, 4, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezVisualScriptAssetDocument::ezVisualScriptAssetDocument(const char* szDocumentPath)
    : ezSimpleAssetDocument<ezVisualScriptAssetProperties>(EZ_DEFAULT_NEW(ezVisualScriptNodeManager), szDocumentPath, ezAssetDocEngineConnection::None)
{
  ezVisualScriptTypeRegistry::GetSingleton()->UpdateNodeTypes();
}

void ezVisualScriptAssetDocument::OnInterDocumentMessage(ezReflectedClass* pMessage, ezDocument* pSender)
{
  if (pMessage->GetDynamicRTTI()->IsDerivedFrom<ezVisualScriptActivityMsgToEditor>())
  {
    HandleVsActivityMsg(static_cast<ezVisualScriptActivityMsgToEditor*>(pMessage));
    return;
  }

  if (pMessage->GetDynamicRTTI()->IsDerivedFrom<ezGatherObjectsForDebugVisMsgInterDoc>())
  {
    m_InterDocumentMessages.Broadcast(pMessage);
  }
}

void ezVisualScriptAssetDocument::HandleVsActivityMsg(const ezVisualScriptActivityMsgToEditor* pActivityMsg)
{
  const auto& db = pActivityMsg->m_Activity;
  const ezUInt8* pData = db.GetData();

  ezUInt32* pNumExecCon = (ezUInt32*)pData;
  ezUInt32* pNumDataCon = (ezUInt32*)ezMemoryUtils::AddByteOffset(pData, 4);

  ezUInt32* pExecCon = (ezUInt32*)ezMemoryUtils::AddByteOffset(pData, 8);
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

ezStatus ezVisualScriptAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile,
                                                             const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  ezVisualScriptResourceDescriptor desc;
  if (GenerateVisualScriptDescriptor(desc).Failed())
  {
    ezLog::Warning("Couldn't generate visual script descriptor!");
    return ezStatus(EZ_FAILURE);
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

        inout_uiHash = ezHashingUtils::xxHash64(&pPinSource->GetParent()->GetGuid(), sizeof(ezUuid), inout_uiHash);
        inout_uiHash = ezHashingUtils::xxHash64(&pPinTarget->GetParent()->GetGuid(), sizeof(ezUuid), inout_uiHash);
        inout_uiHash =
            ezHashingUtils::xxHash64(pPinSource->GetName(), ezStringUtils::GetStringElementCount(pPinSource->GetName()), inout_uiHash);
        inout_uiHash =
            ezHashingUtils::xxHash64(pPinTarget->GetName(), ezStringUtils::GetStringElementCount(pPinTarget->GetName()), inout_uiHash);
      }
    }
  }
}

void ezVisualScriptAssetDocument::AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const
{
  const ezDocumentNodeManager* pManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(graph);
}

void ezVisualScriptAssetDocument::RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph, bool bUndoable)
{
  ezDocumentNodeManager* pManager = static_cast<ezDocumentNodeManager*>(GetObjectManager());
  pManager->RestoreMetaDataAfterLoading(graph, bUndoable);
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

    if (!pDesc)
    {
      ezLog::SeriousWarning("Couldn't get descriptor from type registry. Are all plugins loaded?");
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

  // local variables
  {
    desc.m_BoolParameters.Clear();
    desc.m_BoolParameters.Reserve(GetProperties()->m_BoolParameters.GetCount());

    for (const auto& p : GetProperties()->m_BoolParameters)
    {
      if (p.m_sName.IsEmpty())
      {
        ezLog::Warning("Visual script declared an unnamed bool variable. Variable is ignored.");
        continue;
      }

      auto& outP = desc.m_BoolParameters.ExpandAndGetRef();
      outP.m_sName.Assign(p.m_sName.GetData());
      outP.m_Value = p.m_DefaultValue;
    }

    desc.m_NumberParameters.Clear();
    desc.m_NumberParameters.Reserve(GetProperties()->m_NumberParameters.GetCount());

    for (const auto& p : GetProperties()->m_NumberParameters)
    {
      if (p.m_sName.IsEmpty())
      {
        ezLog::Warning("Visual script declared an unnamed number variable. Variable is ignored.");
        continue;
      }

      auto& outP = desc.m_NumberParameters.ExpandAndGetRef();
      outP.m_sName.Assign(p.m_sName.GetData());
      outP.m_Value = p.m_DefaultValue;
    }
  }

  // verify used local variables
  {
    for (ezUInt32 i = 0; i < allNodes.GetCount(); ++i)
    {
      const ezDocumentObject* pObject = allNodes[i];
      const ezVisualScriptNodeDescriptor* pDesc = pTypeRegistry->GetDescriptorForType(pObject->GetType());

      if (pDesc->m_sTypeName == "ezVisualScriptNode_Bool" || pDesc->m_sTypeName == "ezVisualScriptNode_Number" ||
          pDesc->m_sTypeName == "ezVisualScriptNode_StoreNumber" || pDesc->m_sTypeName == "ezVisualScriptNode_StoreBool" ||
          pDesc->m_sTypeName == "ezVisualScriptNode_ToggleBool")
      {
        const ezVariant varName = pObject->GetTypeAccessor().GetValue("Name");
        EZ_ASSERT_DEBUG(varName.IsA<ezString>(), "Missing or invalid property");

        enum class ValueType
        {
          Bool,
          Number
        };

        ValueType eValueType = ValueType::Number;

        if (pDesc->m_sTypeName == "ezVisualScriptNode_Bool" || pDesc->m_sTypeName == "ezVisualScriptNode_StoreBool" ||
            pDesc->m_sTypeName == "ezVisualScriptNode_ToggleBool")
        {
          eValueType = ValueType::Bool;
        }

        const ezString name = varName.ConvertTo<ezString>();

        bool found = false;

        if (eValueType == ValueType::Bool)
        {
          for (const auto& p : desc.m_BoolParameters)
          {
            if (p.m_sName == name)
            {
              found = true;
              break;
            }
          }
        }

        if (eValueType == ValueType::Number)
        {
          for (const auto& p : desc.m_NumberParameters)
          {
            if (p.m_sName == name)
            {
              found = true;
              break;
            }
          }
        }

        if (!found)
        {
          ezLog::Error("Visual Script uses undeclared {0} variable '{1}'.", (eValueType == ValueType::Bool) ? "bool" : "number", name);
        }
      }
    }
  }

  return EZ_SUCCESS;
}


void ezVisualScriptAssetDocument::GetAllVsNodes(ezDynamicArray<const ezDocumentObject*>& allNodes) const
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

void ezVisualScriptAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  ezExposedParameters* pExposedParams = EZ_DEFAULT_NEW(ezExposedParameters);

  {
    for (const auto& p : GetProperties()->m_BoolParameters)
    {
      if (p.m_bExpose)
      {
        ezExposedParameter* param = EZ_DEFAULT_NEW(ezExposedParameter);
        pExposedParams->m_Parameters.PushBack(param);
        param->m_sName = p.m_sName;
        param->m_DefaultValue = p.m_DefaultValue;
      }
    }

    for (const auto& p : GetProperties()->m_NumberParameters)
    {
      if (p.m_bExpose)
      {
        ezExposedParameter* param = EZ_DEFAULT_NEW(ezExposedParameter);
        pExposedParams->m_Parameters.PushBack(param);
        param->m_sName = p.m_sName;
        param->m_DefaultValue = p.m_DefaultValue;
      }
    }
  }

  // Info takes ownership of meta data.
  pInfo->m_MetaInfo.PushBack(pExposedParams);
}

void ezVisualScriptAssetDocument::GetSupportedMimeTypesForPasting(ezHybridArray<ezString, 4>& out_MimeTypes) const
{
  out_MimeTypes.PushBack("application/ezEditor.VisualScriptGraph");
}

bool ezVisualScriptAssetDocument::CopySelectedObjects(ezAbstractObjectGraph& out_objectGraph, ezStringBuilder& out_MimeType) const
{
  out_MimeType = "application/ezEditor.VisualScriptGraph";

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

bool ezVisualScriptAssetDocument::Paste(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph,
                                        bool bAllowPickedPosition, const char* szMimeType)
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

//////////////////////////////////////////////////////////////////////////

// don't think this will work

//#include <Foundation/Serialization/GraphPatch.h>
//
// class ezVisScriptAsset_3_4 : public ezGraphPatch
//{
// public:
//  ezVisScriptAsset_3_4()
//    : ezGraphPatch("ezVisualScriptAssetDocument", 4) {}
//
//  virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
//  {
//    ezVersionKey bases[] = { { "ezSimpleAssetDocument<ezVisualScriptAssetProperties>", 1 } };
//    context.ChangeBaseClass(bases);
//  }
//};
//
// ezVisScriptAsset_3_4 g_ezVisScriptAsset_3_4;
