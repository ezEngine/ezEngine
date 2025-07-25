#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/GUI/ExposedParameters.h>
#include <EditorPluginVisualScript/VisualScriptClassAsset/VisualScriptClassAsset.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptCompiler.h>
#include <GuiFoundation/NodeEditor/NodeScene.moc.h>
#include <ToolsFoundation/NodeObject/NodeCommandAccessor.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptClassAssetProperties, 1, ezRTTIDefaultAllocator<ezVisualScriptClassAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("BaseClass", m_sBaseClass)->AddAttributes(new ezDefaultValueAttribute(ezStringView("Component")), new ezDynamicStringEnumAttribute("ScriptBaseClasses")),
    EZ_ARRAY_MEMBER_PROPERTY("Variables", m_Variables),
    EZ_MEMBER_PROPERTY("DumpAST", m_bDumpAST),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptClassAssetDocument, 10, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptClassAssetDocument::ezVisualScriptClassAssetDocument(ezStringView sDocumentPath)
  : ezSimpleAssetDocument<ezVisualScriptClassAssetProperties>(EZ_DEFAULT_NEW(ezVisualScriptNodeManager), sDocumentPath, ezAssetDocEngineConnection::None)
{
  m_pObjectAccessor = EZ_DEFAULT_NEW(ezNodeCommandAccessor, GetCommandHistory());
}

ezTransformStatus ezVisualScriptClassAssetDocument::InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  auto pManager = static_cast<ezVisualScriptNodeManager*>(GetObjectManager());

  const auto& children = pManager->GetRootObject()->GetChildren();
  ezHashedString sBaseClass = pManager->GetScriptBaseClass();

  ezStringBuilder sBaseClassName = sBaseClass.GetView();
  if (ezRTTI::FindTypeByName(sBaseClassName) == nullptr)
  {
    sBaseClassName.Prepend("ez");
    if (ezRTTI::FindTypeByName(sBaseClassName) == nullptr)
    {
      return ezStatus(ezFmt("Invalid base class '{}'", sBaseClassName));
    }
  }

  ezStringView sScriptClassName = ezPathUtils::GetFileName(GetDocumentPath());

  ezVisualScriptCompiler compiler(*pManager);
  compiler.InitModule(sBaseClassName, sScriptClassName);

  ezHybridArray<const ezVisualScriptPin*, 16> pins;
  for (const ezDocumentObject* pObject : children)
  {
    if (pManager->IsNode(pObject) == false)
      continue;

    auto pNodeDesc = ezVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
    if (pNodeDesc == nullptr)
      return ezStatus(EZ_FAILURE);

    if (pManager->IsFilteredByBaseClass(pObject->GetType(), *pNodeDesc, sBaseClass, true))
      continue;

    if (ezVisualScriptNodeDescription::Type::IsEntry(pNodeDesc->m_Type))
    {
      pManager->GetOutputExecutionPins(pObject, pins);
      if (pins.IsEmpty())
        continue;

      if (pManager->GetConnections(*pins[0]).IsEmpty())
        continue;

      ezStringView sFunctionName = ezVisualScriptNodeManager::GetNiceFunctionName(pObject);
      EZ_SUCCEED_OR_RETURN(compiler.AddFunction(sFunctionName, pObject));
    }
  }

  ezStringBuilder sDumpPath;
  if (GetProperties()->m_bDumpAST)
  {
    sDumpPath.SetFormat(":appdata/{}_AST.dgml", sScriptClassName);
  }
  EZ_SUCCEED_OR_RETURN(compiler.Compile(sDumpPath));

  auto& compiledModule = compiler.GetCompiledModule();
  EZ_SUCCEED_OR_RETURN(compiledModule.Serialize(stream));

  return ezStatus(EZ_SUCCESS);
}

void ezVisualScriptClassAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  ezExposedParameters* pExposedParams = EZ_DEFAULT_NEW(ezExposedParameters);

  for (const auto& v : GetProperties()->m_Variables)
  {
    if (v.m_bExpose == false)
      continue;

    ezExposedParameter* param = EZ_DEFAULT_NEW(ezExposedParameter);
    param->m_sName = v.m_sName.GetString();
    param->m_DefaultValue = v.m_DefaultValue;

    pExposedParams->m_Parameters.PushBack(param);
  }

  // Info takes ownership of meta data.
  pInfo->m_MetaInfo.PushBack(pExposedParams);
}

void ezVisualScriptClassAssetDocument::InternalGetMetaDataHash(const ezDocumentObject* pObject, ezUInt64& inout_uiHash) const
{
  auto pManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());
  pManager->GetMetaDataHash(pObject, inout_uiHash);
}

void ezVisualScriptClassAssetDocument::AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const
{
  SUPER::AttachMetaDataBeforeSaving(graph);
  const auto pManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());
  pManager->AttachMetaDataBeforeSaving(graph);
}

void ezVisualScriptClassAssetDocument::RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph, bool bUndoable)
{
  SUPER::RestoreMetaDataAfterLoading(graph, bUndoable);
  auto pManager = static_cast<ezDocumentNodeManager*>(GetObjectManager());
  pManager->RestoreMetaDataAfterLoading(graph, bUndoable);
}

void ezVisualScriptClassAssetDocument::GetSupportedMimeTypesForPasting(ezHybridArray<ezString, 4>& out_MimeTypes) const
{
  out_MimeTypes.PushBack("application/ezEditor.VisualScriptClassGraph");
}

bool ezVisualScriptClassAssetDocument::CopySelectedObjects(ezAbstractObjectGraph& out_objectGraph, ezStringBuilder& out_MimeType) const
{
  out_MimeType = "application/ezEditor.VisualScriptClassGraph";

  const ezDocumentNodeManager* pManager = static_cast<const ezDocumentNodeManager*>(GetObjectManager());
  return pManager->CopySelectedObjects(out_objectGraph);
}

bool ezVisualScriptClassAssetDocument::Paste(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, ezStringView sMimeType)
{
  ezDocumentNodeManager* pManager = static_cast<ezDocumentNodeManager*>(GetObjectManager());
  return pManager->PasteObjects(info, objectGraph, ezQtNodeScene::GetLastMouseInteractionPos(), bAllowPickedPosition);
}
