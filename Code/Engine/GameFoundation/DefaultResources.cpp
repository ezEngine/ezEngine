#include <GameFoundation/PCH.h>
#include <GameFoundation/GameApplication.h>

#include <RendererCore/Meshes/MeshResource.h>
#include <RendererCore/Textures/TextureResource.h>
#include <RendererCore/Material/MaterialResource.h>

void ezGameApplication::SetupDefaultResources()
{
  ezTextureResourceHandle hFallbackTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/LoadingTexture_D.dds");
  ezTextureResourceHandle hMissingTexture = ezResourceManager::LoadResource<ezTextureResource>("Textures/MissingTexture_D.dds");
  ezMaterialResourceHandle hMissingMaterial = ezResourceManager::LoadResource<ezMaterialResource>("Materials/BaseMaterials/MissingMaterial.ezMaterial");
  ezMaterialResourceHandle hFallbackMaterial = ezResourceManager::LoadResource<ezMaterialResource>("Materials/BaseMaterials/LoadingMaterial.ezMaterial");
  ezMeshResourceHandle hMissingMesh = ezResourceManager::LoadResource<ezMeshResource>("Meshes/MissingMesh/MissingMesh.ezMesh");

  ezTextureResource::SetTypeFallbackResource(hFallbackTexture);
  ezTextureResource::SetTypeMissingResource(hMissingTexture);
  ezMaterialResource::SetTypeFallbackResource(hFallbackMaterial);
  ezMaterialResource::SetTypeMissingResource(hMissingMaterial);
  ezMeshResource::SetTypeMissingResource(hMissingMesh);
}


