#include <EditorPluginTypeScriptPCH.h>

#include <Core/Graphics/Geometry.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/Util/MeshImportUtils.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAsset.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAssetManager.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAssetObjects.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Utilities/Progress.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <TypeScriptPlugin/Resources/JavaScriptResource.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTypeScriptAssetDocument, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezTypeScriptAssetDocument::ezTypeScriptAssetDocument(const char* szDocumentPath)
  : ezSimpleAssetDocument<ezTypeScriptAssetProperties>(szDocumentPath, ezAssetDocEngineConnection::Simple)
{
}

const char* ezTypeScriptAssetDocument::QueryAssetType() const
{
  return "TypeScript";
}

void ezTypeScriptAssetDocument::EditScript()
{
  ezStringBuilder sTsPath(":project/", GetProperties()->m_sScriptFile);

  if (GetProperties()->m_sScriptFile.IsEmpty())
    return;

  if (!ezFileSystem::ExistsFile(sTsPath))
  {
    CreateComponentFile(sTsPath);
  }

  ezStringBuilder sAbsPath;
  if (ezFileSystem::ResolvePath(sTsPath, &sAbsPath, nullptr).Failed())
    return;

  ezQtUiServices::OpenFileInDefaultProgram(sAbsPath);

  {
    ezTypeScriptAssetDocumentEvent e;
    e.m_Type = ezTypeScriptAssetDocumentEvent::Type::ScriptOpened;
    e.m_pDocument = this;
    m_Events.Broadcast(e);
  }
}

void ezTypeScriptAssetDocument::CreateComponentFile(const char* szFile)
{
  ezStringBuilder sAbsPathToEzTs = ":project/TypeScript/ez";

  ezStringBuilder sScriptFilePath = szFile;
  sScriptFilePath.ChangeFileNameAndExtension("");

  ezStringBuilder sRelPathToEzTS = sAbsPathToEzTs;
  sRelPathToEzTS.MakeRelativeTo(sScriptFilePath);

  if (!sRelPathToEzTS.StartsWith("."))
  {
    sRelPathToEzTS.Prepend("./");
  }

  const ezStringBuilder sComponentName = GetProperties()->m_sComponentName;

  if (sComponentName.IsEmpty())
    return;

  ezStringBuilder sContent;

  {
    ezFileReader fileIn;
    if (fileIn.Open(":plugins/TypeScript/NewComponent.ts").Succeeded())
    {
      sContent.ReadAll(fileIn);
      sContent.ReplaceAll("NewComponent", sComponentName.GetView());
      sContent.ReplaceAll("<PATH-TO-EZ-TS>", sRelPathToEzTS.GetView());
    }
  }

  {
    ezFileWriter file;
    if (file.Open(szFile).Succeeded())
    {
      file.WriteBytes(sContent.GetData(), sContent.GetElementCount());
    }
  }

  {
    ezTypeScriptAssetDocumentEvent e;
    e.m_Type = ezTypeScriptAssetDocumentEvent::Type::ScriptCreated;
    e.m_pDocument = this;
    m_Events.Broadcast(e);
  }
}

//////////////////////////////////////////////////////////////////////////

ezStatus ezTypeScriptAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag,
  const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader,
  ezBitflags<ezTransformFlags> transformFlags)
{
  ezStringBuilder sTsPath = GetProperties()->m_sScriptFile;
  sTsPath.ChangeFileExtension("ts");
  ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sTsPath);

  ezStringBuilder sTranspiledCode;
  ezUInt64 uiSourceHash = 0;
  if (static_cast<ezTypeScriptAssetDocumentManager*>(GetAssetDocumentManager())->GetTranspiler().TranspileFile(sTsPath, 0, sTranspiledCode, uiSourceHash).Failed())
  {
    return ezStatus("Transpiling from TypeScript to JavaScript failed.");
  }

  ezJavaScriptResourceDesc desc;
  desc.m_sComponentName = GetProperties()->m_sComponentName;
  desc.m_JsSource.SetCountUninitialized(sTranspiledCode.GetElementCount() + 1);

  ezMemoryUtils::RawByteCopy(desc.m_JsSource.GetData(), sTranspiledCode.GetData(), desc.m_JsSource.GetCount());

  EZ_SUCCEED_OR_RETURN(desc.Serialize(stream));

  return ezStatus(EZ_SUCCESS);
}

void ezTypeScriptAssetDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  SUPER::InitializeAfterLoading(bFirstTimeCreation);

  if (bFirstTimeCreation)
  {
    auto history = GetCommandHistory();
    history->StartTransaction("Initial Setup");

    if (GetProperties()->m_sComponentName.IsEmpty())
    {
      const ezString sCompName = ezPathUtils::GetFileName(GetDocumentPath());

      ezSetObjectPropertyCommand propCmd;
      propCmd.m_Object = GetPropertyObject()->GetGuid();
      propCmd.m_sProperty = "ComponentName";
      propCmd.m_NewValue = sCompName;
      EZ_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
    }

    if (GetProperties()->m_sScriptFile.IsEmpty())
    {
      ezStringBuilder sDefaultFile = GetDocumentPath();
      sDefaultFile.ChangeFileExtension("ts");
      ezQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sDefaultFile);

      ezSetObjectPropertyCommand propCmd;
      propCmd.m_Object = GetPropertyObject()->GetGuid();
      propCmd.m_sProperty = "ScriptFile";
      propCmd.m_NewValue = ezString(sDefaultFile);
      EZ_VERIFY(history->AddCommand(propCmd).m_Result.Succeeded(), "AddCommand failed");
    }

    history->FinishTransaction();

    ezStringBuilder sTsPath(":project/", GetProperties()->m_sScriptFile);

    if (!ezFileSystem::ExistsFile(sTsPath))
    {
      CreateComponentFile(sTsPath);
    }
  }
}
