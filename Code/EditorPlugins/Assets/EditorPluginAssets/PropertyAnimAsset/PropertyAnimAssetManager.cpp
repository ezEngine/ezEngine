#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAsset.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAssetManager.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAssetWindow.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPropertyAnimAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezPropertyAnimAssetDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezPropertyAnimAssetDocumentManager::ezPropertyAnimAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezPropertyAnimAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "PropertyAnim";
  m_DocTypeDesc.m_sFileExtension = "ezPropertyAnimAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/PropertyAnim.svg";
  m_DocTypeDesc.m_sAssetCategory = "Animation";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezPropertyAnimAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Property_Animation");

  m_DocTypeDesc.m_sResourceFileExtension = "ezBinPropertyAnim";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::AutoTransformOnSave;

  ezQtImageCache::GetSingleton()->RegisterTypeImage("PropertyAnim", QPixmap(":/AssetIcons/PropertyAnim.svg"));
}

ezPropertyAnimAssetDocumentManager::~ezPropertyAnimAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezPropertyAnimAssetDocumentManager::OnDocumentManagerEvent, this));
}

void ezPropertyAnimAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezPropertyAnimAssetDocument>())
      {
        new ezQtPropertyAnimAssetDocumentWindow(static_cast<ezPropertyAnimAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void ezPropertyAnimAssetDocumentManager::InternalCreateDocument(
  ezStringView sDocumentTypeName, ezStringView sPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext)
{
  out_pDocument = new ezPropertyAnimAssetDocument(sPath);
}

void ezPropertyAnimAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
