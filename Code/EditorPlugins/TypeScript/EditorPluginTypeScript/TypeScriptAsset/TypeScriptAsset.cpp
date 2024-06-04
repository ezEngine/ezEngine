#include <EditorPluginTypeScript/EditorPluginTypeScriptPCH.h>

#include <Core/Graphics/Geometry.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/GUI/ExposedParameters.h>
#include <EditorPluginAssets/Util/MeshImportUtils.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAsset.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAssetManager.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAssetObjects.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Utilities/Progress.h>
#include <ToolsFoundation/Command/TreeCommands.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTypeScriptAssetDocument, 2, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezTypeScriptAssetDocument::ezTypeScriptAssetDocument(ezStringView sDocumentPath)
  : ezSimpleAssetDocument<ezTypeScriptAssetProperties>(sDocumentPath, ezAssetDocEngineConnection::None)
{
}

void ezTypeScriptAssetDocument::EditScript()
{
  ezStringBuilder sTsPath(GetProperties()->m_sScriptFile);

  if (GetProperties()->m_sScriptFile.IsEmpty())
    return;

  if (!ezFileSystem::ExistsFile(sTsPath))
  {
    CreateComponentFile(sTsPath);
  }

  ezStringBuilder sTsFileAbsPath;
  if (ezFileSystem::ResolvePath(sTsPath, &sTsFileAbsPath, nullptr).Failed())
    return;

  static_cast<ezTypeScriptAssetDocumentManager*>(GetDocumentManager())->SetupProjectForTypeScript(false);

  CreateTsConfigFiles();

  {
    QStringList args;

    for (const auto& dd : ezQtEditorApp::GetSingleton()->GetFileSystemConfig().m_DataDirs)
    {
      ezStringBuilder path;
      ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, path).IgnoreResult();

      args.append(QString::fromUtf8(path, path.GetElementCount()));
    }

    args.append(sTsFileAbsPath.GetData());

    if (ezQtUiServices::OpenInVsCode(args).Failed())
    {
      // try again with a different program
      ezQtUiServices::OpenFileInDefaultProgram(sTsFileAbsPath);
    }
  }

  {
    ezTypeScriptAssetDocumentEvent e;
    e.m_Type = ezTypeScriptAssetDocumentEvent::Type::ScriptOpened;
    e.m_pDocument = this;
    m_Events.Broadcast(e);
  }
}

void ezTypeScriptAssetDocument::CreateComponentFile(const char* szFile)
{
  ezStringBuilder sScriptFile = szFile;

  {
    const ezDataDirectoryInfo* pDataDir = nullptr;
    if (ezFileSystem::ResolvePath(GetDocumentPath(), nullptr, nullptr, &pDataDir).Failed())
      return;

    sScriptFile.Prepend(pDataDir->m_pDataDirType->GetRedirectedDataDirectoryPath(), "/");
  }

  const ezStringBuilder sComponentName = ezPathUtils::GetFileName(GetDocumentPath());

  if (sComponentName.IsEmpty())
    return;

  ezStringBuilder sContent;

  {
    ezFileReader fileIn;
    if (fileIn.Open(":plugins/TypeScript/NewComponent.ts").Succeeded())
    {
      sContent.ReadAll(fileIn);
      sContent.ReplaceAll("NewComponent", sComponentName.GetView());
      sContent.ReplaceAll("<PATH-TO-EZ-TS>", "TypeScript/ez");
    }
  }

  {
    ezFileWriter file;
    if (file.Open(sScriptFile).Succeeded())
    {
      file.WriteBytes(sContent.GetData(), sContent.GetElementCount()).IgnoreResult();
    }
  }

  {
    ezTypeScriptAssetDocumentEvent e;
    e.m_Type = ezTypeScriptAssetDocumentEvent::Type::ScriptCreated;
    e.m_pDocument = this;
    m_Events.Broadcast(e);
  }
}

void ezTypeScriptAssetDocument::CreateTsConfigFiles()
{
  for (const auto& dd : ezQtEditorApp::GetSingleton()->GetFileSystemConfig().m_DataDirs)
  {
    if (dd.m_sRootName.IsEqual_NoCase("BASE"))
      continue;

    ezStringBuilder path;
    ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, path).IgnoreResult();
    path.MakeCleanPath();

    CreateTsConfigFile(path).IgnoreResult();
  }
}

