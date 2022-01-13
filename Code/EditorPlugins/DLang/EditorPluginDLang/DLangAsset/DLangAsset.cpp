#include <EditorPluginDLang/EditorPluginDLangPCH.h>

#include <Core/Graphics/Geometry.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/GUI/ExposedParameters.h>
#include <EditorPluginAssets/Util/MeshImportUtils.h>
#include <EditorPluginDLang/DLangAsset/DLangAsset.h>
#include <EditorPluginDLang/DLangAsset/DLangAssetManager.h>
#include <EditorPluginDLang/DLangAsset/DLangAssetObjects.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Utilities/Progress.h>
#include <ToolsFoundation/Command/TreeCommands.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDLangAssetDocument, 2, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezDLangAssetDocument::ezDLangAssetDocument(const char* szDocumentPath)
  : ezSimpleAssetDocument<ezDLangAssetProperties>(szDocumentPath, ezAssetDocEngineConnection::None)
{
}

void ezDLangAssetDocument::EditScript()
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

  //static_cast<ezDLangAssetDocumentManager*>(GetDocumentManager())->SetupProjectForDLang(false);

  {
    QStringList args;

    for (const auto& dd : ezQtEditorApp::GetSingleton()->GetFileSystemConfig().m_DataDirs)
    {
      ezStringBuilder path;
      ezFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, path).IgnoreResult();

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
    ezDLangAssetDocumentEvent e;
    e.m_Type = ezDLangAssetDocumentEvent::Type::ScriptOpened;
    e.m_pDocument = this;
    m_Events.Broadcast(e);
  }
}

void ezDLangAssetDocument::CreateComponentFile(const char* szFile)
{
  ezStringBuilder sScriptFile = szFile;

  {
    ezDataDirectoryType* pDataDir = nullptr;
    if (ezFileSystem::ResolvePath(GetDocumentPath(), nullptr, nullptr, &pDataDir).Failed())
      return;

    sScriptFile.Prepend(pDataDir->GetRedirectedDataDirectoryPath(), "/");
  }

  const ezStringBuilder sComponentName = ezPathUtils::GetFileName(GetDocumentPath());

  if (sComponentName.IsEmpty())
    return;

  ezStringBuilder sContent;

  {
    ezFileReader fileIn;
    if (fileIn.Open(":plugins/DLang/Templates/NewComponent.d").Succeeded())
    {
      sContent.ReadAll(fileIn);
      sContent.ReplaceAll("NewComponent", sComponentName.GetView());
      //sContent.ReplaceAll("<PATH-TO-EZ-TS>", "DLang/ez");
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
    ezDLangAssetDocumentEvent e;
    e.m_Type = ezDLangAssetDocumentEvent::Type::ScriptCreated;
    e.m_pDocument = this;
    m_Events.Broadcast(e);
  }
}

void ezDLangAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const
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

ezStatus ezDLangAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  EZ_SUCCEED_OR_RETURN(ValidateDLangCode());
  //EZ_SUCCEED_OR_RETURN(AutoGenerateVariablesCode());

  {
    ezDLangAssetDocumentEvent e;
    e.m_Type = ezDLangAssetDocumentEvent::Type::ScriptTransformed;
    e.m_pDocument = this;
    m_Events.Broadcast(e);
  }

  //ezDLangAssetDocumentManager* pAssMan = static_cast<ezDLangAssetDocumentManager*>(GetAssetDocumentManager());
  //EZ_SUCCEED_OR_RETURN(pAssMan->GenerateScriptCompendium(transformFlags));

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezDLangAssetDocument::ValidateDLangCode()
{
  ezStringBuilder sTsDocPath = GetProperties()->m_sScriptFile;
  ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sTsDocPath);

  ezStringBuilder content;

  // read file content
  {
    ezFileReader tsFile;
    if (tsFile.Open(sTsDocPath).Failed())
    {
      return ezStatus(ezFmt("Couldn't read file '{}'", GetProperties()->m_sScriptFile));
    }

    content.ReadAll(tsFile);
  }

  // validate that the class with the correct name exists
  {
    ezStringBuilder sClass;
    sClass = "class ";
    sClass.Append(sTsDocPath.GetFileName());
    //sClass.Append(" extends");

    //if (content.FindSubString(sClass) == nullptr)
    //{
    //  return ezStatus(ezFmt("Sub-string '{}' not found. Class name may be incorrect.", sClass));
    //}
  }

  return ezStatus(EZ_SUCCESS);
}

