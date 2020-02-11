#include <PCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginUltralight/UltralightHTMLAsset/UltralightHTMLAsset.h>
#include <EditorPluginUltralight/UltralightHTMLAsset/UltralightHTMLAssetManager.h>
#include <EditorPluginUltralight/UltralightHTMLAsset/UltralightHTMLAssetWindow.moc.h>
#include <Foundation/IO/OSFile.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezUltralightHTMLAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezUltralightHTMLAssetDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


ezUltralightHTMLAssetDocumentManager::ezUltralightHTMLAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezUltralightHTMLAssetDocumentManager::OnDocumentManagerEvent, this));

  m_AssetDesc.m_bCanCreate = true;
  m_AssetDesc.m_sDocumentTypeName = "HTML Texture";
  m_AssetDesc.m_sFileExtension = "ezUltralightHTMLAsset";
  m_AssetDesc.m_sIcon = ":/AssetIcons/HTML_Page.png";
  m_AssetDesc.m_pDocumentType = ezGetStaticRTTI<ezUltralightHTMLAssetDocument>();
  m_AssetDesc.m_pManager = this;

  m_AssetDesc.m_sResourceFileExtension = "ezUltralightHTML";

  ezQtImageCache::GetSingleton()->RegisterTypeImage("HTML Texture", QPixmap(":/AssetIcons/HTML_Page.png"));
}

ezUltralightHTMLAssetDocumentManager::~ezUltralightHTMLAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezUltralightHTMLAssetDocumentManager::OnDocumentManagerEvent, this));
}

void ezUltralightHTMLAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezUltralightHTMLAssetDocument>())
      {
        ezUltralightHTMLAssetDocumentWindow* pDocWnd = new ezUltralightHTMLAssetDocumentWindow(e.m_pDocument);
      }
    }
    break;
  }
}

void ezUltralightHTMLAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath,
                                                                 bool bCreateNewDocument, ezDocument*& out_pDocument)
{
  out_pDocument = new ezUltralightHTMLAssetDocument(szPath);
}

void ezUltralightHTMLAssetDocumentManager::InternalGetSupportedDocumentTypes(
    ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_AssetDesc);
}

ezUInt64 ezUltralightHTMLAssetDocumentManager::ComputeAssetProfileHashImpl(const ezPlatformProfile* pAssetProfile) const
{
  // don't have any settings yet, but assets that generate profile specific output must not return 0 here
  return 0;
}