ezResult ezTypeScriptAssetDocument::CreateTsConfigFile(const char* szDirectory)
{
  ezStringBuilder sTsConfig;
  ezStringBuilder sTmp;

  for (ezUInt32 iPlus1 = ezQtEditorApp::GetSingleton()->GetFileSystemConfig().m_DataDirs.GetCount(); iPlus1 > 0; --iPlus1)
  {
    const auto& dd = ezQtEditorApp::GetSingleton()->GetFileSystemConfig().m_DataDirs[iPlus1 - 1];

    ezStringBuilder path;
    EZ_SUCCEED_OR_RETURN(ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, path));
    path.MakeCleanPath();
    path.AppendPath("*");

    sTmp.AppendWithSeparator(", ", "\"", path, "\"");
  }

  sTsConfig.SetFormat(
    R"({
  "compilerOptions": {
    "target": "es5",
    "baseUrl": "",
    "paths": {
      "*": [{0}]
    }    
  }
}
)",
    sTmp);


  {
    sTmp = szDirectory;
    sTmp.AppendPath("tsconfig.json");

    ezFileWriter file;
    EZ_SUCCEED_OR_RETURN(file.Open(sTmp));
    EZ_SUCCEED_OR_RETURN(file.WriteBytes(sTsConfig.GetData(), sTsConfig.GetElementCount()));
  }

  {
    sTmp = szDirectory;
    sTmp.AppendPath(".gitignore");

    ezQtUiServices::AddToGitIgnore(sTmp, "tsconfig.json").IgnoreResult();
  }

  return EZ_SUCCESS;
}

void ezTypeScriptAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  ezExposedParameters* pExposedParams = EZ_DEFAULT_NEW(ezExposedParameters);

  {
    for (const auto& p : GetProperties()->m_NumberParameters)
    {
      ezExposedParameter* param = EZ_DEFAULT_NEW(ezExposedParameter);
      pExposedParams->m_Parameters.PushBack(param);
      param->m_sName = p.m_sName;
      param->m_DefaultValue = p.m_DefaultValue;
    }

    for (const auto& p : GetProperties()->m_BoolParameters)
    {
      ezExposedParameter* param = EZ_DEFAULT_NEW(ezExposedParameter);
      pExposedParams->m_Parameters.PushBack(param);
      param->m_sName = p.m_sName;
      param->m_DefaultValue = p.m_DefaultValue;
    }

    for (const auto& p : GetProperties()->m_StringParameters)
    {
      ezExposedParameter* param = EZ_DEFAULT_NEW(ezExposedParameter);
      pExposedParams->m_Parameters.PushBack(param);
      param->m_sName = p.m_sName;
      param->m_DefaultValue = p.m_DefaultValue;
    }

    for (const auto& p : GetProperties()->m_Vec3Parameters)
    {
      ezExposedParameter* param = EZ_DEFAULT_NEW(ezExposedParameter);
      pExposedParams->m_Parameters.PushBack(param);
      param->m_sName = p.m_sName;
      param->m_DefaultValue = p.m_DefaultValue;
    }

    for (const auto& p : GetProperties()->m_ColorParameters)
    {
      ezExposedParameter* param = EZ_DEFAULT_NEW(ezExposedParameter);
      pExposedParams->m_Parameters.PushBack(param);
      param->m_sName = p.m_sName;
      param->m_DefaultValue = p.m_DefaultValue;
    }
  }

  // Info takes ownership of meta data.
  pInfo->m_MetaInfo.PushBack(pExposedParams);
}

ezTransformStatus ezTypeScriptAssetDocument::InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  EZ_SUCCEED_OR_RETURN(ValidateScriptCode());
  EZ_SUCCEED_OR_RETURN(AutoGenerateVariablesCode());

  ezStringBuilder sTypeName = ezPathUtils::GetFileName(GetDocumentPath());
  stream << sTypeName;

  const ezUuid& docGuid = GetGuid();
  stream << docGuid;

  {
    ezTypeScriptAssetDocumentEvent e;
    e.m_Type = ezTypeScriptAssetDocumentEvent::Type::ScriptTransformed;
    e.m_pDocument = this;
    m_Events.Broadcast(e);
  }

  ezTypeScriptAssetDocumentManager* pAssMan = static_cast<ezTypeScriptAssetDocumentManager*>(GetAssetDocumentManager());
  EZ_SUCCEED_OR_RETURN(pAssMan->GenerateScriptCompendium(transformFlags));

  return ezTransformStatus();
}

