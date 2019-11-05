#include <EditorPluginTypeScriptPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAsset.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAssetManager.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAssetWindow.moc.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Utilities/Progress.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <TypeScriptPlugin/Resources/ScriptCompendiumResource.h>
#include <TypeScriptPlugin/TsBinding/TsBinding.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTypeScriptAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezTypeScriptAssetDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezTypeScriptAssetDocumentManager::ezTypeScriptAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezTypeScriptAssetDocumentManager::OnDocumentManagerEvent, this));

  m_AssetDesc.m_bCanCreate = true;
  m_AssetDesc.m_sDocumentTypeName = "TypeScript Asset";
  m_AssetDesc.m_sFileExtension = "ezTypeScriptAsset";
  m_AssetDesc.m_sIcon = ":/AssetIcons/TypeScript.png";
  m_AssetDesc.m_pDocumentType = ezGetStaticRTTI<ezTypeScriptAssetDocument>();
  m_AssetDesc.m_pManager = this;

  ezQtImageCache::GetSingleton()->RegisterTypeImage("TypeScript", QPixmap(":/AssetIcons/TypeScript.png"));

  ezToolsProject::s_Events.AddEventHandler(ezMakeDelegate(&ezTypeScriptAssetDocumentManager::ToolsProjectEventHandler, this));
}

ezTypeScriptAssetDocumentManager::~ezTypeScriptAssetDocumentManager()
{
  ezToolsProject::s_Events.RemoveEventHandler(ezMakeDelegate(&ezTypeScriptAssetDocumentManager::ToolsProjectEventHandler, this));

  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezTypeScriptAssetDocumentManager::OnDocumentManagerEvent, this));
}

void ezTypeScriptAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezTypeScriptAssetDocument>())
      {
        ezQtTypeScriptAssetDocumentWindow* pDocWnd =
          new ezQtTypeScriptAssetDocumentWindow(static_cast<ezTypeScriptAssetDocument*>(e.m_pDocument));
      }
    }
    break;
  }
}

ezStatus ezTypeScriptAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, ezDocument*& out_pDocument)
{
  out_pDocument = new ezTypeScriptAssetDocument(szPath);

  return ezStatus(EZ_SUCCESS);
}

void ezTypeScriptAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_AssetDesc);
}

ezBitflags<ezAssetDocumentFlags>
ezTypeScriptAssetDocumentManager::GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const
{
  EZ_ASSERT_DEBUG(pDescriptor->m_pManager == this, "Given type descriptor is not part of this document manager!");
  return ezAssetDocumentFlags::None;
}

void ezTypeScriptAssetDocumentManager::ToolsProjectEventHandler(const ezToolsProjectEvent& e)
{
  if (e.m_Type == ezToolsProjectEvent::Type::ProjectOpened)
  {
    InitializeTranspiler();
    SetupProjectForTypeScript();
  }
}

void ezTypeScriptAssetDocumentManager::InitializeTranspiler()
{
  if (m_bTranspilerLoaded)
    return;

  m_bTranspilerLoaded = true;

  ezFileSystem::AddDataDirectory(">sdk/Data/Tools/ezEditor/TypeScript", "TypeScript", "TypeScript");

  m_Transpiler.SetOutputFolder(":project/AssetCache/Temp");
  m_Transpiler.StartLoadTranspiler();
}

void ezTypeScriptAssetDocumentManager::SetupProjectForTypeScript()
{
  if (ezTypeScriptBinding::SetupProjectCode().Failed())
  {
    ezLog::Error("Could not setup Typescript data in project directory");
    return;
  }
}

