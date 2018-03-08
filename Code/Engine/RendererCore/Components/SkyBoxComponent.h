#pragma once

#include <Core/World/World.h>
#include <RendererCore/Meshes/MeshComponent.h>

typedef ezComponentManager<class ezSkyBoxComponent, ezBlockStorageType::Compact> ezSkyBoxComponentManager;
typedef ezTypedResourceHandle<class ezTextureCubeResource> ezTextureCubeResourceHandle;

class EZ_RENDERERCORE_DLL ezSkyBoxComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSkyBoxComponent, ezRenderComponent, ezSkyBoxComponentManager);

public:
  ezSkyBoxComponent();
  ~ezSkyBoxComponent();

  virtual void Initialize() override;

  // ezRenderComponent interface
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) override;

  void OnExtractRenderData(ezMsgExtractRenderData& msg) const;

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  void SetExposureBias(float fExposureBias);
  float GetExposureBias() const { return m_fExposureBias; }

  void SetInverseTonemap(bool bInverseTonemap);
  bool GetInverseTonemap() const { return m_bInverseTonemap; }

  void SetUseFog(bool bUseFog);
  bool GetUseFog() const { return m_bUseFog; }

  void SetVirtualDistance(float fVirtualDistance);
  float GetVirtualDistance() const { return m_fVirtualDistance; }

  void SetCubeMapFile(const char* szFile);
  const char* GetCubeMapFile() const;

  void SetCubeMap(const ezTextureCubeResourceHandle& hCubeMap);
  const ezTextureCubeResourceHandle& GetCubeMap() const;

  virtual void OnActivated() override;

private:

  void UpdateMaterials();

  float m_fExposureBias;
  float m_fVirtualDistance;
  bool m_bInverseTonemap;
  bool m_bUseFog;

  ezTextureCubeResourceHandle m_hCubeMap;

  ezMeshResourceHandle m_hMesh;
  ezMaterialResourceHandle m_hCubeMapMaterial;
};

