#pragma once

#include <RendererCore/Lights/LightComponent.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Textures/TextureCubeResource.h>

using ezPointLightComponentManager = ezComponentManager<class ezPointLightComponent, ezBlockStorageType::Compact>;

/// \brief The render data object for point lights.
class EZ_RENDERERCORE_DLL ezPointLightRenderData : public ezLightRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPointLightRenderData, ezLightRenderData);

public:
  float m_fRange;
  // ezTextureCubeResourceHandle m_hProjectedTexture;
};

/// \brief Adds a dynamic point light to the scene, optionally casting shadows.
///
/// For performance reasons, prefer to use ezSpotLightComponent where possible.
/// Do not use shadows just to limit the light cone, when a spot light could achieve the same.
class EZ_RENDERERCORE_DLL ezPointLightComponent : public ezLightComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPointLightComponent, ezLightComponent, ezPointLightComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;


  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent

public:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // ezPointLightComponent

public:
  ezPointLightComponent();
  ~ezPointLightComponent();

  /// \brief Sets the radius of the lightsource. If zero, the radius is automatically determined from the intensity.
  void SetRange(float fRange); // [ property ]
  float GetRange() const;      // [ property ]

  /// \brief Returns the final radius of the lightsource.
  float GetEffectiveRange() const;

  // void SetProjectedTextureFile(const char* szFile); // [ property ]
  // const char* GetProjectedTextureFile() const;      // [ property ]

  // void SetProjectedTexture(const ezTextureCubeResourceHandle& hProjectedTexture);
  // const ezTextureCubeResourceHandle& GetProjectedTexture() const;

protected:
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  float m_fRange = 0.0f;
  float m_fEffectiveRange = 0.0f;

  // ezTextureCubeResourceHandle m_hProjectedTexture;
};

/// \brief A special visualizer attribute for point lights
class EZ_RENDERERCORE_DLL ezPointLightVisualizerAttribute : public ezVisualizerAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPointLightVisualizerAttribute, ezVisualizerAttribute);

public:
  ezPointLightVisualizerAttribute();
  ezPointLightVisualizerAttribute(const char* szRangeProperty, const char* szIntensityProperty, const char* szColorProperty);

  const ezUntrackedString& GetRangeProperty() const { return m_sProperty1; }
  const ezUntrackedString& GetIntensityProperty() const { return m_sProperty2; }
  const ezUntrackedString& GetColorProperty() const { return m_sProperty3; }
};
