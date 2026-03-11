#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/Curve1DAsset/Curve1DAsset.h>
#include <EditorPluginAssets/Curve1DAsset/Curve1DAssetManager.h>
#include <EditorPluginAssets/Curve1DAsset/Curve1DAssetWindow.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCurve1DAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezCurve1DAssetDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezCurve1DAssetDocumentManager::ezCurve1DAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezCurve1DAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Curve1D";
  m_DocTypeDesc.m_sFileExtension = "ezCurve1DAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Curve1D.svg";
  m_DocTypeDesc.m_sAssetCategory = "Utilities";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezCurve1DAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Data_Curve");

  m_DocTypeDesc.m_sResourceFileExtension = "ezBinCurve1D";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::AutoTransformOnSave | ezAssetDocumentFlags::SupportsThumbnail;
}

ezCurve1DAssetDocumentManager::~ezCurve1DAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezCurve1DAssetDocumentManager::OnDocumentManagerEvent, this));
}

void ezCurve1DAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezCurve1DAssetDocument>())
      {
        new ezQtCurve1DAssetDocumentWindow(e.m_pDocument); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void ezCurve1DAssetDocumentManager::InternalCreateDocument(
  ezStringView sDocumentTypeName, ezStringView sPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext)
{
  out_pDocument = new ezCurve1DAssetDocument(sPath);
}

void ezCurve1DAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
