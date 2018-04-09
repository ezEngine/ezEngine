#include <PCH.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAssetManager.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAssetWindow.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSkeletonAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezSkeletonAssetDocumentManager>);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezSkeletonAssetDocumentManager::ezSkeletonAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezSkeletonAssetDocumentManager::OnDocumentManagerEvent, this));

  m_AssetDesc.m_bCanCreate = true;
  m_AssetDesc.m_sDocumentTypeName = "Skeleton Asset";
  m_AssetDesc.m_sFileExtension = "ezSkeletonAsset";
  m_AssetDesc.m_sIcon = ":/AssetIcons/Skeleton.png";
  m_AssetDesc.m_pDocumentType = ezGetStaticRTTI<ezSkeletonAssetDocument>();
  m_AssetDesc.m_pManager = this;

  //ezQtImageCache::GetSingleton()->RegisterTypeImage("Skeleton", QPixmap(":/AssetIcons/Skeleton.png"));
}

ezSkeletonAssetDocumentManager::~ezSkeletonAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezSkeletonAssetDocumentManager::OnDocumentManagerEvent, this));
}


ezBitflags<ezAssetDocumentFlags> ezSkeletonAssetDocumentManager::GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const
{
  EZ_ASSERT_DEBUG(pDescriptor->m_pManager == this, "Given type descriptor is not part of this document manager!");
  return ezAssetDocumentFlags::SupportsThumbnail;
}

void ezSkeletonAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
  case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezSkeletonAssetDocument>())
      {
        ezQtSkeletonAssetDocumentWindow* pDocWnd = new ezQtSkeletonAssetDocumentWindow(static_cast<ezSkeletonAssetDocument*>(e.m_pDocument));
      }
    }
    break;
  }
}

ezStatus ezSkeletonAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocument*& out_pDocument)
{
  out_pDocument = new ezSkeletonAssetDocument(szPath);

  return ezStatus(EZ_SUCCESS);
}

void ezSkeletonAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_AssetDesc);
}



