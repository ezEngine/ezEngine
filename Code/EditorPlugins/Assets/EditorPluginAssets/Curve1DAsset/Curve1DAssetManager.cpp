#include <EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/Curve1DAsset/Curve1DAsset.h>
#include <EditorPluginAssets/Curve1DAsset/Curve1DAssetManager.h>
#include <EditorPluginAssets/Curve1DAsset/Curve1DAssetWindow.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCurve1DAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezCurve1DAssetDocumentManager>);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezCurve1DAssetDocumentManager::ezCurve1DAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezCurve1DAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Curve1D";
  m_DocTypeDesc.m_sFileExtension = "ezCurve1DAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Curve1D.png";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezCurve1DAssetDocument>();
  m_DocTypeDesc.m_pManager = this;

  m_DocTypeDesc.m_sResourceFileExtension = "ezCurve1D";
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
        ezQtCurve1DAssetDocumentWindow* pDocWnd = new ezQtCurve1DAssetDocumentWindow(e.m_pDocument);
      }
    }
    break;
  }
}

void ezCurve1DAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, ezDocument*& out_pDocument)
{
  out_pDocument = new ezCurve1DAssetDocument(szPath);
}

void ezCurve1DAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
