#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/CustomDataAsset/CustomDataAsset.h>
#include <EditorPluginAssets/CustomDataAsset/CustomDataAssetManager.h>
#include <EditorPluginAssets/CustomDataAsset/CustomDataAssetWindow.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCustomDataAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezCustomDataAssetDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezCustomDataAssetDocumentManager::ezCustomDataAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezCustomDataAssetDocumentManager::OnDocumentManagerEvent, this));

  // \todo icon
  //ezQtImageCache::GetSingleton()->RegisterTypeImage("CustomData", QPixmap(":/AssetIcons/Surface.svg"));
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
  const ezRTTI* pDataType = ezRTTI::FindTypeByName(sDocumentTypeName);
  EZ_ASSERT_DEV(pDataType != nullptr, "Type '{0}' not found", sDocumentTypeName);
  out_pDocument = new ezCustomDataAssetDocument(pDataType, sPath);
}

void ezCustomDataAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  m_DocTypeDescs.Clear();
  ezRTTI::ForEachDerivedType<ezCustomData>([&] (const ezRTTI* pDataType)
    {
      if (pDataType != ezGetStaticRTTI<ezCustomData>())
      {
        ezAssetDocumentTypeDescriptor& docTypeDesc = m_DocTypeDescs.ExpandAndGetRef();
        docTypeDesc.m_sDocumentTypeName = pDataType->GetTypeName();
        docTypeDesc.m_sFileExtension = "ezCustomData";
        docTypeDesc.m_sIcon = ":/AssetIcons/Surface.svg"; // \todo
        docTypeDesc.m_sAssetCategory = "CustomData";
        docTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezCustomDataAssetDocument>();
        docTypeDesc.m_pManager = const_cast<ezCustomDataAssetDocumentManager*>(this);
        docTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_CustomData"); // \todo should only be compatible with same type

        docTypeDesc.m_sResourceFileExtension = "ezCustomData";
        docTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::AutoTransformOnSave;
      }
    },
    ezRTTI::ForEachOptions::ExcludeAbstract | ezRTTI::ForEachOptions::ExcludeNonAllocatable
  );

  for (const auto& docTypeDesc : m_DocTypeDescs)
    inout_DocumentTypes.PushBack(&docTypeDesc);
}
