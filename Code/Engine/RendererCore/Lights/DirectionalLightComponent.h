#pragma once

#include <RendererCore/Lights/LightComponent.h>
#include <RendererCore/Textures/Texture2DResource.h>

typedef ezComponentManager<class ezDirectionalLightComponent, ezBlockStorageType::Compact> ezDirectionalLightComponentManager;

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

public:
  ezDirectionalLightComponent();
  ~ezDirectionalLightComponent();

  // ezRenderComponent interface
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) override;

  void SetNumCascades(ezUInt32 uiNumCascades);
  ezUInt32 GetNumCascades() const;

  void SetMinShadowRange(float fMinShadowRange);
  float GetMinShadowRange() const;

  void SetFadeOutStart(float fFadeOutStart);
  float GetFadeOutStart() const;

  void SetSplitModeWeight(float fSplitModeWeight);
  float GetSplitModeWeight() const;

  void SetNearPlaneOffset(float fNearPlaneOffset);
  float GetNearPlaneOffset() const;

  void OnExtractRenderData(ezMsgExtractRenderData& msg) const;

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

private:
  ezUInt32 m_uiNumCascades;
  float m_fMinShadowRange;
  float m_fFadeOutStart;
  float m_fSplitModeWeight;
  float m_fNearPlaneOffset;
};