//ezStatus ezDLangAssetDocument::AutoGenerateVariablesCode()
//{
//  ezStringBuilder sTsDocPath = GetProperties()->m_sScriptFile;
//  ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sTsDocPath);
//
//  ezStringBuilder content;
//
//  // read typescript file content
//  {
//    ezFileReader tsFile;
//    if (tsFile.Open(sTsDocPath).Failed())
//    {
//      return ezStatus(ezFmt("Could not read .ts file '{}'", GetProperties()->m_sScriptFile));
//    }
//
//    content.ReadAll(tsFile);
//  }
//
//  const char* szTagBegin = "/* BEGIN AUTO-GENERATED: VARIABLES */";
//  const char* szTagEnd = "/* END AUTO-GENERATED: VARIABLES */";
//
//  const char* szBeginAG = content.FindSubString(szTagBegin);
//
//  if (szBeginAG == nullptr)
//  {
//    return ezStatus(ezFmt("'{}' tag is missing or corrupted.", szTagBegin));
//  }
//
//  const char* szEndAG = content.FindSubString(szTagEnd, szBeginAG);
//
//
//  if (szEndAG == nullptr)
//  {
//    return ezStatus(ezFmt("'{}' tag is missing or corrupted.", szTagEnd));
//  }
//
//  ezStringBuilder sAutoGen;
//
//  // create code for exposed parameters
//  {
//    for (const auto& p : GetProperties()->m_NumberParameters)
//    {
//      sAutoGen.AppendFormat("    {}: number = {};\n", p.m_sName, p.m_DefaultValue);
//    }
//    for (const auto& p : GetProperties()->m_BoolParameters)
//    {
//      sAutoGen.AppendFormat("    {}: boolean = {};\n", p.m_sName, p.m_DefaultValue);
//    }
//    for (const auto& p : GetProperties()->m_StringParameters)
//    {
//      sAutoGen.AppendFormat("    {}: string = \"{}\";\n", p.m_sName, p.m_DefaultValue);
//    }
//    for (const auto& p : GetProperties()->m_Vec3Parameters)
//    {
//      sAutoGen.AppendFormat("    {}: ez.Vec3 = new ez.Vec3({}, {}, {});\n", p.m_sName, p.m_DefaultValue.x, p.m_DefaultValue.y, p.m_DefaultValue.z);
//    }
//    for (const auto& p : GetProperties()->m_ColorParameters)
//    {
//      sAutoGen.AppendFormat("    {}: ez.Color = new ez.Color({}, {}, {}, {});\n", p.m_sName, p.m_DefaultValue.r, p.m_DefaultValue.g, p.m_DefaultValue.b, p.m_DefaultValue.a);
//    }
//  }
//
//  // write back the modified file
//  {
//    sAutoGen.Prepend(szTagBegin, "\n");
//    sAutoGen.Append("    ", szTagEnd);
//
//    content.ReplaceSubString(szBeginAG, szEndAG + ezStringUtils::GetStringElementCount(szTagEnd), sAutoGen);
//
//    ezFileWriter tsWriteBack;
//    if (tsWriteBack.Open(sTsDocPath).Failed())
//    {
//      return ezStatus(ezFmt("Could not update .ts file '{}'", GetProperties()->m_sScriptFile));
//    }
//
//    EZ_SUCCEED_OR_RETURN(tsWriteBack.WriteBytes(content.GetData(), content.GetElementCount()));
//  }
//
//  return ezStatus(EZ_SUCCESS);
//}

void ezDLangAssetDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  SUPER::InitializeAfterLoading(bFirstTimeCreation);

  if (bFirstTimeCreation)
  {
    auto history = GetCommandHistory();
    history->StartTransaction("Initial Setup");

    if (GetProperties()->m_sScriptFile.IsEmpty())
    {
      ezStringBuilder sDefaultFile = GetDocumentPath();
      sDefaultFile.ChangeFileExtension("d");
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
