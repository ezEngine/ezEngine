#include <EditorPluginAudioSystem/EditorPluginAudioSystemPCH.h>

#include <EditorPluginAudioSystem/Assets/AudioControlCollectionAsset.h>
#include <EditorPluginAudioSystem/Assets/AudioControlCollectionAssetManager.h>
#include <EditorPluginAudioSystem/Assets/AudioControlCollectionAssetWindow.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAudioControlCollectionAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezAudioControlCollectionAssetDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezAudioControlCollectionAssetDocumentManager::ezAudioControlCollectionAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezAudioControlCollectionAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Audio Control Collection";
  m_DocTypeDesc.m_sFileExtension = "ezAudioControlCollectionAsset";
  // m_DocTypeDesc.m_sIcon = ":/AssetIcons/Collection.png";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezAudioControlCollectionAssetDocument>();
  m_DocTypeDesc.m_pManager = this;

  m_DocTypeDesc.m_sResourceFileExtension = "ezAudioSystemControls";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::AutoTransformOnSave;

  // ezQtImageCache::GetSingleton()->RegisterTypeImage("Audio Control Collection", QPixmap(":/AssetIcons/Collection.png"));
}

ezAudioControlCollectionAssetDocumentManager::~ezAudioControlCollectionAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezAudioControlCollectionAssetDocumentManager::OnDocumentManagerEvent, this));
}

void ezAudioControlCollectionAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezAudioControlCollectionAssetDocument>())
      {
        ezQtAudioControlCollectionAssetDocumentWindow* pDocWnd = new ezQtAudioControlCollectionAssetDocumentWindow(e.m_pDocument);
      }
    }
    break;

    default:
      break;
  }
}

void ezAudioControlCollectionAssetDocumentManager::InternalCreateDocument(
  const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext)
{
  out_pDocument = new ezAudioControlCollectionAssetDocument(szPath);
}

void ezAudioControlCollectionAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
