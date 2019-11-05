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

ezResult ezTypeScriptAssetDocumentManager::GenerateScriptCompendium()
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

  if (bAnythingNew == false && ezFileSystem::ExistsFile(sOutFile))
    return EZ_SUCCESS;

  ezProgressRange progress("Transpiling Scripts", compendium.m_PathToSource.GetCount(), true);

  // remove the output file, so that if anything fails from here on out, it will be re-generated next time
  ezFileSystem::DeleteFile(sOutFile);

  // TODO: could multi-thread this, if we had multiple transpilers loaded
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
