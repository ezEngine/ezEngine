#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/ImageDataAsset/ImageDataAsset.h>
#include <EditorPluginAssets/ImageDataAsset/ImageDataAssetManager.h>
#include <EditorPluginAssets/ImageDataAsset/ImageDataAssetWindow.moc.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezImageDataAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezImageDataAssetDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezImageDataAssetDocumentManager::ezImageDataAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezImageDataAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Image Data";
  m_DocTypeDesc.m_sFileExtension = "ezImageDataAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/ImageData.svg";
  m_DocTypeDesc.m_sAssetCategory = "Utilities";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezImageDataAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_sResourceFileExtension = "ezBinImageData";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::AutoThumbnailOnTransform;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Data_2D");
}

ezImageDataAssetDocumentManager::~ezImageDataAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezImageDataAssetDocumentManager::OnDocumentManagerEvent, this));
}

void ezImageDataAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezImageDataAssetDocument>())
      {
        new ezQtImageDataAssetDocumentWindow(static_cast<ezImageDataAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void ezImageDataAssetDocumentManager::InternalCreateDocument(ezStringView sDocumentTypeName, ezStringView sPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext)
{
  ezImageDataAssetDocument* pDoc = new ezImageDataAssetDocument(sPath);
  out_pDocument = pDoc;
}

void ezImageDataAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
