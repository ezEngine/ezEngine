#include <EditorPluginRmlUi/EditorPluginRmlUiPCH.h>

#include <EditorPluginRmlUi/RmlUiAsset/RmlUiAsset.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <ToolsFoundation/Project/ToolsProject.h>

ezStringView FindNextHREF(ezStringView& ref_sRml)
{
  const char* szCurrent = ref_sRml.FindSubString("href");
  if (szCurrent == nullptr)
    return ezStringView();

  const char* szStart = nullptr;
  const char* szEnd = nullptr;
  while (*szCurrent != '\0')
  {
    if (*szCurrent == '\"')
    {
      if (szStart == nullptr)
      {
        szStart = szCurrent + 1;
      }
      else
      {
        szEnd = szCurrent;
        break;
      }
    }

    ++szCurrent;
  }

  if (szStart != nullptr && szEnd != nullptr)
  {
    ref_sRml.SetStartPosition(szEnd);

    ezStringView href = ezStringView(szStart, szEnd);
    if (href.HasExtension(".rcss"))
    {
      return href;
    }
    if (href.HasExtension(".rml"))
    {
      return href;
    }
  }

  return ezStringView();
}

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRmlUiAssetDocument, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezRmlUiAssetDocument::ezRmlUiAssetDocument(ezStringView sDocumentPath)
  : ezSimpleAssetDocument<ezRmlUiAssetProperties>(sDocumentPath, ezAssetDocEngineConnection::Simple)
{
}

void ezRmlUiAssetDocument::OpenExternalEditor()
{
  ezStringBuilder sFile(GetProperties()->m_sRmlFile);

  if (!ezFileSystem::ExistsFile(sFile))
  {
    ezQtUiServices::GetSingleton()->MessageBoxInformation(ezFmt("Can't find the file '{}'.\nTo create an RML file click the button next to 'RmlFile'.", sFile));

    ShowDocumentStatus("RML file doesn't exist.");
    return;
  }

  ezStringBuilder sFileAbs;
  if (ezFileSystem::ResolvePath(sFile, &sFileAbs, nullptr).Failed())
    return;

  {
    QStringList args;

    args.append(ezMakeQString(ezToolsProject::GetSingleton()->GetProjectDirectory()));
    args.append(sFileAbs.GetData());

    if (ezQtUiServices::OpenInVsCode(args).Failed())
    {
      // try again with a different program
      ezQtUiServices::OpenFileInDefaultProgram(sFileAbs).IgnoreResult();
    }
  }
}

ezTransformStatus ezRmlUiAssetDocument::InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  ezRmlUiAssetProperties* pProp = GetProperties();

  ezRmlUiResourceDescriptor desc;
  desc.m_sRmlFile = pProp->m_sRmlFile;
  desc.m_ScaleMode = pProp->m_ScaleMode;
  desc.m_ReferenceResolution = pProp->m_ReferenceResolution;

  desc.m_DependencyFile.AddFileDependency(pProp->m_sRmlFile);

  EZ_SUCCEED_OR_RETURN(FindDependencies(desc.m_DependencyFile, pProp->m_sRmlFile));

  EZ_SUCCEED_OR_RETURN(desc.Save(stream));

  return ezStatus(EZ_SUCCESS);
}

ezTransformStatus ezRmlUiAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  ezStatus status = ezAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}

ezStatus ezRmlUiAssetDocument::FindDependencies(ezDependencyFile& ref_Dependencies, ezStringView sFilePath) const
{
  ezStringBuilder sContent;
  {
    ezFileReader reader;
    if (reader.Open(sFilePath).Failed())
      return ezStatus(ezFmt("Failed to read file: '{}'", sFilePath));

    sContent.ReadAll(reader);
  }

  const ezStringView sFileDir = sFilePath.GetFileDirectory();

  ezStringBuilder sTemp;
  ezStringView sContentView = sContent;

  while (true)
  {
    ezStringView href = FindNextHREF(sContentView);
    if (href.IsEmpty())
      break;

    if (ezFileSystem::ExistsFile(href))
    {
      ref_Dependencies.AddFileDependency(href);
      EZ_SUCCEED_OR_RETURN(FindDependencies(ref_Dependencies, href));
      continue;
    }

    sTemp.SetPath(sFileDir, href);
    if (ezFileSystem::ExistsFile(sTemp))
    {
      ref_Dependencies.AddFileDependency(sTemp);
      EZ_SUCCEED_OR_RETURN(FindDependencies(ref_Dependencies, sTemp));
      continue;
    }
  }

  return EZ_SUCCESS;
}

void ezRmlUiAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  const ezRmlUiAssetProperties* pProp = GetProperties();

  ezDependencyFile deps;
  FindDependencies(deps, pProp->m_sRmlFile).IgnoreResult();

  for (const auto& file : deps.GetFileDependencies())
  {
    pInfo->m_TransformDependencies.Insert(file);
  }
}
