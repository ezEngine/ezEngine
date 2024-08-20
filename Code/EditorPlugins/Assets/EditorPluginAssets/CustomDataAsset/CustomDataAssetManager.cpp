#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/CustomDataAsset/CustomDataAsset.h>
#include <EditorPluginAssets/CustomDataAsset/CustomDataAssetManager.h>
#include <EditorPluginAssets/CustomDataAsset/CustomDataAssetWindow.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCustomDataAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezCustomDataAssetDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezCustomDataAssetDocumentManager::ezCustomDataAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezCustomDataAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "CustomData";
  m_DocTypeDesc.m_sFileExtension = "ezCustomDataAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/CustomData.svg";
  m_DocTypeDesc.m_sAssetCategory = "Logic";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezCustomDataAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_CustomData"); // \todo should only be compatible with same type

  m_DocTypeDesc.m_sResourceFileExtension = "ezBinCustomData";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::AutoTransformOnSave;

  ezQtImageCache::GetSingleton()->RegisterTypeImage("CustomData", QPixmap(":/AssetIcons/CustomData.svg"));
}

ezCustomDataAssetDocumentManager::~ezCustomDataAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezCustomDataAssetDocumentManager::OnDocumentManagerEvent, this));
}

void ezCustomDataAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezCustomDataAssetDocument>())
      {
        new ezQtCustomDataAssetDocumentWindow(e.m_pDocument); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void ezCustomDataAssetDocumentManager::InternalCreateDocument(ezStringView sDocumentTypeName, ezStringView sPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext)
{
  out_pDocument = new ezCustomDataAssetDocument(sPath);
}

void ezCustomDataAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
