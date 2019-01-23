#include <PCH.h>

#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAsset.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAssetManager.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAssetWindow.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezKrautTreeAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezKrautTreeAssetDocumentManager>);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezKrautTreeAssetDocumentManager::ezKrautTreeAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezKrautTreeAssetDocumentManager::OnDocumentManagerEvent, this));

  m_AssetDesc.m_bCanCreate = true;
  m_AssetDesc.m_sDocumentTypeName = "Kraut Tree Asset";
  m_AssetDesc.m_sFileExtension = "ezKrautTreeAsset";
  m_AssetDesc.m_sIcon = ":/AssetIcons/Kraut_Tree.png";
  m_AssetDesc.m_pDocumentType = ezGetStaticRTTI<ezKrautTreeAssetDocument>();
  m_AssetDesc.m_pManager = this;
}

ezKrautTreeAssetDocumentManager::~ezKrautTreeAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezKrautTreeAssetDocumentManager::OnDocumentManagerEvent, this));
}

void ezKrautTreeAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezKrautTreeAssetDocument>())
      {
        ezQtKrautTreeAssetDocumentWindow* pDocWnd =
            new ezQtKrautTreeAssetDocumentWindow(static_cast<ezKrautTreeAssetDocument*>(e.m_pDocument));
      }
    }
    break;
  }
}

ezStatus ezKrautTreeAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath,
                                                                     bool bCreateNewDocument, ezDocument*& out_pDocument)
{
  out_pDocument = new ezKrautTreeAssetDocument(szPath);

  return ezStatus(EZ_SUCCESS);
}

void ezKrautTreeAssetDocumentManager::InternalGetSupportedDocumentTypes(
    ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_AssetDesc);
}

ezBitflags<ezAssetDocumentFlags>
ezKrautTreeAssetDocumentManager::GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const
{
  EZ_ASSERT_DEBUG(pDescriptor->m_pManager == this, "Given type descriptor is not part of this document manager!");
  return ezAssetDocumentFlags::SupportsThumbnail;
}

