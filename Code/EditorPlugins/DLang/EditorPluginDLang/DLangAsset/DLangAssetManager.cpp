#include <EditorPluginDLang/EditorPluginDLangPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorPluginDLang/DLangAsset/DLangAsset.h>
#include <EditorPluginDLang/DLangAsset/DLangAssetManager.h>
#include <EditorPluginDLang/DLangAsset/DLangAssetWindow.moc.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Utilities/Progress.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>
#include <ToolsFoundation/Command/TreeCommands.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDLangAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezDLangAssetDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezDLangAssetDocumentManager::ezDLangAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezDLangAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "DLang";
  m_DocTypeDesc.m_sFileExtension = "ezDLangAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/DLang.svg";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezDLangAssetDocument>();
  m_DocTypeDesc.m_pManager = this;

  m_DocTypeDesc.m_sResourceFileExtension = "ezDLangRes";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::None;

  ezQtImageCache::GetSingleton()->RegisterTypeImage("DLang", QPixmap(":/AssetIcons/DLang.svg"));

  ezGameObjectDocument::s_GameObjectDocumentEvents.AddEventHandler(ezMakeDelegate(&ezDLangAssetDocumentManager::GameObjectDocumentEventHandler, this));

  // make sure the preferences exist
  ezPreferences::QueryPreferences<ezDLangPreferences>();
}

ezDLangAssetDocumentManager::~ezDLangAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezDLangAssetDocumentManager::OnDocumentManagerEvent, this));

  ezGameObjectDocument::s_GameObjectDocumentEvents.RemoveEventHandler(ezMakeDelegate(&ezDLangAssetDocumentManager::GameObjectDocumentEventHandler, this));
}

void ezDLangAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezDLangAssetDocument>())
      {
        ezQtDLangAssetDocumentWindow* pDocWnd = new ezQtDLangAssetDocumentWindow(static_cast<ezDLangAssetDocument*>(e.m_pDocument));
      }
    }
    break;

    default:
      break;
  }
}

void ezDLangAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext)
{
  out_pDocument = new ezDLangAssetDocument(szPath);
}

void ezDLangAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}

void ezDLangAssetDocumentManager::GameObjectDocumentEventHandler(const ezGameObjectDocumentEvent& e)
{
  switch (e.m_Type)
  {
    case ezGameObjectDocumentEvent::Type::GameMode_StartingSimulate:
    {
      //if (ezPreferences::QueryPreferences<ezDLangPreferences>()->m_bAutoUpdateScriptsForSimulation)
      //{
      //  GenerateScriptCompendium(ezTransformFlags::Default).IgnoreResult();
      //}
      break;
    }

    case ezGameObjectDocumentEvent::Type::GameMode_StartingPlay:
    case ezGameObjectDocumentEvent::Type::GameMode_StartingExternal:
    {
      //if (ezPreferences::QueryPreferences<ezDLangPreferences>()->m_bAutoUpdateScriptsForPlayTheGame)
      //{
      //  GenerateScriptCompendium(ezTransformFlags::Default).IgnoreResult();
      //}
      break;
    }

    default:
      break;
  }
}

//void ezDLangAssetDocumentManager::SetupProjectForDLang(bool bForce)
//{
//  if (m_bProjectSetUp && !bForce)
//    return;
//
//  m_bProjectSetUp = true;
//
//  if (ezDLangBinding::SetupProjectCode().Failed())
//  {
//    ezLog::Error("Could not setup Typescript data in project directory");
//    return;
//  }
//}

