#include <PCH.h>

#include <EditorPluginAssets/CollectionAsset/CollectionAsset.h>
#include <EditorPluginAssets/CollectionAsset/CollectionAssetManager.h>
#include <EditorPluginAssets/CollectionAsset/CollectionAssetWindow.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCollectionAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezCollectionAssetDocumentManager>);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezCollectionAssetDocumentManager::ezCollectionAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezCollectionAssetDocumentManager::OnDocumentManagerEvent, this));

  m_AssetDesc.m_bCanCreate = true;
  m_AssetDesc.m_sDocumentTypeName = "Collection Asset";
  m_AssetDesc.m_sFileExtension = "ezCollectionAsset";
  m_AssetDesc.m_sIcon = ":/AssetIcons/Collection.png";
  m_AssetDesc.m_pDocumentType = ezGetStaticRTTI<ezCollectionAssetDocument>();
  m_AssetDesc.m_pManager = this;

  ezQtImageCache::GetSingleton()->RegisterTypeImage("Collection", QPixmap(":/AssetIcons/Collection.png"));
}

ezCollectionAssetDocumentManager::~ezCollectionAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezCollectionAssetDocumentManager::OnDocumentManagerEvent, this));
}


ezBitflags<ezAssetDocumentFlags>
ezCollectionAssetDocumentManager::GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const
{
  EZ_ASSERT_DEBUG(pDescriptor->m_pManager == this, "Given type descriptor is not part of this document manager!");
  return ezAssetDocumentFlags::AutoTransformOnSave;
}

void ezCollectionAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezCollectionAssetDocument>())
      {
        ezQtCollectionAssetDocumentWindow* pDocWnd = new ezQtCollectionAssetDocumentWindow(e.m_pDocument);
      }
    }
    break;
  }
}

ezStatus ezCollectionAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath,
                                                                  bool bCreateNewDocument, ezDocument*& out_pDocument)
{
  out_pDocument = new ezCollectionAssetDocument(szPath);

  return ezStatus(EZ_SUCCESS);
}

void ezCollectionAssetDocumentManager::InternalGetSupportedDocumentTypes(
    ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_AssetDesc);
}
