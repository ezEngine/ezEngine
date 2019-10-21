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
  ezStringBuilder sTsPath = GetDocumentPath();
  sTsPath.ChangeFileExtension("ts");

  if (!ezFileSystem::ExistsFile(sTsPath))
  {
    CreateComponentFile(sTsPath);
  }

  ezStringBuilder sAbsPath;
  if (ezFileSystem::ResolvePath(sTsPath, &sAbsPath, nullptr).Failed())
    return;

  ezQtUiServices::OpenFileInDefaultProgram(sAbsPath);
}

void ezTypeScriptAssetDocument::CreateComponentFile(const char* szFile)
{
  ezStringBuilder sAbsPathToEzTs;
  if (ezFileSystem::ResolvePath(":project/TypeScript/ez", &sAbsPathToEzTs, nullptr).Failed())
  {
    ezLog::Error("TypeScript data not found in ':project/TypeScript'");
    return;
  }

  ezStringBuilder sScriptFilePath = szFile;
  sScriptFilePath.ChangeFileNameAndExtension("");

  ezStringBuilder sRelPathToEzTS = sAbsPathToEzTs;
  sRelPathToEzTS.MakeRelativeTo(sScriptFilePath);

  if (!sRelPathToEzTS.StartsWith("."))
  {
    sRelPathToEzTS.Prepend("./");
  }

  const ezStringBuilder sComponentName = ezPathUtils::GetFileName(szFile);

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

  ezFileWriter file;
  if (file.Open(szFile).Succeeded())
  {
    file.WriteBytes(sContent.GetData(), sContent.GetElementCount());
  }
}

//////////////////////////////////////////////////////////////////////////

ezStatus ezTypeScriptAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag,
  const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader,
  ezBitflags<ezTransformFlags> transformFlags)
{

  return ezStatus(EZ_SUCCESS);
}
