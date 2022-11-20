#include <EditorPluginRmlUi/EditorPluginRmlUiPCH.h>

#include <EditorPluginRmlUi/RmlUiAsset/RmlUiAsset.h>
#include <Foundation/IO/FileSystem/FileReader.h>

ezStringView FindRCSSReference(ezStringView& sRml)
{
  const char* szCurrent = sRml.FindSubString("href");
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
    sRml.SetStartPosition(szEnd);

    ezStringView rcss = ezStringView(szStart, szEnd);
    if (rcss.EndsWith_NoCase(".rcss"))
    {
      return rcss;
    }
  }

  return ezStringView();
}

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRmlUiAssetDocument, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezRmlUiAssetDocument::ezRmlUiAssetDocument(const char* szDocumentPath)
  : ezSimpleAssetDocument<ezRmlUiAssetProperties>(szDocumentPath, ezAssetDocEngineConnection::Simple)
{
}

ezTransformStatus ezRmlUiAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  ezRmlUiAssetProperties* pProp = GetProperties();

  ezRmlUiResourceDescriptor desc;
  desc.m_sRmlFile = pProp->m_sRmlFile;
  desc.m_ScaleMode = pProp->m_ScaleMode;
  desc.m_ReferenceResolution = pProp->m_ReferenceResolution;

  desc.m_DependencyFile.AddFileDependency(pProp->m_sRmlFile);

  // Find rcss dependencies
  {
    ezStringBuilder sContent;
    {
      ezFileReader reader;
      if (reader.Open(pProp->m_sRmlFile).Failed())
        return ezStatus("Failed to read rml file");

      sContent.ReadAll(reader);
    }

    ezStringBuilder sRmlFilePath = pProp->m_sRmlFile;
    sRmlFilePath = sRmlFilePath.GetFileDirectory();

    ezStringView sContentView = sContent;

    while (true)
    {
      ezStringView rcssReference = FindRCSSReference(sContentView);
      if (rcssReference.IsEmpty())
        break;

      ezStringBuilder sRcssRef = rcssReference;
      if (!ezFileSystem::ExistsFile(sRcssRef))
      {
        ezStringBuilder sTemp;
        sTemp.AppendPath(sRmlFilePath, sRcssRef);
        sRcssRef = sTemp;
      }

      if (ezFileSystem::ExistsFile(sRcssRef))
      {
        desc.m_DependencyFile.AddFileDependency(sRcssRef);
      }
      else
      {
        ezLog::Warning("RCSS file '{}' was not added as dependency since it doesn't exist", sRcssRef);
      }
    }
  }

  EZ_SUCCEED_OR_RETURN(desc.Save(stream));

  return ezStatus(EZ_SUCCESS);
}

ezTransformStatus ezRmlUiAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  ezStatus status = ezAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}