ezResult ezTypeScriptAssetDocumentManager::GenerateScriptCompendium(ezBitflags<ezTransformFlags> transformFlags)
{
  EZ_LOG_BLOCK("Generating Script Compendium");

  // TODO: store GUID of all TypeScript assets with lookup to ComponentName + relative path to file -> get rid of ezJavaScriptResource

  ezStringBuilder sProjectPath = ezToolsProject::GetSingleton()->GetProjectDirectory();
  sProjectPath.MakeCleanPath();

  ezFileSystemIterator fsIt;
  fsIt.StartSearch(sProjectPath, ezFileSystemIteratorFlags::ReportFilesRecursive);

  ezScriptCompendiumResourceDesc compendium;

  bool bAnythingNew = false;

  ezStringBuilder sTsFilePath;
  for (; fsIt.IsValid(); fsIt.Next())
  {
    if (!fsIt.GetStats().m_sName.EndsWith_NoCase(".ts"))
      continue;

    fsIt.GetStats().GetFullPath(sTsFilePath);

    sTsFilePath.MakeRelativeTo(sProjectPath);

    compendium.m_PathToSource.Insert(sTsFilePath, ezString());

    ezTimestamp& lastModification = m_CheckedTsFiles[sTsFilePath];

    if (!lastModification.Compare(fsIt.GetStats().m_LastModificationTime, ezTimestamp::CompareMode::FileTimeEqual))
    {
      bAnythingNew = true;
      lastModification = fsIt.GetStats().m_LastModificationTime;
    }
  }

  ezStringBuilder sOutFile(":project/AssetCache/Common/Scripts.ezScriptCompendium");

  if (!transformFlags.IsSet(ezTransformFlags::ForceTransform))
  {
    if (bAnythingNew == false && ezFileSystem::ExistsFile(sOutFile))
      return EZ_SUCCESS;
  }

  ezMap<ezString, ezString> filenameToSourceTsPath;

  ezProgressRange progress("Transpiling Scripts", compendium.m_PathToSource.GetCount(), true);

  // remove the output file, so that if anything fails from here on out, it will be re-generated next time
  ezFileSystem::DeleteFile(sOutFile);

  ezStringBuilder sFilename;

  // TODO: could multi-thread this, if we had multiple transpilers loaded
  {
    ezStringBuilder sTranspiledJs;
    for (auto it : compendium.m_PathToSource)
    {
      if (!progress.BeginNextStep(it.Key()))
        return EZ_FAILURE;

      if (m_Transpiler.TranspileFileAndStoreJS(it.Key(), sTranspiledJs).Failed())
      {
        ezLog::Error("Failed to transpile '{}'", it.Key());
        return EZ_FAILURE;
      }

      it.Value() = sTranspiledJs;

      sFilename = ezPathUtils::GetFileName(it.Key());
      filenameToSourceTsPath[sFilename] = it.Key();
    }
  }

  // at runtime we need to be able to load a typescript component
  // at edit time, the ezTypeScriptComponent should present the component type as a reference to an asset document
  // thus at edit time, this reference should look like a path to a document
  // however, at runtime we only need the name of the component type to instantiate (for the call to 'new' in Duktape/JS)
  // and the relative path to the source ts/js file (for the call to 'require' in Duktape/JS to 'load' the module)
  // just for these two strings we do not want to load an entire resource, as we would typically do with other asset types
  // therefore we extract the required data (component name and path) here and store it in the compendium
  // now all we need is the GUID of the TypeScript asset to look up this information at runtime
  // thus the ezTypeScriptComponent does not need to store the asset document reference as a full string (path), but can just
  // store it as the GUID
  // at runtime this 'path' is not used as an ezResource path/id, as would be common, but is used to look up the information
  // directly from the compendium
  {
    ezAssetCurator* pCurator = ezAssetCurator::GetSingleton();
    const auto& allAssets = pCurator->GetKnownSubAssets();

    for (auto it = allAssets->GetIterator(); it.IsValid(); ++it)
    {
      const auto& asset = it.Value();

      if (asset.m_pAssetInfo->m_pManager != this)
        continue;

      const ezString& docPath = asset.m_pAssetInfo->m_sDataDirRelativePath;
      const ezUuid& docGuid = asset.m_pAssetInfo->m_Info->m_DocumentID;

      sFilename = ezPathUtils::GetFileName(docPath);

      // TODO: handle filenameToSourceTsPath[sFilename] == "" case (log error)
      compendium.m_AssetGuidToInfo[docGuid].m_sComponentTypeName = sFilename;
      compendium.m_AssetGuidToInfo[docGuid].m_sComponentFilePath = filenameToSourceTsPath[sFilename];
    }
  }

  {
    ezDeferredFileWriter file;
    file.SetOutput(sOutFile);

    ezAssetFileHeader header;
    header.SetFileHashAndVersion(1, 1);

    header.Write(file);

    compendium.Serialize(file);

    EZ_SUCCEED_OR_RETURN(file.Close());
  }

  return EZ_SUCCESS;
}