ezStatus ezTypeScriptAssetDocument::ValidateScriptCode()
{
  ezStringBuilder sTsDocPath = GetProperties()->m_sScriptFile;
  ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sTsDocPath);

  ezStringBuilder content;

  // read typescript file content
  {
    ezFileReader tsFile;
    if (tsFile.Open(sTsDocPath).Failed())
    {
      return ezStatus(ezFmt("Could not read .ts file '{}'", GetProperties()->m_sScriptFile));
    }

    content.ReadAll(tsFile);
  }

  // validate that the class with the correct name exists
  {
    ezStringBuilder sClass;
    sClass = "class ";
    sClass.Append(sTsDocPath.GetFileName());
    sClass.Append(" extends");

    if (content.FindSubString(sClass) == nullptr)
    {
      return ezStatus(ezFmt("Sub-string '{}' not found. Class name may be incorrect.", sClass));
    }
  }

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezTypeScriptAssetDocument::AutoGenerateVariablesCode()
{
  ezStringBuilder sTsDocPath = GetProperties()->m_sScriptFile;
  ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sTsDocPath);

  ezStringBuilder content;

  // read typescript file content
  {
    ezFileReader tsFile;
    if (tsFile.Open(sTsDocPath).Failed())
    {
      return ezStatus(ezFmt("Could not read .ts file '{}'", GetProperties()->m_sScriptFile));
    }

    content.ReadAll(tsFile);
  }

  const char* szTagBegin = "/* BEGIN AUTO-GENERATED: VARIABLES */";
  const char* szTagEnd = "/* END AUTO-GENERATED: VARIABLES */";

  const char* szBeginAG = content.FindSubString(szTagBegin);

  if (szBeginAG == nullptr)
  {
    return ezStatus(ezFmt("'{}' tag is missing or corrupted.", szTagBegin));
  }

  const char* szEndAG = content.FindSubString(szTagEnd, szBeginAG);


  if (szEndAG == nullptr)
  {
    return ezStatus(ezFmt("'{}' tag is missing or corrupted.", szTagEnd));
  }

  ezStringBuilder sAutoGen;

  // create code for exposed parameters
  {
    for (const auto& p : GetProperties()->m_NumberParameters)
    {
      sAutoGen.AppendFormat("    {}: number = {};\n", p.m_sName, p.m_DefaultValue);
    }
    for (const auto& p : GetProperties()->m_BoolParameters)
    {
      sAutoGen.AppendFormat("    {}: boolean = {};\n", p.m_sName, p.m_DefaultValue);
    }
    for (const auto& p : GetProperties()->m_StringParameters)
    {
      sAutoGen.AppendFormat("    {}: string = \"{}\";\n", p.m_sName, p.m_DefaultValue);
    }
    for (const auto& p : GetProperties()->m_Vec3Parameters)
    {
      sAutoGen.AppendFormat("    {}: ez.Vec3 = new ez.Vec3({}, {}, {});\n", p.m_sName, p.m_DefaultValue.x, p.m_DefaultValue.y, p.m_DefaultValue.z);
    }
    for (const auto& p : GetProperties()->m_ColorParameters)
    {
      sAutoGen.AppendFormat("    {}: ez.Color = new ez.Color({}, {}, {}, {});\n", p.m_sName, p.m_DefaultValue.r, p.m_DefaultValue.g, p.m_DefaultValue.b, p.m_DefaultValue.a);
    }
  }

  // write back the modified file
  {
    sAutoGen.Prepend(szTagBegin, "\n");
    sAutoGen.Append("    ", szTagEnd);

    content.ReplaceSubString(szBeginAG, szEndAG + ezStringUtils::GetStringElementCount(szTagEnd), sAutoGen);

    ezFileWriter tsWriteBack;
    if (tsWriteBack.Open(sTsDocPath).Failed())
    {
      return ezStatus(ezFmt("Could not update .ts file '{}'", GetProperties()->m_sScriptFile));
    }

    EZ_SUCCEED_OR_RETURN(tsWriteBack.WriteBytes(content.GetData(), content.GetElementCount()));
  }

  return ezStatus(EZ_SUCCESS);
}

void ezTypeScriptAssetDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  SUPER::InitializeAfterLoading(bFirstTimeCreation);

  if (bFirstTimeCreation)
  {
    auto history = GetCommandHistory();
    history->StartTransaction("Initial Setup");

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

    const ezString& sTsPath = GetProperties()->m_sScriptFile;

    if (!ezFileSystem::ExistsFile(sTsPath))
    {
      CreateComponentFile(sTsPath);
    }
  }
}
