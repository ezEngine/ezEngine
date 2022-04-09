#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/GUI/ExposedParameters.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptAsset.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptGraph.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptTypeRegistry.h>
#include <GameEngine/VisualScript/VisualScriptInstance.h>
#include <GameEngine/VisualScript/VisualScriptResource.h>

//////////////////////////////////////////////////////////////////////////
// ezVisualScriptAssetDocument
//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptParameter, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Name", m_sName),
    EZ_MEMBER_PROPERTY("Expose", m_bExpose),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptParameterBool, 1, ezRTTIDefaultAllocator<ezVisualScriptParameterBool>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Default", m_DefaultValue),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptParameterNumber, 1, ezRTTIDefaultAllocator<ezVisualScriptParameterNumber>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Default", m_DefaultValue),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptParameterString, 1, ezRTTIDefaultAllocator<ezVisualScriptParameterString>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Default", m_DefaultValue),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptAssetProperties, 1, ezRTTIDefaultAllocator<ezVisualScriptAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_MEMBER_PROPERTY("BoolParameters", m_BoolParameters),
    EZ_ARRAY_MEMBER_PROPERTY("NumberParameters", m_NumberParameters),
    EZ_ARRAY_MEMBER_PROPERTY("StringParameters", m_StringParameters)
  }
    EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////
// ezVisualScriptAssetDocument
//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptAssetDocument, 6, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezVisualScriptAssetDocument::ezVisualScriptAssetDocument(const char* szDocumentPath)
  : ezSimpleAssetDocument<ezVisualScriptAssetProperties>(EZ_DEFAULT_NEW(ezVisualScriptNodeManager_Legacy), szDocumentPath, ezAssetDocEngineConnection::None)
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

ezTransformStatus ezVisualScriptAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  ezVisualScriptResourceDescriptor desc;
  if (GenerateVisualScriptDescriptor(desc).Failed())
  {
    ezLog::Warning("Couldn't generate visual script descriptor!");
    return ezStatus(EZ_FAILURE);
  }

  desc.Save(stream);

  return ezStatus(EZ_SUCCESS);
}

void ezVisualScriptAssetDocument::InternalGetMetaDataHash(const ezDocumentObject* pObject, ezUInt64& inout_uiHash) const
{
  const ezDocumentNodeManager* pManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());
  pManager->GetMetaDataHash(pObject, inout_uiHash);
}

void ezVisualScriptAssetDocument::AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const
{
  SUPER::AttachMetaDataBeforeSaving(graph);
  const ezDocumentNodeManager* pManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(graph);
}

void ezVisualScriptAssetDocument::RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph, bool bUndoable)
{
  SUPER::RestoreMetaDataAfterLoading(graph, bUndoable);
  ezDocumentNodeManager* pManager = static_cast<ezDocumentNodeManager*>(GetObjectManager());
  pManager->RestoreMetaDataAfterLoading(graph, bUndoable);
}

