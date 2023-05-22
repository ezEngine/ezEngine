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

/// \brief The standard directional light component.
/// This component represents directional lights.
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

  void SetNumCascades(ezUInt32 uiNumCascades); // [ property ]
  ezUInt32 GetNumCascades() const;             // [ property ]

  void SetMinShadowRange(float fMinShadowRange); // [ property ]
  float GetMinShadowRange() const;               // [ property ]

  void SetFadeOutStart(float fFadeOutStart); // [ property ]
  float GetFadeOutStart() const;             // [ property ]

  void SetSplitModeWeight(float fSplitModeWeight); // [ property ]
  float GetSplitModeWeight() const;                // [ property ]

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
