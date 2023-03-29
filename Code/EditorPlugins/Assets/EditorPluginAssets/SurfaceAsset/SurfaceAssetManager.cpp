#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/SurfaceAsset/SurfaceAsset.h>
#include <EditorPluginAssets/SurfaceAsset/SurfaceAssetManager.h>
#include <EditorPluginAssets/SurfaceAsset/SurfaceAssetWindow.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSurfaceAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezSurfaceAssetDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezSurfaceAssetDocumentManager::ezSurfaceAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezSurfaceAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Surface";
  m_DocTypeDesc.m_sFileExtension = "ezSurfaceAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Surface.png";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezSurfaceAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Surface");

  m_DocTypeDesc.m_sResourceFileExtension = "ezSurface";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::AutoTransformOnSave;

  ezQtImageCache::GetSingleton()->RegisterTypeImage("Surface", QPixmap(":/AssetIcons/Surface.png"));
}

ezSurfaceAssetDocumentManager::~ezSurfaceAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezSurfaceAssetDocumentManager::OnDocumentManagerEvent, this));
}

void ezSurfaceAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezSurfaceAssetDocument>())
      {
        ezQtSurfaceAssetDocumentWindow* pDocWnd = new ezQtSurfaceAssetDocumentWindow(e.m_pDocument);
      }
    }
    break;

    default:
      break;
  }
}

void ezSurfaceAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext)
{
  out_pDocument = new ezSurfaceAssetDocument(szPath);
}

void ezSurfaceAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
