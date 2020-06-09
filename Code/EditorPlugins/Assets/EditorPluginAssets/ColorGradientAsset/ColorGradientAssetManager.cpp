#include <EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAsset.h>
#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAssetManager.h>
#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAssetWindow.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezColorGradientAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezColorGradientAssetDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezColorGradientAssetDocumentManager::ezColorGradientAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezColorGradientAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "ColorGradient";
  m_DocTypeDesc.m_sFileExtension = "ezColorGradientAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/ColorGradient.png";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezColorGradientAssetDocument>();
  m_DocTypeDesc.m_pManager = this;

  m_DocTypeDesc.m_sResourceFileExtension = "ezColorGradient";
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
        ezQtColorGradientAssetDocumentWindow* pDocWnd = new ezQtColorGradientAssetDocumentWindow(e.m_pDocument);
      }
    }
    break;

    default:
      break;
  }
}

void ezColorGradientAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, ezDocument*& out_pDocument)
{
  out_pDocument = new ezColorGradientAssetDocument(szPath);
}

void ezColorGradientAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
