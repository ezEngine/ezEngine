#include <PCH.h>

#include <RendererCore/Meshes/MeshComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off

EZ_BEGIN_COMPONENT_TYPE(ezMeshComponent, 3, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Mesh", GetMeshFile, SetMeshFile)->AddAttributes(new ezAssetBrowserAttribute("Mesh;Animated Mesh")),
    EZ_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new ezExposeColorAlphaAttribute()),
    EZ_ARRAY_ACCESSOR_PROPERTY("Materials", Materials_GetCount, Materials_GetValue, Materials_SetValue, Materials_Insert, Materials_Remove)->AddAttributes(new ezAssetBrowserAttribute("Material")),
  }
  EZ_END_PROPERTIES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezMeshComponent::ezMeshComponent() = default;
ezMeshComponent::~ezMeshComponent() = default;



EZ_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_MeshComponent);
