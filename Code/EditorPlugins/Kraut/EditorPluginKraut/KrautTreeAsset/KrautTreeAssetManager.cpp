#include <EditorPluginKraut/EditorPluginKrautPCH.h>

#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAssetManager.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAssetWindow.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezKrautTreeAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezKrautTreeAssetDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezKrautTreeAssetDocumentManager::ezKrautTreeAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezKrautTreeAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Kraut Tree";
  m_DocTypeDesc.m_sFileExtension = "ezKrautTreeAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Kraut_Tree.svg";
  m_DocTypeDesc.m_sAssetCategory = "Terrain";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezKrautTreeAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Kraut_Tree");

  m_DocTypeDesc.m_sResourceFileExtension = "ezBinKrautTree";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::SupportsThumbnail;
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
        new ezQtKrautTreeAssetDocumentWindow(static_cast<ezKrautTreeAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void ezKrautTreeAssetDocumentManager::InternalCreateDocument(
  ezStringView sDocumentTypeName, ezStringView sPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext)
{
  out_pDocument = new ezKrautTreeAssetDocument(sPath);
}

void ezKrautTreeAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
