#pragma once

#include <RendererCore/Lights/LightComponent.h>
#include <RendererCore/Textures/Texture2DResource.h>

using ezDirectionalLightComponentManager = ezComponentManager<class ezDirectionalLightComponent, ezBlockStorageType::Compact>;

/// \brief The render data object for directional lights.
class EZ_RENDERERCORE_DLL ezDirectionalLightRenderData : public ezLightRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDirectionalLightRenderData, ezLightRenderData);

public:
};

/// \brief A directional lightsource shines light into one fixed direction and has infinite size. It is usually used for sunlight.
///
/// It is very rare to use more than one directional lightsource at the same time.
/// Directional lightsources are used to fake the large scale light of the sun (or moon).
/// They use cascaded shadow maps to reduce the performance overhead for dynamic shadows of such large lights.
class EZ_RENDERERCORE_DLL ezDirectionalLightComponent : public ezLightComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezDirectionalLightComponent, ezLightComponent, ezDirectionalLightComponentManager);

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
  // ezDirectionalLightComponent

public:
  ezDirectionalLightComponent();
  ~ezDirectionalLightComponent();

  /// \brief Sets how many shadow map cascades to use. Typically between 2 and 4.
  void SetNumCascades(ezUInt32 uiNumCascades); // [ property ]
  ezUInt32 GetNumCascades() const;             // [ property ]

  /// \brief Sets the distance around the main camera in which to apply dynamic shadows.
  void SetMinShadowRange(float fMinShadowRange); // [ property ]
  float GetMinShadowRange() const;               // [ property ]

  /// \brief The factor (0 to 1) at which relative distance to start fading out the shadow map. Typically 0.8 or 0.9.
  void SetFadeOutStart(float fFadeOutStart); // [ property ]
  float GetFadeOutStart() const;             // [ property ]

  /// \brief Has something to do with shadow map cascades (TODO: figure out what).
  void SetSplitModeWeight(float fSplitModeWeight); // [ property ]
  float GetSplitModeWeight() const;                // [ property ]

  /// \brief Has something to do with shadow map cascades (TODO: figure out what).
  void SetNearPlaneOffset(float fNearPlaneOffset); // [ property ]
  float GetNearPlaneOffset() const;                // [ property ]

protected:
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  ezUInt32 m_uiNumCascades = 3;
  float m_fMinShadowRange = 50.0f;
  float m_fFadeOutStart = 0.8f;
  float m_fSplitModeWeight = 0.7f;
  float m_fNearPlaneOffset = 100.0f;
};
