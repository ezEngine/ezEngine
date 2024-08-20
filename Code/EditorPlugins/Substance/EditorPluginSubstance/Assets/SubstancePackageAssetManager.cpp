#include <EditorPluginSubstance/EditorPluginSubstancePCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginSubstance/Assets/SubstancePackageAsset.h>
#include <EditorPluginSubstance/Assets/SubstancePackageAssetManager.h>
#include <EditorPluginSubstance/Assets/SubstancePackageAssetWindow.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSubstancePackageAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezSubstancePackageAssetDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezSubstancePackageAssetDocumentManager::ezSubstancePackageAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezSubstancePackageAssetDocumentManager::OnDocumentManagerEvent, this));

  {
    m_PackageTypeDesc.m_sDocumentTypeName = "Substance Package";
    m_PackageTypeDesc.m_sFileExtension = "ezSubstancePackageAsset";
    m_PackageTypeDesc.m_sIcon = ":/AssetIcons/SubstanceDesigner.svg";
    m_PackageTypeDesc.m_sAssetCategory = "Rendering";
    m_PackageTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezSubstancePackageAssetDocument>();
    m_PackageTypeDesc.m_pManager = this;
    m_PackageTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Substance_Package");

    m_PackageTypeDesc.m_sResourceFileExtension = "ezBinSubstancePackage";
    m_PackageTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::SubAssetsAutoThumbnailOnTransform;

    ezQtImageCache::GetSingleton()->RegisterTypeImage("Substance Package", QPixmap(":/AssetIcons/SubstanceDesigner.svg"));
  }

  {
    m_TextureTypeDesc.m_bCanCreate = false;
    m_TextureTypeDesc.m_sDocumentTypeName = "Substance Texture";
    m_TextureTypeDesc.m_sFileExtension = "ezSubstanceTextureAsset";
    m_TextureTypeDesc.m_sIcon = ":/AssetIcons/SubstanceDesigner.svg";
    m_TextureTypeDesc.m_sAssetCategory = "Rendering";
    m_TextureTypeDesc.m_pManager = this;
    m_TextureTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Texture_2D");

    m_TextureTypeDesc.m_sResourceFileExtension = "ezBinTexture2D";
    m_TextureTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::AutoThumbnailOnTransform;
  }
}

ezSubstancePackageAssetDocumentManager::~ezSubstancePackageAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezSubstancePackageAssetDocumentManager::OnDocumentManagerEvent, this));
}

void ezSubstancePackageAssetDocumentManager::FillOutSubAssetList(const ezAssetDocumentInfo& assetInfo, ezDynamicArray<ezSubAssetData>& out_subAssets) const
{
  auto pMetaData = assetInfo.GetMetaInfo<ezSubstancePackageAssetMetaData>();
  if (pMetaData == nullptr)
    return;

  for (ezUInt32 i = 0; i < pMetaData->m_OutputUuids.GetCount(); ++i)
  {
    auto& subAsset = out_subAssets.ExpandAndGetRef();
    subAsset.m_Guid = pMetaData->m_OutputUuids[i];
    subAsset.m_sName = pMetaData->m_OutputNames[i];
    subAsset.m_sSubAssetsDocumentTypeName.Assign(m_TextureTypeDesc.m_sDocumentTypeName);
  }
}

ezString ezSubstancePackageAssetDocumentManager::GetAssetTableEntry(const ezSubAsset* pSubAsset, ezStringView sDataDirectory, const ezPlatformProfile* pAssetProfile) const
{
  if (pSubAsset->m_bMainAsset)
  {
    return SUPER::GetAssetTableEntry(pSubAsset, sDataDirectory, pAssetProfile);
  }

  ezStringBuilder sTargetFile = pSubAsset->m_pAssetInfo->m_Path.GetAbsolutePath().GetFileDirectory();
  sTargetFile.Append(pSubAsset->m_Data.m_sName);

  return GetRelativeOutputFileName(&m_TextureTypeDesc, sDataDirectory, sTargetFile, "", pAssetProfile);
}

void ezSubstancePackageAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezSubstancePackageAssetDocument>())
      {
        new ezQtSubstancePackageAssetWindow(e.m_pDocument); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void ezSubstancePackageAssetDocumentManager::InternalCreateDocument(ezStringView sDocumentTypeName, ezStringView sPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext)
{
  out_pDocument = new ezSubstancePackageAssetDocument(sPath);
}

void ezSubstancePackageAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_PackageTypeDesc);
  inout_DocumentTypes.PushBack(&m_TextureTypeDesc);
}
