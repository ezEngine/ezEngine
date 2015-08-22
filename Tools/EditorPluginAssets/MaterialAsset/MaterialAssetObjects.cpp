#include <PCH.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetObjects.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMaterialAssetProperties, ezReflectedClass, 1, ezRTTIDefaultAllocator<ezMaterialAssetProperties>);
  EZ_BEGIN_PROPERTIES
    EZ_MEMBER_PROPERTY("Base Material", m_sBaseMaterial)->AddAttributes(new ezAssetBrowserAttribute("Material")),
    EZ_MEMBER_PROPERTY("Shader", m_sShader),
    EZ_MEMBER_PROPERTY("Permutations", m_sPermutationVarValues),
    EZ_MEMBER_PROPERTY("Diffuse Texture", m_sTextureDiffuse)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D")),
    EZ_MEMBER_PROPERTY("Mask Texture", m_sTextureMask)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D")),
    EZ_MEMBER_PROPERTY("Normal Map", m_sTextureNormal)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D")),
  EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezMaterialAssetProperties::ezMaterialAssetProperties()
{
}

