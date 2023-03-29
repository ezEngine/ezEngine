#pragma once

#include <Core/World/World.h>
#include <RendererCore/Meshes/MeshComponent.h>

typedef ezComponentManager<class ezSkyBoxComponent, ezBlockStorageType::Compact> ezSkyBoxComponentManager;
using ezTextureCubeResourceHandle = ezTypedResourceHandle<class ezTextureCubeResource>;

class EZ_RENDERERCORE_DLL ezSkyBoxComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSkyBoxComponent, ezRenderComponent, ezSkyBoxComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void Initialize() override;
  virtual void OnActivated() override;


  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent

public:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg) override;


  //////////////////////////////////////////////////////////////////////////
  // ezSkyBoxComponent

public:
  ezSkyBoxComponent();
  ~ezSkyBoxComponent();

  void SetExposureBias(float fExposureBias);                // [ property ]
  float GetExposureBias() const { return m_fExposureBias; } // [ property ]

  void SetInverseTonemap(bool bInverseTonemap);                // [ property ]
  bool GetInverseTonemap() const { return m_bInverseTonemap; } // [ property ]

  void SetUseFog(bool bUseFog);                // [ property ]
  bool GetUseFog() const { return m_bUseFog; } // [ property ]

  void SetVirtualDistance(float fVirtualDistance);                // [ property ]
  float GetVirtualDistance() const { return m_fVirtualDistance; } // [ property ]

  void SetCubeMapFile(const char* szFile); // [ property ]
  const char* GetCubeMapFile() const;      // [ property ]

  void SetCubeMap(const ezTextureCubeResourceHandle& hCubeMap);
  const ezTextureCubeResourceHandle& GetCubeMap() const;

private:
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;
  void UpdateMaterials();

  float m_fExposureBias = 0.0f;
  float m_fVirtualDistance = 1000.0f;
  bool m_bInverseTonemap = false;
  bool m_bUseFog = true;

  ezTextureCubeResourceHandle m_hCubeMap;

  ezMeshResourceHandle m_hMesh;
  ezMaterialResourceHandle m_hCubeMapMaterial;
};
