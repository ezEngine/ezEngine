#pragma once

#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>

/// \brief Base class for light render data objects.
class EZ_RENDERERCORE_DLL ezLightRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLightRenderData, ezRenderData);

public:
  ezColor m_LightColor;
  float m_fIntensity;
  bool m_bCastShadows;
};

/// \brief Base class for all ez light components containing shared properties
class EZ_RENDERERCORE_DLL ezLightComponent : public ezRenderComponent
{
  EZ_DECLARE_ABSTRACT_COMPONENT_TYPE(ezLightComponent, ezRenderComponent);

public:
  ezLightComponent();

  void SetLightColor(ezColor LightColor);
  ezColor GetLightColor() const;

  void SetIntensity(float fIntensity);
  float GetIntensity() const;

  void SetCastShadows(bool bCastShadows);
  bool GetCastShadows();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:

  ezColor m_LightColor;
  float m_fIntensity;
  bool m_bCastShadows;

};
