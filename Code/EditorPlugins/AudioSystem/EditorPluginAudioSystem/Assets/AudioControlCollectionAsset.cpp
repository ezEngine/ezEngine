#include <EditorPluginAudioSystem/EditorPluginAudioSystemPCH.h>

#include <EditorPluginAudioSystem/Assets/AudioControlCollectionAsset.h>

#include <AudioSystemPlugin/Resources/AudioControlCollectionResource.h>

#include <EditorFramework/Assets/AssetCurator.h>

#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAudioControlCollectionAssetEntry, 1, ezRTTIDefaultAllocator<ezAudioControlCollectionAssetEntry>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Name", m_sName),
    EZ_ENUM_MEMBER_PROPERTY("Type", ezAudioSystemControlType, m_Type)->AddAttributes(new ezDefaultValueAttribute(ezAudioSystemControlType::Invalid)),
    EZ_MEMBER_PROPERTY("Control", m_sControlFile)->AddAttributes(new ezFileBrowserAttribute("Select Audio System Control", "*.ezAudioSystemControl")),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAudioControlCollectionAsset, 1, ezRTTIDefaultAllocator<ezAudioControlCollectionAsset>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_MEMBER_PROPERTY("Entries", m_Entries),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAudioControlCollectionAssetDocument, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezAudioControlCollectionAssetDocument::ezAudioControlCollectionAssetDocument(const char* szDocumentPath)
  : ezSimpleAssetDocument<ezAudioControlCollectionAsset>(szDocumentPath, ezAssetDocEngineConnection::None)
{
}

void ezAudioControlCollectionAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  const ezAudioControlCollectionAsset* pProp = GetProperties();

  for (const auto& e : pProp->m_Entries)
  {
    if (!e.m_sControlFile.IsEmpty())
      pInfo->m_AssetTransformDependencies.Insert(e.m_sControlFile);
  }
}

ezStatus ezAudioControlCollectionAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  const ezAudioControlCollectionAsset* pProp = GetProperties();

  ezAudioControlCollectionResourceDescriptor descriptor;

  for (const auto& e : pProp->m_Entries)
  {
    if (e.m_sControlFile.IsEmpty() || e.m_Type == ezAudioSystemControlType::Invalid)
      continue;

    ezStringBuilder sAssetFile = e.m_sControlFile;
    if (!ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAssetFile))
    {
      ezLog::Warning("Failed to make audio control path absolute: '{0}'", e.m_sControlFile);
      continue;
    }

    ezAudioControlCollectionEntry entry;
    entry.m_sName = e.m_sName;
    entry.m_Type = e.m_Type;
    entry.m_sControlFile = e.m_sControlFile;

    descriptor.m_Entries.PushBack(entry);
  }

  descriptor.Save(stream);
  return {EZ_SUCCESS};
}
