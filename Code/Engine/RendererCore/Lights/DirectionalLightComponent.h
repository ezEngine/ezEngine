#pragma once

#include <RendererCore/Lights/LightComponent.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Textures/TextureResource.h>

class ezDirectionalLightComponent;
typedef ezComponentManager<ezDirectionalLightComponent> ezDirectionalLightComponentManager;

/// \brief The render data object for directional lights.
class EZ_RENDERERCORE_DLL ezDirectionalLightRenderData : public ezRenderData
{
	EZ_ADD_DYNAMIC_REFLECTION(ezDirectionalLightRenderData, ezRenderData);

public:
	ezTransform m_GlobalTransform;
	ezColor m_LightColor;
  float m_fIntensity;
  bool m_bCastShadows;
};

/// \brief The standard directional light component.
/// This component represents directional lights.
class EZ_RENDERERCORE_DLL ezDirectionalLightComponent : public ezLightComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezDirectionalLightComponent, ezLightComponent, ezDirectionalLightComponentManager);

public:
  ezDirectionalLightComponent();

  virtual void OnAfterAttachedToObject() override;
  virtual void OnBeforeDetachedFromObject() override;

  void OnUpdateLocalBounds(ezUpdateLocalBoundsMessage& msg) const;
  void OnExtractRenderData(ezExtractRenderDataMessage& msg) const;

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

private:
};
