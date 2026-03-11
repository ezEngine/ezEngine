#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/BlackboardTemplateAsset/BlackboardTemplateAsset.h>
#include <EditorPluginAssets/BlackboardTemplateAsset/BlackboardTemplateAssetManager.h>
#include <EditorPluginAssets/BlackboardTemplateAsset/BlackboardTemplateAssetWindow.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezBlackboardTemplateAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezBlackboardTemplateAssetDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezBlackboardTemplateAssetDocumentManager::ezBlackboardTemplateAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezBlackboardTemplateAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "BlackboardTemplate";
  m_DocTypeDesc.m_sFileExtension = "ezBlackboardTemplateAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/BlackboardTemplate.svg";
  m_DocTypeDesc.m_sAssetCategory = "Logic";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezBlackboardTemplateAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_BlackboardTemplate");

  m_DocTypeDesc.m_sResourceFileExtension = "ezBinBlackboardTemplate";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::AutoTransformOnSave;

  ezQtImageCache::GetSingleton()->RegisterTypeImage("BlackboardTemplate", QPixmap(":/AssetIcons/BlackboardTemplate.svg"));
}

ezBlackboardTemplateAssetDocumentManager::~ezBlackboardTemplateAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezBlackboardTemplateAssetDocumentManager::OnDocumentManagerEvent, this));
}

void ezBlackboardTemplateAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezBlackboardTemplateAssetDocument>())
      {
        new ezQtBlackboardTemplateAssetDocumentWindow(e.m_pDocument); // NOLINT: not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void ezBlackboardTemplateAssetDocumentManager::InternalCreateDocument(ezStringView sDocumentTypeName, ezStringView sPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext)
{
  out_pDocument = new ezBlackboardTemplateAssetDocument(sPath);
}

void ezBlackboardTemplateAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
