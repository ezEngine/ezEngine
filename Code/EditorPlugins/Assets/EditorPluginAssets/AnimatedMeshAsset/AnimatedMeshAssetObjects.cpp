#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAssetObjects.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimatedMeshAssetProperties, 2, ezRTTIDefaultAllocator<ezAnimatedMeshAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("MeshFile", m_sMeshFile)->AddAttributes(new ezFileBrowserAttribute("Select Mesh", ezFileBrowserAttribute::MeshesWithAnimations)),
    EZ_MEMBER_PROPERTY("DefaultSkeleton", m_sDefaultSkeleton)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Mesh_Skeleton")),
    EZ_MEMBER_PROPERTY("RecalculateNormals", m_bRecalculateNormals),
    EZ_MEMBER_PROPERTY("RecalculateTangents", m_bRecalculateTrangents)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ENUM_MEMBER_PROPERTY("NormalPrecision", ezMeshNormalPrecision, m_NormalPrecision),
    EZ_ENUM_MEMBER_PROPERTY("TexCoordPrecision", ezMeshTexCoordPrecision, m_TexCoordPrecision),
    EZ_ENUM_MEMBER_PROPERTY("VertexColorConversion", ezMeshVertexColorConversion, m_VertexColorConversion),
    EZ_ENUM_MEMBER_PROPERTY("BoneWeightPrecision", ezMeshBoneWeigthPrecision, m_BoneWeightPrecision),
    EZ_MEMBER_PROPERTY("NormalizeWeights", m_bNormalizeWeights)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("ImportMaterials", m_bImportMaterials),
    EZ_ARRAY_MEMBER_PROPERTY("Materials", m_Slots)->AddAttributes(new ezContainerAttribute(false, true, true)),
    EZ_MEMBER_PROPERTY("SimplifyMesh", m_bSimplifyMesh),
    EZ_MEMBER_PROPERTY("MeshSimplification", m_uiMeshSimplification)->AddAttributes(new ezDefaultValueAttribute(50), new ezClampValueAttribute(1, 100)),
    EZ_MEMBER_PROPERTY("MaxSimplificationError", m_uiMaxSimplificationError)->AddAttributes(new ezDefaultValueAttribute(5), new ezClampValueAttribute(1, 100)),
    EZ_MEMBER_PROPERTY("AggressiveSimplification", m_bAggressiveSimplification),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezAnimatedMeshAssetProperties::ezAnimatedMeshAssetProperties() = default;
ezAnimatedMeshAssetProperties::~ezAnimatedMeshAssetProperties() = default;

void ezAnimatedMeshAssetProperties::PropertyMetaStateEventHandler(ezPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == ezGetStaticRTTI<ezAnimatedMeshAssetProperties>())
  {
    const bool bSimplify = e.m_pObject->GetTypeAccessor().GetValue("SimplifyMesh").ConvertTo<bool>();

    auto& props = *e.m_pPropertyStates;

    props["MeshSimplification"].m_Visibility = bSimplify ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["MaxSimplificationError"].m_Visibility = bSimplify ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
    props["AggressiveSimplification"].m_Visibility = bSimplify ? ezPropertyUiState::Default : ezPropertyUiState::Invisible;
  }
}
