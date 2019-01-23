#include <PCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginFmod/SoundBankAsset/SoundBankAsset.h>
#include <EditorPluginFmod/SoundBankAsset/SoundBankAssetManager.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Strings/PathUtils.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSoundBankAssetDocument, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSoundBankAssetProperties, 1, ezRTTIDefaultAllocator<ezSoundBankAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("SoundBankFile", m_sSoundBank)->AddAttributes(new ezFileBrowserAttribute("Select SoundBank", "*.bank")),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSoundBankAssetDocument::ezSoundBankAssetDocument(const char* szDocumentPath)
    : ezSimpleAssetDocument<ezSoundBankAssetProperties>(szDocumentPath)
{
}

void ezSoundBankAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  const ezSoundBankAssetProperties* pProp = GetProperties();

  pInfo->m_AssetTransformDependencies.Insert(pProp->m_sSoundBank);
}

ezStatus ezSoundBankAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile,
                                                          const ezAssetFileHeader& AssetHeader, bool bTriggeredManually)
{
  const ezSoundBankAssetProperties* pProp = GetProperties();

  if (pProp->m_sSoundBank.IsEmpty())
    return ezStatus("No sound-bank file has been specified.");

  if (!ezPathUtils::HasExtension(pProp->m_sSoundBank, "bank"))
    return ezStatus(ezFmt("Specified sound-bank file should have 'bank' extension: '{0}'", pProp->m_sSoundBank));

  /// \todo For platform specific sound banks, adjust the path to point to the correct file

  ezStringBuilder sAssetFile = pProp->m_sSoundBank;
  if (!ezQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAssetFile))
    return ezStatus(ezFmt("Failed to make sound-bank path absolute: '{0}'", pProp->m_sSoundBank));

  ezFileReader SoundBankFile;
  if (SoundBankFile.Open(sAssetFile).Failed())
    return ezStatus(ezFmt("Could not open sound-bank for reading: '{0}'", sAssetFile));

  // we copy the entire sound bank into our transformed asset
  // however, at least during development, we typically do not load the data from there,
  // but from the fmod sound bank files directly, so that we do not need to wait for an asset transform

  ezMemoryStreamStorage storage;

  // copy the file from disk into memory
  {
    ezMemoryStreamWriter writer(&storage);

    ezUInt8 Temp[4 * 1024];

    while (true)
    {
      ezUInt64 uiRead = SoundBankFile.ReadBytes(Temp, EZ_ARRAY_SIZE(Temp));

      if (uiRead == 0)
        break;

      writer.WriteBytes(Temp, uiRead);
    }
  }

  const ezUInt8 uiVersion = 1;
  stream << uiVersion;

  // now store the entire file in our asset output
  {
    stream << storage.GetStorageSize();
    stream.WriteBytes(storage.GetData(), storage.GetStorageSize());
  }

  return ezStatus(EZ_SUCCESS);
}
