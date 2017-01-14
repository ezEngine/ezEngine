#pragma once

#include <RendererCore/Lights/LightComponent.h>
#include <RendererCore/Textures/Texture2DResource.h>

class ezDirectionalLightComponent;
typedef ezComponentManager<ezDirectionalLightComponent> ezDirectionalLightComponentManager;

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
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds) override;
  
  void OnExtractRenderData(ezExtractRenderDataMessage& msg) const;

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

private:
};
