#include <EditorPluginRmlUi/EditorPluginRmlUiPCH.h>

#include <EditorPluginRmlUi/RmlUiAsset/RmlUiAsset.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <ToolsFoundation/Project/ToolsProject.h>

ezStringView FindNextHREF(ezStringView& ref_sRml)
{
  while (true)
  {
    const char* szCurrent = ref_sRml.FindSubString("href");
    if (szCurrent == nullptr)
      return ezStringView();

    szCurrent += 4; // skip "href"

    const char* szStart = nullptr;
    const char* szEnd = nullptr;
    while (*szCurrent != '\0')
    {
      if (*szCurrent == '\"')
      {
        if (szStart == nullptr)
          szStart = szCurrent + 1;
        else
        {
          szEnd = szCurrent;
          break;
        }
      }
      ++szCurrent;
    }

    if (szStart == nullptr || szEnd == nullptr)
    {
      // malformed or reached end of string
      ref_sRml.SetStartPosition(szCurrent);
      return ezStringView();
    }

    ref_sRml.SetStartPosition(szEnd + 1);

    const ezStringView href = ezStringView(szStart, szEnd);
    if (href.HasExtension(".rcss") || href.HasExtension(".rml"))
      return href;

    // non-matching extension; continue searching
  }
}

ezStringView FindNextSrcValue(ezStringView& ref_sContent)
{
  while (true)
  {
    const char* szCurrent = ref_sContent.FindSubString("src");
    if (szCurrent == nullptr)
      return ezStringView();

    szCurrent += 3;

    szCurrent = ezStringUtils::SkipCharacters(szCurrent, ezStringUtils::IsWhiteSpace);

    if (*szCurrent == '=')
    {
      // HTML attribute: src="value" or src='value'
      ++szCurrent;
      const char cQuote = *szCurrent;
      if (cQuote == '"' || cQuote == '\'')
      {
        ++szCurrent;
        const char* szStart = szCurrent;
        while (*szCurrent != '\0' && *szCurrent != cQuote)
          ++szCurrent;
        if (*szCurrent == cQuote)
        {
          ref_sContent.SetStartPosition(szCurrent + 1);
          return ezStringView(szStart, szCurrent);
        }
      }
      ref_sContent.SetStartPosition(szCurrent);
      continue;
    }
    else if (*szCurrent == ':')
    {
      // CSS property: src: value;
      ++szCurrent;
      szCurrent = ezStringUtils::SkipCharacters(szCurrent, ezStringUtils::IsWhiteSpace);
      const char* szStart = szCurrent;
      while (*szCurrent != '\0' && *szCurrent != ';' && *szCurrent != '}' && *szCurrent != '\n' && *szCurrent != '\r')
        ++szCurrent;
      const char* szEnd = szCurrent;
      ezStringUtils::Trim(szStart, szEnd, nullptr, " \t");
      ref_sContent.SetStartPosition(szCurrent);
      if (szEnd > szStart)
        return ezStringView(szStart, szEnd);
      continue;
    }
    else
    {
      // "src" was part of another word, skip past it
      ref_sContent.SetStartPosition(szCurrent);
      continue;
    }
  }
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
  ezSet<ezString> visited;
  return FindDependencies(ref_Dependencies, sFilePath, visited);
}

ezStatus ezRmlUiAssetDocument::FindDependencies(ezDependencyFile& ref_Dependencies, ezStringView sFilePath, ezSet<ezString>& ref_visited) const
{
  ezString sFilePathStr = sFilePath;
  if (ref_visited.Contains(sFilePathStr))
    return EZ_SUCCESS;
  ref_visited.Insert(sFilePathStr);

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
    const ezStringView href = FindNextHREF(sContentView);
    if (href.IsEmpty())
      break;

    if (ezFileSystem::ExistsFile(href))
    {
      ref_Dependencies.AddFileDependency(href);
      EZ_SUCCEED_OR_RETURN(FindDependencies(ref_Dependencies, href, ref_visited));
      continue;
    }

    sTemp.SetPath(sFileDir, href);
    if (ezFileSystem::ExistsFile(sTemp))
    {
      ref_Dependencies.AddFileDependency(sTemp);
      EZ_SUCCEED_OR_RETURN(FindDependencies(ref_Dependencies, sTemp, ref_visited));
      continue;
    }
  }

  return EZ_SUCCESS;
}

void ezRmlUiAssetDocument::FindPackageDependencies(ezSet<ezString>& ref_packageDeps, ezStringView sFilePath, ezSet<ezString>& ref_visited) const
{
  ezString sFilePathStr = sFilePath;
  if (ref_visited.Contains(sFilePathStr))
    return;
  ref_visited.Insert(sFilePathStr);

  ezStringBuilder sContent;
  {
    ezFileReader reader;
    if (reader.Open(sFilePath).Failed())
      return;
    sContent.ReadAll(reader);
  }

  const ezStringView sFileDir = sFilePath.GetFileDirectory();
  ezStringBuilder sTemp;

  // Recurse into referenced rcss/rml files
  {
    ezStringView sContentView = sContent;
    while (true)
    {
      const ezStringView href = FindNextHREF(sContentView);
      if (href.IsEmpty())
        break;

      if (ezFileSystem::ExistsFile(href))
      {
        FindPackageDependencies(ref_packageDeps, href, ref_visited);
      }
      else
      {
        sTemp.SetPath(sFileDir, href);
        if (ezFileSystem::ExistsFile(sTemp))
          FindPackageDependencies(ref_packageDeps, sTemp, ref_visited);
      }
    }
  }

  // Collect source file references (images, fonts, etc.)
  {
    ezStringView sContentView = sContent;
    while (true)
    {
      const ezStringView src = FindNextSrcValue(sContentView);
      if (src.IsEmpty())
        break;

      if (ezFileSystem::ExistsFile(src))
      {
        ref_packageDeps.Insert(src);
      }
      else
      {
        sTemp.SetPath(sFileDir, src);
        if (ezFileSystem::ExistsFile(sTemp))
          ref_packageDeps.Insert(sTemp);
      }
    }
  }
}

void ezRmlUiAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  const ezRmlUiAssetProperties* pProp = GetProperties();

  // rcss/rml files referenced via href must be re-transformed and re-thumbnailed if changed, and packaged for runtime
  ezDependencyFile deps;
  FindDependencies(deps, pProp->m_sRmlFile).IgnoreResult();

  for (const auto& file : deps.GetFileDependencies())
  {
    pInfo->m_TransformDependencies.Insert(file);
    pInfo->m_ThumbnailDependencies.Insert(file);
    pInfo->m_PackageDependencies.Insert(file);
  }

  // source files (images, fonts) referenced via src must be packaged for runtime and affect the thumbnail,
  // but do not affect the transform output (which only stores file paths)
  ezSet<ezString> sourceDeps;
  ezSet<ezString> visited;
  FindPackageDependencies(sourceDeps, pProp->m_sRmlFile, visited);

  for (const auto& file : sourceDeps)
  {
    pInfo->m_ThumbnailDependencies.Insert(file);
    pInfo->m_PackageDependencies.Insert(file);
  }
}
