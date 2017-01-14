#pragma once

#include <Core/World/World.h>
#include <RendererCore/Meshes/MeshComponent.h>

class ezSkyBoxComponent;
typedef ezComponentManager<ezSkyBoxComponent, true> ezSkyBoxComponentManager;

class EZ_RENDERERCORE_DLL ezSkyBoxComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSkyBoxComponent, ezRenderComponent, ezSkyBoxComponentManager);

public:
  ezSkyBoxComponent();
  ~ezSkyBoxComponent();

  virtual void Initialize() override;

  // ezRenderComponent interface
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds) override;

  void OnExtractRenderData(ezExtractRenderDataMessage& msg) const;

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  void SetExposureBias(float fExposureBias);
  float GetExposureBias() const { return m_fExposureBias; }

  void SetInverseTonemap(bool bInverseTonemap);
  bool GetInverseTonemap() const { return m_bInverseTonemap; }

  void SetCubeMap(const char* szFile);
  const char* GetCubeMap() const;

private:

  void UpdateMaterials();

  float m_fExposureBias;
  bool m_bInverseTonemap;

  ezTexture2DResourceHandle m_hCubeMap;

  ezMeshResourceHandle m_hMesh;
  ezMaterialResourceHandle m_hCubeMapMaterial;
};

