#include <EditorPluginMiniAudio/EditorPluginMiniAudioPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginMiniAudio/SoundAsset/MiniAudioSoundAssetManager.h>
#include <EditorPluginMiniAudio/SoundAsset/MiniAudioSoundAssetWindow.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMiniAudioSoundAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezMiniAudioSoundAssetDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezMiniAudioSoundAssetDocumentManager::ezMiniAudioSoundAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezMiniAudioSoundAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "MiniAudioSound";
  m_DocTypeDesc.m_sFileExtension = "ezMiniAudioSoundAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/MiniAudioSound.svg";
  m_DocTypeDesc.m_sAssetCategory = "Sound";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezMiniAudioSoundAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_MiniAudio_Sound");

  m_DocTypeDesc.m_sResourceFileExtension = "ezBinMiniAudioSound";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::AutoTransformOnSave;

  ezQtImageCache::GetSingleton()->RegisterTypeImage("Sound", QPixmap(":/AssetIcons/MiniAudioSound.svg"));
}

ezMiniAudioSoundAssetDocumentManager::~ezMiniAudioSoundAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezMiniAudioSoundAssetDocumentManager::OnDocumentManagerEvent, this));
}

void ezMiniAudioSoundAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezMiniAudioSoundAssetDocument>())
      {
        new ezMiniAudioSoundAssetDocumentWindow(e.m_pDocument); // NOLINT: Not a memory leak
      }
    }
    break;
    default:
      break;
  }
}

void ezMiniAudioSoundAssetDocumentManager::InternalCreateDocument(ezStringView sDocumentTypeName, ezStringView sPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext)
{
  out_pDocument = new ezMiniAudioSoundAssetDocument(sPath);
}

void ezMiniAudioSoundAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
