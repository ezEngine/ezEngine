#include <EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAssetObjects.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimatedMeshAssetProperties, 2, ezRTTIDefaultAllocator<ezAnimatedMeshAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("DefaultSkeleton", m_sDefaultSkeleton)->AddAttributes(new ezAssetBrowserAttribute("Skeleton")),
    EZ_MEMBER_PROPERTY("MeshFile", m_sMeshFile)->AddAttributes(new ezFileBrowserAttribute("Select Mesh", "*.fbx;*.gltf;*.glb")),
    EZ_ENUM_MEMBER_PROPERTY("RightDir", ezBasisAxis, m_RightDir)->AddAttributes(new ezDefaultValueAttribute((int)ezBasisAxis::PositiveX)),
    EZ_ENUM_MEMBER_PROPERTY("UpDir", ezBasisAxis, m_UpDir)->AddAttributes(new ezDefaultValueAttribute((int)ezBasisAxis::PositiveY)),
    EZ_MEMBER_PROPERTY("FlipForwardDir", m_bFlipForwardDir),
    EZ_MEMBER_PROPERTY("UniformScaling", m_fUniformScaling)->AddAttributes(new ezDefaultValueAttribute(1.0f), new ezClampValueAttribute(0.0001f, 10000.0f)),
    EZ_MEMBER_PROPERTY("RecalculateNormals", m_bRecalculateNormals),
    EZ_MEMBER_PROPERTY("RecalculateTangents", m_bRecalculateTrangents)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ENUM_MEMBER_PROPERTY("NormalPrecision", ezMeshNormalPrecision, m_NormalPrecision),
    EZ_ENUM_MEMBER_PROPERTY("TexCoordPrecision", ezMeshTexCoordPrecision, m_TexCoordPrecision),
    EZ_MEMBER_PROPERTY("ImportMaterials", m_bImportMaterials)->AddAttributes(new ezDefaultValueAttribute(true)),
    EZ_ARRAY_MEMBER_PROPERTY("Materials", m_Slots)->AddAttributes(new ezContainerAttribute(false, true, true)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezAnimatedMeshAssetProperties::ezAnimatedMeshAssetProperties() = default;
ezAnimatedMeshAssetProperties::~ezAnimatedMeshAssetProperties() = default;
