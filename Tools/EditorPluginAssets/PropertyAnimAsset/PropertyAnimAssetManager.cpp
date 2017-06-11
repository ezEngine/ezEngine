#include <PCH.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAssetManager.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAsset.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAssetWindow.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPropertyAnimAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezPropertyAnimAssetDocumentManager>);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezPropertyAnimAssetDocumentManager::ezPropertyAnimAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezPropertyAnimAssetDocumentManager::OnDocumentManagerEvent, this));

  m_AssetDesc.m_bCanCreate = true;
  m_AssetDesc.m_sDocumentTypeName = "PropertyAnim Asset";
  m_AssetDesc.m_sFileExtension = "ezPropertyAnimAsset";
  m_AssetDesc.m_sIcon = ":/AssetIcons/PropertyAnim.png";
  m_AssetDesc.m_pDocumentType = ezGetStaticRTTI<ezPropertyAnimAssetDocument>();
  m_AssetDesc.m_pManager = this;

  ezQtImageCache::GetSingleton()->RegisterTypeImage("PropertyAnim", QPixmap(":/AssetIcons/PropertyAnim.png"));
}

ezPropertyAnimAssetDocumentManager::~ezPropertyAnimAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezPropertyAnimAssetDocumentManager::OnDocumentManagerEvent, this));
}


ezBitflags<ezAssetDocumentFlags> ezPropertyAnimAssetDocumentManager::GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const
{
  EZ_ASSERT_DEBUG(pDescriptor->m_pManager == this, "Given type descriptor is not part of this document manager!");
  return ezAssetDocumentFlags::AutoTransformOnSave;
}

void ezPropertyAnimAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
  case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezPropertyAnimAssetDocument>())
      {
        ezQtPropertyAnimAssetDocumentWindow* pDocWnd = new ezQtPropertyAnimAssetDocumentWindow(e.m_pDocument);
      }
    }
    break;
  }
}

ezStatus ezPropertyAnimAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocument*& out_pDocument)
{
  out_pDocument = new ezPropertyAnimAssetDocument(szPath);

  return ezStatus(EZ_SUCCESS);
}

void ezPropertyAnimAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_AssetDesc);
}



