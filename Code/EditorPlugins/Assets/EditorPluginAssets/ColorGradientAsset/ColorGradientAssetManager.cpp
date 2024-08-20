#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAsset.h>
#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAssetManager.h>
#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAssetWindow.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezColorGradientAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezColorGradientAssetDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezColorGradientAssetDocumentManager::ezColorGradientAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezColorGradientAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "ColorGradient";
  m_DocTypeDesc.m_sFileExtension = "ezColorGradientAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/ColorGradient.svg";
  m_DocTypeDesc.m_sAssetCategory = "Animation";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezColorGradientAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Data_Gradient");

  m_DocTypeDesc.m_sResourceFileExtension = "ezBinColorGradient";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::AutoTransformOnSave | ezAssetDocumentFlags::SupportsThumbnail;
}

ezColorGradientAssetDocumentManager::~ezColorGradientAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezColorGradientAssetDocumentManager::OnDocumentManagerEvent, this));
}

void ezColorGradientAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezColorGradientAssetDocument>())
      {
        new ezQtColorGradientAssetDocumentWindow(e.m_pDocument); // NOLINT: not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void ezColorGradientAssetDocumentManager::InternalCreateDocument(
  ezStringView sDocumentTypeName, ezStringView sPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext)
{
  out_pDocument = new ezColorGradientAssetDocument(sPath);
}

void ezColorGradientAssetDocumentManager::InternalGetSupportedDocumentTypes(
  ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
