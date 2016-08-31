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

  void SetLeftTextureFile(const char* szFile) { SetTextureFile(4, szFile); }
  const char* GetLeftTextureFile() const { return GetTextureFile(4); }

  void SetFrontTextureFile(const char* szFile) { SetTextureFile(2, szFile); }
  const char* GetFrontTextureFile() const { return GetTextureFile(2); }

  void SetRightTextureFile(const char* szFile) { SetTextureFile(5, szFile); }
  const char* GetRightTextureFile() const { return GetTextureFile(5); }

  void SetBackTextureFile(const char* szFile) { SetTextureFile(3, szFile); }
  const char* GetBackTextureFile() const { return GetTextureFile(3); }

  void SetUpTextureFile(const char* szFile) { SetTextureFile(0, szFile); }
  const char* GetUpTextureFile() const { return GetTextureFile(0); }

  void SetDownTextureFile(const char* szFile) { SetTextureFile(1, szFile); }
  const char* GetDownTextureFile() const { return GetTextureFile(1); }

private:

  void SetTextureFile(ezUInt32 uiIndex, const char* szFile);
  const char* GetTextureFile(ezUInt32 uiIndex) const;
  void UpdateMaterials();

  float m_fExposureBias;
  bool m_bInverseTonemap;

  ezTextureResourceHandle m_Textures[6];

  ezMeshResourceHandle m_hMesh;
  ezMaterialResourceHandle m_Materials[6];
};

