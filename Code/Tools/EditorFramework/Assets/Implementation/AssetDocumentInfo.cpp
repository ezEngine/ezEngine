#include <EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocumentInfo.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetDocumentInfo, 2, ezRTTIDefaultAllocator<ezAssetDocumentInfo>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_SET_MEMBER_PROPERTY("Dependencies", m_AssetTransformDependencies),
    EZ_SET_MEMBER_PROPERTY("References", m_RuntimeDependencies),
    EZ_SET_MEMBER_PROPERTY("Outputs", m_Outputs),
    EZ_MEMBER_PROPERTY("Hash", m_uiSettingsHash),
    EZ_ACCESSOR_PROPERTY("AssetType", GetAssetTypeName, SetAssetTypeName),
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
  m_AssetTransformDependencies = rhs.m_AssetTransformDependencies;
  m_RuntimeDependencies = rhs.m_RuntimeDependencies;
  m_Outputs = rhs.m_Outputs;
  m_sAssetTypeName = rhs.m_sAssetTypeName;
  m_MetaInfo = std::move(rhs.m_MetaInfo);
}

void ezAssetDocumentInfo::CreateShallowClone(ezAssetDocumentInfo& rhs) const
{
  rhs.m_uiSettingsHash = m_uiSettingsHash;
  rhs.m_AssetTransformDependencies = m_AssetTransformDependencies;
  rhs.m_RuntimeDependencies = m_RuntimeDependencies;
  rhs.m_Outputs = m_Outputs;
  rhs.m_sAssetTypeName = m_sAssetTypeName;
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

const char* ezAssetDocumentInfo::GetAssetTypeName() const
{
  return m_sAssetTypeName.GetData();
}

void ezAssetDocumentInfo::SetAssetTypeName(const char* sz)
{
  m_sAssetTypeName.Assign(sz);
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
