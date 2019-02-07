#include <EditorPluginKrautPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAssetObjects.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezKrautAssetMaterial, ezNoBase, 1, ezRTTIDefaultAllocator<ezKrautAssetMaterial>)
{
  EZ_BEGIN_PROPERTIES
  {
    // TODO: make the texture references read-only somehow ?
    EZ_MEMBER_PROPERTY("DiffuseTexture", m_sDiffuseTexture)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D")),
    EZ_MEMBER_PROPERTY("NormalMapTexture", m_sNormalMapTexture)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D")),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezKrautTreeAssetProperties, 1, ezRTTIDefaultAllocator<ezKrautTreeAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("KrautFile", m_sKrautFile)->AddAttributes(new ezFileBrowserAttribute("Select Kraut Tree file", "*.kraut")),
    EZ_MEMBER_PROPERTY("UniformScaling", m_fUniformScaling)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("LodDistanceScale", m_fLodDistanceScale)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("StaticColliderRadius", m_fStaticColliderRadius)->AddAttributes(new ezDefaultValueAttribute(0.4f), new ezClampValueAttribute(0.0f, 10.0f)),
    EZ_MEMBER_PROPERTY("Surface", m_sSurface)->AddAttributes(new ezAssetBrowserAttribute("Surface")),
    EZ_ARRAY_MEMBER_PROPERTY("Materials", m_Materials)->AddAttributes(new ezContainerAttribute(false, false, false)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezKrautTreeAssetProperties::ezKrautTreeAssetProperties() = default;
ezKrautTreeAssetProperties::~ezKrautTreeAssetProperties() = default;
