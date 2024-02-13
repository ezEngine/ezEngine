#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocumentInfo.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetDocumentInfo, 2, ezRTTIDefaultAllocator<ezAssetDocumentInfo>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_SET_MEMBER_PROPERTY("Dependencies", m_TransformDependencies),
    EZ_SET_MEMBER_PROPERTY("References", m_ThumbnailDependencies),
    EZ_SET_MEMBER_PROPERTY("PackageDeps", m_PackageDependencies),
    EZ_SET_MEMBER_PROPERTY("Outputs", m_Outputs),
    EZ_MEMBER_PROPERTY("Hash", m_uiSettingsHash),
    EZ_ACCESSOR_PROPERTY("AssetType", GetAssetsDocumentTypeName, SetAssetsDocumentTypeName),
    EZ_ACCESSOR_PROPERTY("Tags", GetAssetsDocumentTags, SetAssetsDocumentTags),
    EZ_ARRAY_MEMBER_PROPERTY("MetaInfo", m_MetaInfo)->AddFlags(ezPropertyFlags::PointerOwner),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezAssetDocumentInfo::ezAssetDocumentInfo()
{
  m_uiSettingsHash = 0;
}

ezAssetDocumentInfo::~ezAssetDocumentInfo()
{
  ClearMetaData();
}

ezAssetDocumentInfo::ezAssetDocumentInfo(ezAssetDocumentInfo&& rhs)
{
  (*this) = std::move(rhs);
}

void ezAssetDocumentInfo::operator=(ezAssetDocumentInfo&& rhs)
{
  m_uiSettingsHash = rhs.m_uiSettingsHash;
  m_TransformDependencies = rhs.m_TransformDependencies;
  m_ThumbnailDependencies = rhs.m_ThumbnailDependencies;
  m_PackageDependencies = rhs.m_PackageDependencies;
  m_Outputs = rhs.m_Outputs;
  m_sAssetsDocumentTypeName = rhs.m_sAssetsDocumentTypeName;
  m_sAssetsDocumentTags = rhs.m_sAssetsDocumentTags;
  m_MetaInfo = std::move(rhs.m_MetaInfo);
}

void ezAssetDocumentInfo::CreateShallowClone(ezAssetDocumentInfo& rhs) const
{
  rhs.m_uiSettingsHash = m_uiSettingsHash;
  rhs.m_TransformDependencies = m_TransformDependencies;
  rhs.m_ThumbnailDependencies = m_ThumbnailDependencies;
  rhs.m_PackageDependencies = m_PackageDependencies;
  rhs.m_Outputs = m_Outputs;
  rhs.m_sAssetsDocumentTypeName = m_sAssetsDocumentTypeName;
  rhs.m_sAssetsDocumentTags = m_sAssetsDocumentTags;
  rhs.m_MetaInfo.Clear();
}

void ezAssetDocumentInfo::ClearMetaData()
{
  for (auto* pObj : m_MetaInfo)
  {
    pObj->GetDynamicRTTI()->GetAllocator()->Deallocate(pObj);
  }
  m_MetaInfo.Clear();
}

const char* ezAssetDocumentInfo::GetAssetsDocumentTypeName() const
{
  return m_sAssetsDocumentTypeName.GetData();
}

const ezString& ezAssetDocumentInfo::GetAssetsDocumentTags() const
{
  return m_sAssetsDocumentTags;
}

void ezAssetDocumentInfo::SetAssetsDocumentTypeName(const char* szSz)
{
  m_sAssetsDocumentTypeName.Assign(szSz);
}

void ezAssetDocumentInfo::SetAssetsDocumentTags(const ezString& sTags)
{
  m_sAssetsDocumentTags = sTags;
}

const ezReflectedClass* ezAssetDocumentInfo::GetMetaInfo(const ezRTTI* pType) const
{
  for (auto* pObj : m_MetaInfo)
  {
    if (pObj->GetDynamicRTTI()->IsDerivedFrom(pType))
      return pObj;
  }
  return nullptr;
}