//ezResult ezDLangAssetDocumentManager::GenerateScriptCompendium(ezBitflags<ezTransformFlags> transformFlags)
//{
//  EZ_LOG_BLOCK("Generating Script Compendium");
//
//  ezHybridArray<ezAssetInfo*, 256> allTsAssets;
//
//  // keep this locked until the end of the function
//  ezAssetCurator::ezLockedSubAssetTable AllAssetsLocked = ezAssetCurator::GetSingleton()->GetKnownSubAssets();
//  const ezHashTable<ezUuid, ezSubAsset>& AllAssets = *AllAssetsLocked;
//
//  for (auto it = AllAssets.GetIterator(); it.IsValid(); ++it)
//  {
//    const ezSubAsset* pSub = &it.Value();
//
//    if (pSub->m_pAssetInfo->GetManager() == this)
//    {
//      allTsAssets.PushBack(pSub->m_pAssetInfo);
//    }
//  }
//
//  if (allTsAssets.IsEmpty())
//  {
//    ezLog::Debug("Skipping script compendium creation - no DLang assets in project.");
//    return EZ_SUCCESS;
//  }
//
//
//  SetupProjectForDLang(false);
//
//  // read m_CheckedTsFiles cache
//  if (m_CheckedTsFiles.IsEmpty())
//  {
//    ezStringBuilder sFile = ezApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
//    sFile.AppendPath("LastDLangChanges.tmp");
//
//    ezFileReader file;
//    if (file.Open(sFile).Succeeded())
//    {
//      file.ReadMap(m_CheckedTsFiles).IgnoreResult();
//    }
//  }
//
//  ezMap<ezString, ezUInt32> relPathToDataDirIdx;
//
//  ezScriptCompendiumResourceDesc compendium;
//  bool bAnythingNew = false;
//
//  for (ezUInt32 ddIdx = 0; ddIdx < ezFileSystem::GetNumDataDirectories(); ++ddIdx)
//  {
//    ezStringBuilder sDataDirPath = ezFileSystem::GetDataDirectory(ddIdx)->GetRedirectedDataDirectoryPath();
//    sDataDirPath.MakeCleanPath();
//
//    if (!sDataDirPath.IsAbsolutePath())
//      continue;
//
//    ezFileSystemIterator fsIt;
//    fsIt.StartSearch(sDataDirPath, ezFileSystemIteratorFlags::ReportFilesRecursive);
//
//    ezStringBuilder sTsFilePath;
//    for (; fsIt.IsValid(); fsIt.Next())
//    {
//      if (!fsIt.GetStats().m_sName.EndsWith_NoCase(".ts"))
//        continue;
//
//      fsIt.GetStats().GetFullPath(sTsFilePath);
//
//      sTsFilePath.MakeRelativeTo(sDataDirPath).IgnoreResult();
//
//      relPathToDataDirIdx[sTsFilePath] = ddIdx;
//
//      compendium.m_PathToSource.Insert(sTsFilePath, ezString());
//
//      ezTimestamp& lastModification = m_CheckedTsFiles[sTsFilePath];
//
//      if (!lastModification.Compare(fsIt.GetStats().m_LastModificationTime, ezTimestamp::CompareMode::FileTimeEqual))
//      {
//        bAnythingNew = true;
//        lastModification = fsIt.GetStats().m_LastModificationTime;
//      }
//    }
//  }
//
//  ezStringBuilder sOutFile(":project/AssetCache/Common/Scripts.ezScriptCompendium");
//
//  if (!transformFlags.IsSet(ezTransformFlags::ForceTransform))
//  {
//    if (bAnythingNew == false && ezFileSystem::ExistsFile(sOutFile))
//      return EZ_SUCCESS;
//  }
//
//  ezMap<ezString, ezString> filenameToSourceTsPath;
//
//  ezProgressRange progress("Transpiling Scripts", compendium.m_PathToSource.GetCount(), true);
//
//  // remove the output file, so that if anything fails from here on out, it will be re-generated next time
//  ezFileSystem::DeleteFile(sOutFile);
//
//  ezStringBuilder sFilename;
//
//  // TODO: could multi-thread this, if we had multiple transpilers loaded
//  {
//    ezStringBuilder sOutputFolder;
//
//    ezStringBuilder sTranspiledJs;
//    for (auto it : compendium.m_PathToSource)
//    {
//      if (!progress.BeginNextStep(it.Key()))
//        return EZ_FAILURE;
//
//      sOutputFolder = ezFileSystem::GetDataDirectory(relPathToDataDirIdx[it.Key()])->GetRedirectedDataDirectoryPath();
//      sOutputFolder.MakeCleanPath();
//      sOutputFolder.AppendPath("AssetCache/Temp");
//      m_Transpiler.SetOutputFolder(sOutputFolder);
//
//      if (m_Transpiler.TranspileFileAndStoreJS(it.Key(), sTranspiledJs).Failed())
//      {
//        ezLog::Error("Failed to transpile '{}'", it.Key());
//        return EZ_FAILURE;
//      }
//
//      it.Value() = sTranspiledJs;
//
//      sFilename = ezPathUtils::GetFileName(it.Key());
//      filenameToSourceTsPath[sFilename] = it.Key();
//    }
//  }
//
//  // at runtime we need to be able to load a typescript component
//  // at edit time, the ezDLangComponent should present the component type as a reference to an asset document
//  // thus at edit time, this reference should look like a path to a document
//  // however, at runtime we only need the name of the component type to instantiate (for the call to 'new' in Duktape/JS)
//  // and the relative path to the source ts/js file (for the call to 'require' in Duktape/JS to 'load' the module)
//  // just for these two strings we do not want to load an entire resource, as we would typically do with other asset types
//  // therefore we extract the required data (component name and path) here and store it in the compendium
//  // now all we need is the GUID of the DLang asset to look up this information at runtime
//  // thus the ezDLangComponent does not need to store the asset document reference as a full string (path), but can just
//  // store it as the GUID
//  // at runtime this 'path' is not used as an ezResource path/id, as would be common, but is used to look up the information
//  // directly from the compendium
//  {
//    for (auto pAssetInfo : allTsAssets)
//    {
//      const ezString& docPath = pAssetInfo->m_sDataDirRelativePath;
//      const ezUuid& docGuid = pAssetInfo->m_Info->m_DocumentID;
//
//      sFilename = ezPathUtils::GetFileName(docPath);
//
//      // TODO: handle filenameToSourceTsPath[sFilename] == "" case (log error)
//      compendium.m_AssetGuidToInfo[docGuid].m_sComponentTypeName = sFilename;
//      compendium.m_AssetGuidToInfo[docGuid].m_sComponentFilePath = filenameToSourceTsPath[sFilename];
//    }
//  }
//
//  {
//    ezDeferredFileWriter file;
//    file.SetOutput(sOutFile);
//
//    ezAssetFileHeader header;
//    header.SetFileHashAndVersion(1, 1);
//
//    EZ_SUCCEED_OR_RETURN(header.Write(file));
//
//    EZ_SUCCEED_OR_RETURN(compendium.Serialize(file));
//
//    EZ_SUCCEED_OR_RETURN(file.Close());
//  }
//
//  // write m_CheckedTsFiles cache
//  {
//    ezStringBuilder sFile = ezApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
//    sFile.AppendPath("LastDLangChanges.tmp");
//
//    ezFileWriter file;
//    if (file.Open(sFile).Succeeded())
//    {
//      EZ_SUCCEED_OR_RETURN(file.WriteMap(m_CheckedTsFiles));
//    }
//  }
//
//  return EZ_SUCCESS;
//}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDLangPreferences, 1, ezRTTIDefaultAllocator<ezDLangPreferences>)
{
  //EZ_BEGIN_PROPERTIES
  //{
  //  EZ_MEMBER_PROPERTY("CompileForSimulation", m_bAutoUpdateScriptsForSimulation),
  //  EZ_MEMBER_PROPERTY("CompileForPlayTheGame", m_bAutoUpdateScriptsForPlayTheGame),
  //}
  //EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezDLangPreferences::ezDLangPreferences()
  : ezPreferences(ezPreferences::Domain::Application, "DLang")
{
}
