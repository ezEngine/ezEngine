#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

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
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptClassAssetDocument, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezVisualScriptClassAssetDocument::ezVisualScriptClassAssetDocument(const char* szDocumentPath)
  : ezSimpleAssetDocument<ezVisualScriptClassAssetProperties>(EZ_DEFAULT_NEW(ezVisualScriptNodeManager), szDocumentPath, ezAssetDocEngineConnection::None)
{
  m_pObjectAccessor = EZ_DEFAULT_NEW(ezNodeCommandAccessor, GetCommandHistory());
}

ezTransformStatus ezVisualScriptClassAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
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

  ezHybridArray<const ezVisualScriptPin*, 16> pins;
  ezVisualScriptCompiler compiler;

  for (const ezDocumentObject* pObject : children)
  {
    if (pManager->IsNode(pObject) == false)
      continue;

    auto pNodeDesc = ezVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
    if (pNodeDesc == nullptr)
      return ezStatus(EZ_FAILURE);

    if (pManager->IsFilteredByBaseClass(pObject->GetType(), *pNodeDesc, sBaseClass, true))
      continue;

    const char* szFunctionName = ezStringUtils::FindLastSubString(pObject->GetType()->GetTypeName(), "::");
    if (szFunctionName != nullptr)
    {
      szFunctionName += 2;
    }

    for (auto& eventHandler : compiler.GetCompiledModule().m_Functions)
    {
      if (eventHandler.m_sName == szFunctionName)
      {
        return ezStatus(ezFmt("A event handler for '{}' already exists. Can't have multiple event handler for the same event.", szFunctionName));
      }
    }

    if (pNodeDesc->m_Type == ezVisualScriptNodeDescription::Type::EntryCall ||
        pNodeDesc->m_Type == ezVisualScriptNodeDescription::Type::MessageHandler)
    {
      pManager->GetOutputExecutionPins(pObject, pins);
      if (pins.IsEmpty())
        continue;

      if (pManager->GetConnections(*pins[0]).IsEmpty())
        continue;

      EZ_SUCCEED_OR_RETURN(compiler.AddFunction(szFunctionName, pNodeDesc->m_Type, pObject));
    }
  }

  ezStringBuilder sDumpPath;
  if (false)
  {
    sDumpPath.Format(":appdata/{}_AST.dgml", sScriptClassName);
  }
  EZ_SUCCEED_OR_RETURN(compiler.Compile(sDumpPath));

  auto& compiledModule = compiler.GetCompiledModule();
  EZ_SUCCEED_OR_RETURN(compiledModule.Serialize(stream, sBaseClassName, sScriptClassName));

  return ezStatus(EZ_SUCCESS);
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

bool ezVisualScriptClassAssetDocument::Paste(const ezArrayPtr<PasteInfo>& info, const ezAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, const char* szMimeType)
{
  ezDocumentNodeManager* pManager = static_cast<ezDocumentNodeManager*>(GetObjectManager());
  return pManager->PasteObjects(info, objectGraph, ezQtNodeScene::GetLastMouseInteractionPos(), bAllowPickedPosition);
}
