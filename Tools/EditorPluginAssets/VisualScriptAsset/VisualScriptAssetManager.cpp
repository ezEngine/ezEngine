#include <PCH.h>

#include <EditorPluginAssets/VisualScriptAsset/VisualScriptAsset.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptAssetManager.h>
#include <EditorPluginAssets/VisualScriptAsset/VisualScriptAssetWindow.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezVisualScriptAssetManager, 1, ezRTTIDefaultAllocator<ezVisualScriptAssetManager>);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezVisualScriptAssetManager::ezVisualScriptAssetManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezVisualScriptAssetManager::OnDocumentManagerEvent, this));

  // additional whitelist for non-asset files where an asset may be selected
  // ezAssetFileExtensionWhitelist::AddAssetFileExtension("Material", "ezMaterial");

  m_AssetDesc.m_bCanCreate = true;
  m_AssetDesc.m_sDocumentTypeName = "Visual Script Asset";
  m_AssetDesc.m_sFileExtension = "ezVisualScriptAsset";
  m_AssetDesc.m_sIcon = ":/AssetIcons/Visual_Script.png";
  m_AssetDesc.m_pDocumentType = ezGetStaticRTTI<ezVisualScriptAssetDocument>();
  m_AssetDesc.m_pManager = this;

  ezQtImageCache::GetSingleton()->RegisterTypeImage("Visual Script", QPixmap(":/AssetIcons/Visual_Script.png"));
}

ezVisualScriptAssetManager::~ezVisualScriptAssetManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezVisualScriptAssetManager::OnDocumentManagerEvent, this));
}


ezBitflags<ezAssetDocumentFlags> ezVisualScriptAssetManager::GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const
{
  EZ_ASSERT_DEBUG(pDescriptor->m_pManager == this, "Given type descriptor is not part of this document manager!");
  return ezAssetDocumentFlags::AutoTransformOnSave;
}

void ezVisualScriptAssetManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezVisualScriptAssetDocument>())
      {
        ezQtVisualScriptAssetDocumentWindow* pDocWnd = new ezQtVisualScriptAssetDocumentWindow(e.m_pDocument, e.m_pOpenContext);
      }
    }
    break;
  }
}

ezStatus ezVisualScriptAssetManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocument*& out_pDocument)
{
  out_pDocument = new ezVisualScriptAssetDocument(szPath);

  return ezStatus(EZ_SUCCESS);
}

void ezVisualScriptAssetManager::InternalGetSupportedDocumentTypes(
    ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_AssetDesc);
}
