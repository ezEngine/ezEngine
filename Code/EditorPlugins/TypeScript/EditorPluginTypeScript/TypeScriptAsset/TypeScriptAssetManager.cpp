#include <EditorPluginTypeScript/EditorPluginTypeScriptPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAsset.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAssetManager.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAssetWindow.moc.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <Foundation/Utilities/Progress.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
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

  m_DocTypeDesc.m_sDocumentTypeName = "TypeScript";
  m_DocTypeDesc.m_sFileExtension = "ezTypeScriptAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/TypeScript.svg";
  m_DocTypeDesc.m_sAssetCategory = "Scripting";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezTypeScriptAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Code_TypeScript");

  // Typescript doesn't fully work with the new scripting infrastructure yet. Uncomment at your own risk.
  // m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_ScriptClass");

  m_DocTypeDesc.m_sResourceFileExtension = "ezBinTypeScript";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::None;

  ezQtImageCache::GetSingleton()->RegisterTypeImage("TypeScript", QPixmap(":/AssetIcons/TypeScript.svg"));

  ezToolsProject::s_Events.AddEventHandler(ezMakeDelegate(&ezTypeScriptAssetDocumentManager::ToolsProjectEventHandler, this));

  ezGameObjectDocument::s_GameObjectDocumentEvents.AddEventHandler(ezMakeDelegate(&ezTypeScriptAssetDocumentManager::GameObjectDocumentEventHandler, this));

  // make sure the preferences exist
  ezPreferences::QueryPreferences<ezTypeScriptPreferences>();
}

ezTypeScriptAssetDocumentManager::~ezTypeScriptAssetDocumentManager()
{
  ShutdownTranspiler();

  ezToolsProject::s_Events.RemoveEventHandler(ezMakeDelegate(&ezTypeScriptAssetDocumentManager::ToolsProjectEventHandler, this));

  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezTypeScriptAssetDocumentManager::OnDocumentManagerEvent, this));

  ezGameObjectDocument::s_GameObjectDocumentEvents.RemoveEventHandler(ezMakeDelegate(&ezTypeScriptAssetDocumentManager::GameObjectDocumentEventHandler, this));
}

void ezTypeScriptAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezTypeScriptAssetDocument>())
      {
        new ezQtTypeScriptAssetDocumentWindow(static_cast<ezTypeScriptAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void ezTypeScriptAssetDocumentManager::InternalCreateDocument(ezStringView sDocumentTypeName, ezStringView sPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext)
{
  out_pDocument = new ezTypeScriptAssetDocument(sPath);
}

void ezTypeScriptAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}

void ezTypeScriptAssetDocumentManager::ToolsProjectEventHandler(const ezToolsProjectEvent& e)
{
  if (e.m_Type == ezToolsProjectEvent::Type::ProjectOpened)
  {
    InitializeTranspiler();
  }

  if (e.m_Type == ezToolsProjectEvent::Type::ProjectClosing)
  {
    ShutdownTranspiler();
  }
}

void ezTypeScriptAssetDocumentManager::GameObjectDocumentEventHandler(const ezGameObjectDocumentEvent& e)
{
  switch (e.m_Type)
  {
    case ezGameObjectDocumentEvent::Type::GameMode_StartingSimulate:
    {
      if (ezPreferences::QueryPreferences<ezTypeScriptPreferences>()->m_bAutoUpdateScriptsForSimulation)
      {
        GenerateScriptCompendium(ezTransformFlags::Default).IgnoreResult();
      }
      break;
    }

    case ezGameObjectDocumentEvent::Type::GameMode_StartingPlay:
    case ezGameObjectDocumentEvent::Type::GameMode_StartingExternal:
    {
      if (ezPreferences::QueryPreferences<ezTypeScriptPreferences>()->m_bAutoUpdateScriptsForPlayTheGame)
      {
        GenerateScriptCompendium(ezTransformFlags::Default).IgnoreResult();
      }
      break;
    }

    default:
      break;
  }
}

void ezTypeScriptAssetDocumentManager::ModifyTsBeforeTranspilation(ezStringBuilder& content)
{
  const char* szTagBegin = "EZ_DECLARE_MESSAGE_TYPE;";

  ezUInt32 uiContinueAfterOffset = 0;
  ezStringBuilder sAutoGen;

  while (true)
  {

    const char* szBeginAG = content.FindSubString(szTagBegin, content.GetData() + uiContinueAfterOffset);

    if (szBeginAG == nullptr)
    {
      break;
    }

    const char* szClassAG = content.FindLastSubString("class", szBeginAG);
    if (szClassAG == nullptr)
    {
      // return ezStatus(ezFmt("'{}' tag is incorrectly placed.", szBeginAG));
      return;
    }

    ezUInt32 uiTypeNameHash = 0;

    {
      ezStringView classNameView(szClassAG + 5, szBeginAG);
      classNameView.Trim(" \t\n\r");

      ezStringBuilder sClassName;

      ezStringIterator classNameIt = classNameView.GetIteratorFront();
      while (classNameIt.IsValid() && !ezStringUtils::IsIdentifierDelimiter_C_Code(classNameIt.GetCharacter()))
      {
        sClassName.Append(classNameIt.GetCharacter());
        ++classNameIt;
      }

      if (sClassName.IsEmpty())
      {
        // return ezStatus("Message class name not found.");
        return;
      }

      uiTypeNameHash = ezHashingUtils::StringHashTo32(ezHashingUtils::StringHash(sClassName.GetData()));
    }

    sAutoGen.SetFormat("public static GetTypeNameHash(): number { return {0}; }\nconstructor() { super(); this.TypeNameHash = {0}; }\n", uiTypeNameHash);

    uiContinueAfterOffset = szBeginAG - content.GetData();

    content.ReplaceSubString(szBeginAG, szBeginAG + ezStringUtils::GetStringElementCount(szTagBegin), sAutoGen);
  }
}

void ezTypeScriptAssetDocumentManager::InitializeTranspiler()
{
  if (m_bTranspilerLoaded)
    return;

  m_bTranspilerLoaded = true;

  if (ezFileSystem::FindDataDirectoryWithRoot("TypeScript") == nullptr)
  {
    ezFileSystem::AddDataDirectory(">sdk/Data/Tools/ezEditor/Typescript", "TypeScript", "TypeScript").IgnoreResult();
  }

  m_Transpiler.SetOutputFolder(":project/AssetCache/Temp");
  m_Transpiler.StartLoadTranspiler();
  m_Transpiler.SetModifyTsBeforeTranspilationCallback(&ezTypeScriptAssetDocumentManager::ModifyTsBeforeTranspilation);
}

void ezTypeScriptAssetDocumentManager::ShutdownTranspiler()
{
  if (!m_bTranspilerLoaded)
    return;

  m_bTranspilerLoaded = false;

  m_Transpiler.FinishLoadTranspiler();
}

void ezTypeScriptAssetDocumentManager::SetupProjectForTypeScript(bool bForce)
{
  if (m_bProjectSetUp && !bForce)
    return;

  m_bProjectSetUp = true;

  if (ezTypeScriptBinding::SetupProjectCode().Failed())
  {
    ezLog::Error("Could not setup Typescript data in project directory");
    return;
  }
}

ezResult ezTypeScriptAssetDocumentManager::GenerateScriptCompendium(ezBitflags<ezTransformFlags> transformFlags)
{
  EZ_LOG_BLOCK("Generating Script Compendium");

  ezHybridArray<ezAssetInfo*, 256> allTsAssets;

  // keep this locked until the end of the function
  ezAssetCurator::ezLockedSubAssetTable AllAssetsLocked = ezAssetCurator::GetSingleton()->GetKnownSubAssets();
  const ezHashTable<ezUuid, ezSubAsset>& AllAssets = *AllAssetsLocked;

  for (auto it = AllAssets.GetIterator(); it.IsValid(); ++it)
  {
    const ezSubAsset* pSub = &it.Value();

    if (pSub->m_pAssetInfo->GetManager() == this)
    {
      allTsAssets.PushBack(pSub->m_pAssetInfo);
    }
  }

  if (allTsAssets.IsEmpty())
  {
    ezLog::Debug("Skipping script compendium creation - no TypeScript assets in project.");
    return EZ_SUCCESS;
  }


  SetupProjectForTypeScript(false);

  // read m_CheckedTsFiles cache
  if (m_CheckedTsFiles.IsEmpty())
  {
    ezStringBuilder sFile = ezApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
    sFile.AppendPath("LastTypeScriptChanges.tmp");

    ezFileReader file;
    if (file.Open(sFile).Succeeded())
    {
      file.ReadMap(m_CheckedTsFiles).IgnoreResult();
    }
  }

  ezMap<ezString, ezUInt32> relPathToDataDirIdx;

  ezScriptCompendiumResourceDesc compendium;
  bool bAnythingNew = false;

  for (ezUInt32 ddIdx = 0; ddIdx < ezFileSystem::GetNumDataDirectories(); ++ddIdx)
  {
    ezStringBuilder sDataDirPath = ezFileSystem::GetDataDirectory(ddIdx)->GetRedirectedDataDirectoryPath();
    sDataDirPath.MakeCleanPath();

    if (!sDataDirPath.IsAbsolutePath())
      continue;

    ezFileSystemIterator fsIt;
    fsIt.StartSearch(sDataDirPath, ezFileSystemIteratorFlags::ReportFilesRecursive);

    ezStringBuilder sTsFilePath;
    for (; fsIt.IsValid(); fsIt.Next())
    {
      if (!fsIt.GetStats().m_sName.EndsWith_NoCase(".ts"))
        continue;

      fsIt.GetStats().GetFullPath(sTsFilePath);

      sTsFilePath.MakeRelativeTo(sDataDirPath).IgnoreResult();

      relPathToDataDirIdx[sTsFilePath] = ddIdx;

      compendium.m_PathToSource.Insert(sTsFilePath, ezString());

      ezTimestamp& lastModification = m_CheckedTsFiles[sTsFilePath];

      if (!lastModification.Compare(fsIt.GetStats().m_LastModificationTime, ezTimestamp::CompareMode::FileTimeEqual))
      {
        bAnythingNew = true;
        lastModification = fsIt.GetStats().m_LastModificationTime;
      }
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
    ezStringBuilder sOutputFolder;

    ezStringBuilder sTranspiledJs;
    for (auto it : compendium.m_PathToSource)
    {
      if (!progress.BeginNextStep(it.Key()))
        return EZ_FAILURE;

      sOutputFolder = ezFileSystem::GetDataDirectory(relPathToDataDirIdx[it.Key()])->GetRedirectedDataDirectoryPath();
      sOutputFolder.MakeCleanPath();
      sOutputFolder.AppendPath("AssetCache/Temp");
      m_Transpiler.SetOutputFolder(sOutputFolder);

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
    for (auto pAssetInfo : allTsAssets)
    {
      const ezString& docPath = pAssetInfo->m_Path.GetDataDirParentRelativePath();
      const ezUuid& docGuid = pAssetInfo->m_Info->m_DocumentID;

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

    EZ_SUCCEED_OR_RETURN(header.Write(file));

    EZ_SUCCEED_OR_RETURN(compendium.Serialize(file));

    EZ_SUCCEED_OR_RETURN(file.Close());
  }

  // write m_CheckedTsFiles cache
  {
    ezStringBuilder sFile = ezApplicationServices::GetSingleton()->GetProjectPreferencesFolder();
    sFile.AppendPath("LastTypeScriptChanges.tmp");

    ezFileWriter file;
    if (file.Open(sFile).Succeeded())
    {
      EZ_SUCCEED_OR_RETURN(file.WriteMap(m_CheckedTsFiles));
    }
  }

  return EZ_SUCCESS;
}

ezStatus ezTypeScriptAssetDocumentManager::GetAdditionalOutputs(ezDynamicArray<ezString>& ref_files)
{
  if (GenerateScriptCompendium(ezTransformFlags::Default).Failed())
    return ezStatus("Failed to build TypeScript compendium.");

  ref_files.PushBack("AssetCache/Common/Scripts.ezScriptCompendium");

  return ezStatus(EZ_SUCCESS);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTypeScriptPreferences, 1, ezRTTIDefaultAllocator<ezTypeScriptPreferences>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("CompileForSimulation", m_bAutoUpdateScriptsForSimulation),
    EZ_MEMBER_PROPERTY("CompileForPlayTheGame", m_bAutoUpdateScriptsForPlayTheGame),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezTypeScriptPreferences::ezTypeScriptPreferences()
  : ezPreferences(ezPreferences::Domain::Application, "TypeScript")
{
}
