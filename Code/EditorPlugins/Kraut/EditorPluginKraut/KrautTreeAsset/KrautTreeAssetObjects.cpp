#include <EditorPluginKraut/EditorPluginKrautPCH.h>

#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAssetObjects.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezKrautAssetMaterial, ezNoBase, 1, ezRTTIDefaultAllocator<ezKrautAssetMaterial>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Label", m_sLabel)->AddAttributes(new ezReadOnlyAttribute()),
    EZ_MEMBER_PROPERTY("Material", m_sMaterial)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Material", "Kraut")),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezKrautTreeAssetProperties, 1, ezRTTIDefaultAllocator<ezKrautTreeAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("KrautFile", m_sKrautFile)->AddAttributes(new ezFileBrowserAttribute("Select Kraut Tree file", "*.tree")),
    EZ_MEMBER_PROPERTY("UniformScaling", m_fUniformScaling)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("LodDistanceScale", m_fLodDistanceScale)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("StaticColliderRadius", m_fStaticColliderRadius)->AddAttributes(new ezDefaultValueAttribute(0.4f), new ezClampValueAttribute(0.0f, 10.0f)),
    EZ_MEMBER_PROPERTY("TreeStiffness", m_fTreeStiffness)->AddAttributes(new ezDefaultValueAttribute(10.0f), new ezClampValueAttribute(1.0f, 10000.0f)),
    EZ_MEMBER_PROPERTY("Surface", m_sSurface)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Surface", ezDependencyFlags::Package)),
    EZ_ARRAY_MEMBER_PROPERTY("Materials", m_Materials)->AddAttributes(new ezContainerAttribute(false, false, false)),
    EZ_MEMBER_PROPERTY("DisplayRandomSeed", m_uiRandomSeedForDisplay),
    EZ_ARRAY_MEMBER_PROPERTY("GoodRandomSeeds", m_GoodRandomSeeds),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezKrautTreeAssetProperties::ezKrautTreeAssetProperties() = default;
ezKrautTreeAssetProperties::~ezKrautTreeAssetProperties() = default;
