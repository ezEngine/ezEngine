#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAssetObjects.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimatedMeshAssetProperties, 3, ezRTTIDefaultAllocator<ezAnimatedMeshAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("MeshFile", m_sMeshFile)->AddAttributes(new ezFileBrowserAttribute("Select Mesh", ezFileBrowserAttribute::MeshesWithAnimations)),
    EZ_MEMBER_PROPERTY("MeshIncludeTags", m_sMeshIncludeTags),
    EZ_MEMBER_PROPERTY("MeshExcludeTags", m_sMeshExcludeTags)->AddAttributes(new ezDefaultValueAttribute("$;UCX_")),
    EZ_MEMBER_PROPERTY("DefaultSkeleton", m_sDefaultSkeleton)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Mesh_Skeleton")),
    EZ_MEMBER_PROPERTY("RecalculateNormals", m_bRecalculateNormals),
    EZ_MEMBER_PROPERTY("RecalculateTangents", m_bRecalculateTangents)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_MEMBER_PROPERTY("HighPrecision", m_bHighPrecision),
    EZ_ENUM_MEMBER_PROPERTY("VertexColorConversion", ezMeshVertexColorConversion, m_VertexColorConversion),
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

//////////////////////////////////////////////////////////////////////////

class ezAnimatedMeshAssetPropertiesPatch_2_3 : public ezGraphPatch
{
public:
  ezAnimatedMeshAssetPropertiesPatch_2_3()
    : ezGraphPatch("ezAnimatedMeshAssetProperties", 3)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    bool bHighPrecision = false;

    if (auto pProp = pNode->FindProperty("NormalPrecision"))
    {
      if (pProp->m_Value.IsA<ezString>() && pProp->m_Value.Get<ezString>() != "ezMeshNormalPrecision::_10Bit")
      {
        bHighPrecision = true;
      }
    }

    if (auto pProp = pNode->FindProperty("TexCoordPrecision"))
    {
      if (pProp->m_Value.IsA<ezString>() && pProp->m_Value.Get<ezString>() != "ezMeshTexCoordPrecision::_16Bit")
      {
        bHighPrecision = true;
      }
    }

    if (auto pProp = pNode->FindProperty("BoneWeightPrecision"))
    {
      if (pProp->m_Value.IsA<ezString>() && pProp->m_Value.Get<ezString>() != "ezMeshBoneWeigthPrecision::_8Bit")
      {
        bHighPrecision = true;
      }
    }

    pNode->AddProperty("HighPrecision", bHighPrecision);
  }
};

ezAnimatedMeshAssetPropertiesPatch_2_3 g_ezAnimatedMeshAssetPropertiesPatch_2_3;
