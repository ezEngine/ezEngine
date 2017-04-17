#include <PCH.h>
#include <EditorPluginFmod/SoundBankAsset/SoundBankAsset.h>
#include <EditorPluginFmod/SoundBankAsset/SoundBankAssetManager.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSoundBankAssetDocument, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSoundBankAssetProperties, 1, ezRTTIDefaultAllocator<ezSoundBankAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("SoundBankFile", m_sSoundBank)->AddAttributes(new ezFileBrowserAttribute("Select SoundBank", "*.bank")),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezSoundBankAssetDocument::ezSoundBankAssetDocument(const char* szDocumentPath) : ezSimpleAssetDocument<ezSoundBankAssetProperties>(szDocumentPath)
{
}

void ezSoundBankAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const
{
  ezAssetDocument::UpdateAssetDocumentInfo(pInfo);

  const ezSoundBankAssetProperties* pProp = GetProperties();

  pInfo->m_FileDependencies.Insert(pProp->m_sSoundBank);
}

ezStatus ezSoundBankAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually)
{
  return ezStatus(EZ_SUCCESS);
}
