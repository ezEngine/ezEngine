#include <EditorPluginTypeScriptPCH.h>

#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAsset.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAssetManager.h>
#include <EditorPluginTypeScript/TypeScriptAsset/TypeScriptAssetWindow.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTypeScriptAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezTypeScriptAssetDocumentManager>);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezTypeScriptAssetDocumentManager::ezTypeScriptAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezTypeScriptAssetDocumentManager::OnDocumentManagerEvent, this));

  m_AssetDesc.m_bCanCreate = true;
  m_AssetDesc.m_sDocumentTypeName = "TypeScript Asset";
  m_AssetDesc.m_sFileExtension = "ezTypeScriptAsset";
  m_AssetDesc.m_sIcon = ":/AssetIcons/TypeScript.png";
  m_AssetDesc.m_pDocumentType = ezGetStaticRTTI<ezTypeScriptAssetDocument>();
  m_AssetDesc.m_pManager = this;

  ezQtImageCache::GetSingleton()->RegisterTypeImage("TypeScript", QPixmap(":/AssetIcons/TypeScript.png"));
}

ezTypeScriptAssetDocumentManager::~ezTypeScriptAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezTypeScriptAssetDocumentManager::OnDocumentManagerEvent, this));
}

void ezTypeScriptAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezTypeScriptAssetDocument>())
      {
        ezQtTypeScriptAssetDocumentWindow* pDocWnd =
            new ezQtTypeScriptAssetDocumentWindow(static_cast<ezTypeScriptAssetDocument*>(e.m_pDocument));
      }
    }
    break;
  }
}

ezStatus ezTypeScriptAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath,
                                                                     bool bCreateNewDocument, ezDocument*& out_pDocument)
{
  out_pDocument = new ezTypeScriptAssetDocument(szPath);

  return ezStatus(EZ_SUCCESS);
}

void ezTypeScriptAssetDocumentManager::InternalGetSupportedDocumentTypes(
    ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_AssetDesc);
}

ezBitflags<ezAssetDocumentFlags>
ezTypeScriptAssetDocumentManager::GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const
{
  EZ_ASSERT_DEBUG(pDescriptor->m_pManager == this, "Given type descriptor is not part of this document manager!");
  return ezAssetDocumentFlags::None;
}

