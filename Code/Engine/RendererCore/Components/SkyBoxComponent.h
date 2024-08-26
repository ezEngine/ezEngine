#pragma once

#include <Core/World/World.h>
#include <RendererCore/Meshes/MeshComponent.h>

using ezSkyBoxComponentManager = ezComponentManager<class ezSkyBoxComponent, ezBlockStorageType::Compact>;
using ezTextureCubeResourceHandle = ezTypedResourceHandle<class ezTextureCubeResource>;

/// \brief Adds a static image of a sky to the scene.
///
/// This is used to fill the scene background with a picture of a sky.
/// The sky image comes from a cubemap texture.
///
/// Position and scale of the game object are irrelevant, the sky always appears behind all other objects.
/// The rotation, however, is used to rotate the sky image.
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

  /// \brief Changes the brightness of the sky image. Mainly useful when an HDR skybox is used.
  void SetExposureBias(float fExposureBias);                // [ property ]
  float GetExposureBias() const { return m_fExposureBias; } // [ property ]

  /// \brief For HDR skyboxes this should stay off. For LDR skyboxes, enabling this will improve brightness and contrast.
  void SetInverseTonemap(bool bInverseTonemap);                // [ property ]
  bool GetInverseTonemap() const { return m_bInverseTonemap; } // [ property ]

  /// \brief Enables that fog is applied to the sky. See SetVirtualDistance().
  void SetUseFog(bool bUseFog);                // [ property ]
  bool GetUseFog() const { return m_bUseFog; } // [ property ]

  /// \brief If fog is enabled, the virtual distance is used to determine how foggy the sky should be.
  void SetVirtualDistance(float fVirtualDistance);                // [ property ]
  float GetVirtualDistance() const { return m_fVirtualDistance; } // [ property ]

  // adds SetCubeMapFile() and GetCubeMapFile() for convenience
  EZ_ADD_RESOURCEHANDLE_ACCESSORS_WITH_SETTER(CubeMap, m_hCubeMap, SetCubeMap);

  void SetCubeMap(const ezTextureCubeResourceHandle& hCubeMap); // [ property ]
  const ezTextureCubeResourceHandle& GetCubeMap() const;        // [ property ]

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
