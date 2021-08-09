#include <EditorPluginFmod/EditorPluginFmodPCH.h>

#include <EditorPluginFmod/SoundEventAsset/SoundEventAssetManager.h>
#include <EditorPluginFmod/SoundEventAsset/SoundEventAssetWindow.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSoundEventAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezSoundEventAssetDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezSoundEventAssetDocumentManager::ezSoundEventAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezSoundEventAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_bCanCreate = false;
  m_DocTypeDesc.m_sDocumentTypeName = "Sound Event";
  m_DocTypeDesc.m_sFileExtension = "ezSoundEventAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Sound_Event.png";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezSoundEventAssetDocument>();
  m_DocTypeDesc.m_pManager = this;

  m_DocTypeDesc.m_sResourceFileExtension = "ezFmodSoundEvent";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::None;

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

void ezSoundEventAssetDocumentManager::InternalCreateDocument(
  const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, ezDocument*& out_pDocument)
{
  out_pDocument = new ezSoundEventAssetDocument(szPath);
}

void ezSoundEventAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
