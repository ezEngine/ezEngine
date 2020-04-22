#include <EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/LUTAsset/LUTAsset.h>
#include <EditorPluginAssets/LUTAsset/LUTAssetManager.h>
#include <EditorPluginAssets/LUTAsset/LUTAssetWindow.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLUTAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezLUTAssetDocumentManager>);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezLUTAssetDocumentManager::ezLUTAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezLUTAssetDocumentManager::OnDocumentManagerEvent, this));

  // LUT asset source files
  ezAssetFileExtensionWhitelist::AddAssetFileExtension("LUT", "cube");

  m_DocTypeDesc.m_sDocumentTypeName = "LUT";
  m_DocTypeDesc.m_sFileExtension = "ezLUTAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/LUT.png";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezLUTAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_sResourceFileExtension = "ezLUT";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::None;

  ezQtImageCache::GetSingleton()->RegisterTypeImage("LUT", QPixmap(":/AssetIcons/LUT.png"));

  //ezQtImageCache::GetSingleton()->RegisterTypeImage("Render Target", QPixmap(":/AssetIcons/Render_Target.png"));
}

ezLUTAssetDocumentManager::~ezLUTAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezLUTAssetDocumentManager::OnDocumentManagerEvent, this));
}

void ezLUTAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezLUTAssetDocument>())
      {
        ezQtLUTAssetDocumentWindow* pDocWnd = new ezQtLUTAssetDocumentWindow(static_cast<ezLUTAssetDocument*>(e.m_pDocument));
      }
    }
    break;
  }
}

void ezLUTAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, ezDocument*& out_pDocument)
{
  ezLUTAssetDocument* pDoc = new ezLUTAssetDocument(szPath);
  out_pDocument = pDoc;
}

void ezLUTAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
