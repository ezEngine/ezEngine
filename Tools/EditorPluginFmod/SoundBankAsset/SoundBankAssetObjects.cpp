#include <PCH.h>
#include <EditorPluginFmod/SoundBankAsset/SoundBankAssetObjects.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSoundBankAssetProperties, 1, ezRTTIDefaultAllocator<ezSoundBankAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("SoundBank File", m_sSoundBank)->AddAttributes(new ezFileBrowserAttribute("Select SoundBank", "*.bank")),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ezSoundBankAssetProperties::ezSoundBankAssetProperties()
{
}

