#pragma once

#include <Core/World/World.h>
#include <RendererCore/Meshes/MeshComponent.h>

typedef ezComponentManager<class ezVolumetricCloudsComponent, ezBlockStorageType::Compact> ezVolumetricCloudsComponentManager;

class EZ_RENDERERCORE_DLL ezVolumetricCloudsComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezVolumetricCloudsComponent, ezRenderComponent, ezVolumetricCloudsComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void Initialize() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent

public:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg) override;


  //////////////////////////////////////////////////////////////////////////
  // ezVolumetricCloudsComponent

public:
  ezVolumetricCloudsComponent();
  ~ezVolumetricCloudsComponent();

  /// Scales the raw [0, 1] density read from the volume/noise textures into a per-world-unit extinction coefficient.
  ///
  /// The textures only store a normalized density fraction, not a value calibrated for how large the cloud is in
  /// world units, so this needs to be tuned per asset/scale to make the cloud interior look properly opaque instead
  /// of translucent and grey.
  void SetDensityMultiplier(float fDensityMultiplier);                // [ property ]
  float GetDensityMultiplier() const { return m_fDensityMultiplier; } // [ property ]

  /// Ambient light color at the bottom of the cloud layer, where the cloud mass above occludes most of the sky.
  void SetBaseAmbientColor(ezColor color);                          // [ property ]
  ezColor GetBaseAmbientColor() const { return m_BaseAmbientColor; } // [ property ]

  /// Ambient light color at the top of the cloud layer, which faces the open sky.
  void SetTopAmbientColor(ezColor color);                         // [ property ]
  ezColor GetTopAmbientColor() const { return m_TopAmbientColor; } // [ property ]

private:
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;
  void UpdateMaterials() const;

  ezMeshResourceHandle m_hMesh;
  ezMaterialResourceHandle m_hMaterial;

  ezTexture3DResourceHandle m_hNoiseLut;
  ezTexture3DResourceHandle m_hDetailNoiseLut;

  float m_fDensityMultiplier = 6.0f;
  ezColor m_BaseAmbientColor = ezColor(0.1f, 0.1f, 0.13f);
  ezColor m_TopAmbientColor = ezColor(0.4f, 0.55f, 0.8f);

  mutable ezInstanceDataOffset m_InstanceDataOffset;
};
