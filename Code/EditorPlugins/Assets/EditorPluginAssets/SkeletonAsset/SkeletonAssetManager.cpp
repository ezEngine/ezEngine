#include <EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/SkeletonAsset/SkeletonAssetManager.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAssetWindow.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSkeletonAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezSkeletonAssetDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezSkeletonAssetDocumentManager::ezSkeletonAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezSkeletonAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Skeleton";
  m_DocTypeDesc.m_sFileExtension = "ezSkeletonAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Skeleton.png";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezSkeletonAssetDocument>();
  m_DocTypeDesc.m_pManager = this;

  m_DocTypeDesc.m_sResourceFileExtension = "ezSkeleton";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::SupportsThumbnail;
}

ezSkeletonAssetDocumentManager::~ezSkeletonAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezSkeletonAssetDocumentManager::OnDocumentManagerEvent, this));
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

    default:
      break;
  }
}

void ezSkeletonAssetDocumentManager::InternalCreateDocument(
  const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, ezDocument*& out_pDocument)
{
  out_pDocument = new ezSkeletonAssetDocument(szPath);
}

void ezSkeletonAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
