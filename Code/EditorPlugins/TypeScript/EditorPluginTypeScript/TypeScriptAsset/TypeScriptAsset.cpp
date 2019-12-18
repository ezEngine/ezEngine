#include <EditorPluginTypeScriptPCH.h>

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

  ezStringBuilder sTsFileAbsPath;
  if (ezFileSystem::ResolvePath(sTsPath, &sTsFileAbsPath, nullptr).Failed())
    return;

  {
    QStringList args;

    for (const auto& dd : ezQtEditorApp::GetSingleton()->GetFileSystemConfig().m_DataDirs)
    {
      ezStringBuilder path;
      ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, path);

      args.append(QString::fromUtf8(path));
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
  ezStringBuilder sAbsPathToEzTs = ":project/TypeScript/ez";

  ezStringBuilder sScriptFilePath = szFile;
  sScriptFilePath.ChangeFileNameAndExtension("");

  ezStringBuilder sRelPathToEzTS = sAbsPathToEzTs;
  sRelPathToEzTS.MakeRelativeTo(sScriptFilePath);

  if (!sRelPathToEzTS.StartsWith("."))
  {
    sRelPathToEzTS.Prepend("./");
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

ezStatus ezTypeScriptAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag,
  const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader,
  ezBitflags<ezTransformFlags> transformFlags)
{
  EZ_SUCCEED_OR_RETURN(AutoGenerateVariablesCode());

  {
    ezTypeScriptAssetDocumentEvent e;
    e.m_Type = ezTypeScriptAssetDocumentEvent::Type::ScriptTransformed;
    e.m_pDocument = this;
    m_Events.Broadcast(e);
  }

  ezTypeScriptAssetDocumentManager* pAssMan = static_cast<ezTypeScriptAssetDocumentManager*>(GetAssetDocumentManager());
  pAssMan->GenerateScriptCompendium(transformFlags);

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

    tsWriteBack.WriteBytes(content.GetData(), content.GetElementCount());
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

    ezStringBuilder sTsPath(":project/", GetProperties()->m_sScriptFile);

    if (!ezFileSystem::ExistsFile(sTsPath))
    {
      CreateComponentFile(sTsPath);
    }
  }
}