ezResult ezVisualScriptAssetDocument::GenerateVisualScriptDescriptor(ezVisualScriptResourceDescriptor& desc)
{
  ezVisualScriptNodeManager_Legacy* pNodeManager = static_cast<ezVisualScriptNodeManager_Legacy*>(GetObjectManager());
  ezVisualScriptTypeRegistry* pTypeRegistry = ezVisualScriptTypeRegistry::GetSingleton();

  ezDynamicArray<const ezDocumentObject*> allNodes;
  GetAllVsNodes(allNodes);

  ezMap<const ezDocumentObject*, ezUInt16> ObjectToIndex;
  desc.m_Nodes.Reserve(allNodes.GetCount());

  for (ezUInt32 i = 0; i < allNodes.GetCount(); ++i)
  {
    const ezDocumentObject* pObject = allNodes[i];
    const ezVisualScriptNodeDescriptor* pDesc = pTypeRegistry->GetDescriptorForType(pObject->GetType());

    if (pDesc == nullptr)
    {
      ezLog::Error("Couldn't get descriptor from type registry. Are all required plugins loaded?");
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

        if (const ezVisScriptMappingAttribute* pMappingAttr = pProp->GetAttributeByType<ezVisScriptMappingAttribute>())
        {
          ref.m_iMappingIndex = pMappingAttr->m_iMapping;
        }

        node.m_uiNumProperties++;
      }
    }
  }

  for (ezUInt32 srcNodeIdx = 0; srcNodeIdx < allNodes.GetCount(); ++srcNodeIdx)
  {
    const ezDocumentObject* pSrcObject = allNodes[srcNodeIdx];

    auto outputPins = pNodeManager->GetOutputPins(pSrcObject);

    for (auto& pPin : outputPins)
    {
      const auto connections = pNodeManager->GetConnections(*pPin);

      for (const ezConnection* pCon : connections)
      {
        const ezVisualScriptPin_Legacy& vsPinSource = static_cast<const ezVisualScriptPin_Legacy&>(pCon->GetSourcePin());
        const ezVisualScriptPin_Legacy& vsPinTarget = static_cast<const ezVisualScriptPin_Legacy&>(pCon->GetTargetPin());

        if (vsPinSource.GetDescriptor()->m_PinType == ezVisualScriptPinDescriptor::PinType::Execution)
        {
          auto& path = desc.m_ExecutionPaths.ExpandAndGetRef();
          path.m_uiSourceNode = srcNodeIdx;
          path.m_uiOutputPin = vsPinSource.GetDescriptor()->m_uiPinIndex;

          path.m_uiTargetNode = ObjectToIndex[vsPinTarget.GetParent()];
          path.m_uiInputPin = vsPinTarget.GetDescriptor()->m_uiPinIndex;
        }
        else if (vsPinSource.GetDescriptor()->m_PinType == ezVisualScriptPinDescriptor::PinType::Data)
        {
          auto& path = desc.m_DataPaths.ExpandAndGetRef();
          path.m_uiSourceNode = srcNodeIdx;
          path.m_uiOutputPin = vsPinSource.GetDescriptor()->m_uiPinIndex;
          path.m_uiOutputPinType = vsPinSource.GetDescriptor()->m_DataType;
          path.m_uiTargetNode = ObjectToIndex[vsPinTarget.GetParent()];
          path.m_uiInputPin = vsPinTarget.GetDescriptor()->m_uiPinIndex;
          path.m_uiInputPinType = vsPinTarget.GetDescriptor()->m_DataType;
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

    desc.m_StringParameters.Clear();
    desc.m_StringParameters.Reserve(GetProperties()->m_StringParameters.GetCount());

    for (const auto& p : GetProperties()->m_StringParameters)
    {
      if (p.m_sName.IsEmpty())
      {
        ezLog::Warning("Visual script declared an unnamed number variable. Variable is ignored.");
        continue;
      }

      auto& outP = desc.m_StringParameters.ExpandAndGetRef();
      outP.m_sName.Assign(p.m_sName.GetData());
      outP.m_sValue = p.m_DefaultValue;
    }
  }

  // verify used local variables
  {
    for (ezUInt32 i = 0; i < allNodes.GetCount(); ++i)
    {
      const ezDocumentObject* pObject = allNodes[i];
      const ezVisualScriptNodeDescriptor* pDesc = pTypeRegistry->GetDescriptorForType(pObject->GetType());

      if (pDesc->m_sTypeName == "ezVisualScriptNode_Bool" || pDesc->m_sTypeName == "ezVisualScriptNode_Number" || pDesc->m_sTypeName == "ezVisualScriptNode_String" || pDesc->m_sTypeName == "ezVisualScriptNode_StoreNumber" || pDesc->m_sTypeName == "ezVisualScriptNode_StoreBool" || pDesc->m_sTypeName == "ezVisualScriptNode_ToggleBool" || pDesc->m_sTypeName == "ezVisualScriptNode_StoreString")
      {
        const ezVariant varName = pObject->GetTypeAccessor().GetValue("Name");
        EZ_ASSERT_DEBUG(varName.IsA<ezString>(), "Missing or invalid property");
        const ezString name = varName.ConvertTo<ezString>();

        auto findVarName = [&](auto parameters) {
          for (const auto& p : parameters)
          {
            if (p.m_sName == name)
            {
              return true;
            }
          }

          return false;
        };

        bool found = false;
        const char* szValueType;

        if (pDesc->m_sTypeName == "ezVisualScriptNode_Bool" || pDesc->m_sTypeName == "ezVisualScriptNode_StoreBool" || pDesc->m_sTypeName == "ezVisualScriptNode_ToggleBool")
        {
          found = findVarName(desc.m_BoolParameters);
          szValueType = "bool";
        }
        else if (pDesc->m_sTypeName == "ezVisualScriptNode_String" || pDesc->m_sTypeName == "ezVisualScriptNode_StoreString")
        {
          found = findVarName(desc.m_StringParameters);
          szValueType = "string";
        }
        else
        {
          found = findVarName(desc.m_NumberParameters);
          szValueType = "number";
        }

        if (!found)
        {
          ezLog::Error("Visual Script uses undeclared {0} variable '{1}'.", szValueType, name);
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

    for (const auto& p : GetProperties()->m_StringParameters)
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

  const ezDocumentNodeManager* pManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());
  return pManager->CopySelectedObjects(out_objectGraph);
}

bool ezVisualScriptAssetDocument::Paste(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, const char* szMimeType)
{
  ezDocumentNodeManager* pManager = static_cast<ezDocumentNodeManager*>(GetObjectManager());
  return pManager->PasteObjects(info, objectGraph, ezQtNodeScene::GetLastMouseInteractionPos(), bAllowPickedPosition);
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
