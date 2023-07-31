#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/CollectionAsset/CollectionAsset.h>
#include <EditorPluginAssets/CollectionAsset/CollectionAssetManager.h>
#include <EditorPluginAssets/CollectionAsset/CollectionAssetWindow.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCollectionAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezCollectionAssetDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezCollectionAssetDocumentManager::ezCollectionAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezCollectionAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Collection";
  m_DocTypeDesc.m_sFileExtension = "ezCollectionAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Collection.svg";
  m_DocTypeDesc.m_IconColorGroup = ezColorScheme::Utilities;
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezCollectionAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_AssetCollection");

  m_DocTypeDesc.m_sResourceFileExtension = "ezCollection";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::AutoTransformOnSave | ezAssetDocumentFlags::OnlyTransformManually;

  ezQtImageCache::GetSingleton()->RegisterTypeImage("Collection", QPixmap(":/AssetIcons/Collection.svg"));
}

ezCollectionAssetDocumentManager::~ezCollectionAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezCollectionAssetDocumentManager::OnDocumentManagerEvent, this));
}

void ezCollectionAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezCollectionAssetDocument>())
      {
        new ezQtCollectionAssetDocumentWindow(e.m_pDocument); // NOLINT: not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void ezCollectionAssetDocumentManager::InternalCreateDocument(
  const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext)
{
  out_pDocument = new ezCollectionAssetDocument(szPath);
}

void ezCollectionAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
