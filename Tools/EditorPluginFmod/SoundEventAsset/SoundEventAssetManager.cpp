#include <PCH.h>
#include <EditorPluginFmod/SoundEventAsset/SoundEventAssetManager.h>
#include <EditorPluginFmod/SoundEventAsset/SoundEventAsset.h>
#include <EditorPluginFmod/SoundEventAsset/SoundEventAssetWindow.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSoundEventAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezSoundEventAssetDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezSoundEventAssetDocumentManager::ezSoundEventAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezSoundEventAssetDocumentManager::OnDocumentManagerEvent, this));

  m_AssetDesc.m_bCanCreate = false;
  m_AssetDesc.m_sDocumentTypeName = "Sound Event Asset";
  m_AssetDesc.m_sFileExtension = "ezSoundEventAsset";
  m_AssetDesc.m_sIcon = ":/AssetIcons/Sound_Event.png";
  m_AssetDesc.m_pDocumentType = ezGetStaticRTTI<ezSoundEventAssetDocument>();
  m_AssetDesc.m_pManager = this;

  ezQtImageCache::GetSingleton()->RegisterTypeImage("Sound Event", QPixmap(":/AssetIcons/Sound_Event.png"));
}

ezSoundEventAssetDocumentManager::~ezSoundEventAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezSoundEventAssetDocumentManager::OnDocumentManagerEvent, this));
}

void ezSoundEventAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
  case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezSoundEventAssetDocument>())
      {
        ezSoundEventAssetDocumentWindow* pDocWnd = new ezSoundEventAssetDocumentWindow(e.m_pDocument);
      }
    }
    break;
  }
}

ezStatus ezSoundEventAssetDocumentManager::InternalCanOpenDocument(const char* szDocumentTypeName, const char* szFilePath) const
{
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezSoundEventAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocument*& out_pDocument)
{
  out_pDocument = new ezSoundEventAssetDocument(szPath);

  return ezStatus(EZ_SUCCESS);
}

void ezSoundEventAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_AssetDesc);
}



