#include <EditorPluginRmlUi/EditorPluginRmlUiPCH.h>

#include <EditorPluginRmlUi/RmlUiAsset/RmlUiAssetManager.h>
#include <EditorPluginRmlUi/RmlUiAsset/RmlUiAssetWindow.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRmlUiAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezRmlUiAssetDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezRmlUiAssetDocumentManager::ezRmlUiAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezRmlUiAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "RmlUi";
  m_DocTypeDesc.m_sFileExtension = "ezRmlUiAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/RmlUi.png";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezRmlUiAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Rml_UI");

  m_DocTypeDesc.m_sResourceFileExtension = "ezRmlUi";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::SupportsThumbnail;
}

ezRmlUiAssetDocumentManager::~ezRmlUiAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezRmlUiAssetDocumentManager::OnDocumentManagerEvent, this));
}

void ezRmlUiAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezRmlUiAssetDocument>())
      {
        ezQtRmlUiAssetDocumentWindow* pDocWnd = new ezQtRmlUiAssetDocumentWindow(static_cast<ezRmlUiAssetDocument*>(e.m_pDocument));
      }
    }
    break;
  }
}

void ezRmlUiAssetDocumentManager::InternalCreateDocument(
  const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext)
{
  out_pDocument = new ezRmlUiAssetDocument(szPath);
}

void ezRmlUiAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
