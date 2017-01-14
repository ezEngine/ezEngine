#pragma once

#include <RendererCore/Lights/LightComponent.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Textures/Texture2DResource.h>

class ezPointLightComponent;
typedef ezComponentManager<ezPointLightComponent> ezPointLightComponentManager;

/// \brief The render data object for point lights.
class EZ_RENDERERCORE_DLL ezPointLightRenderData : public ezLightRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPointLightRenderData, ezLightRenderData);

public:
  float m_fRange;
  ezTexture2DResourceHandle m_hProjectedTexture;
};

/// \brief The standard point light component.
/// This component represents point lights with various properties (e.g. a projected cube map, range, etc.)
class EZ_RENDERERCORE_DLL ezPointLightComponent : public ezLightComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPointLightComponent, ezLightComponent, ezPointLightComponentManager);

public:
  ezPointLightComponent();
  ~ezPointLightComponent();

  // ezRenderComponent interface
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds) override;

  void SetRange(float fRange);
  float GetRange() const;

  void SetProjectedTexture(const ezTexture2DResourceHandle& hProjectedTexture);
  const ezTexture2DResourceHandle& GetProjectedTexture() const;

  void SetProjectedTextureFile(const char* szFile);
  const char* GetProjectedTextureFile() const;

  void OnExtractRenderData(ezExtractRenderDataMessage& msg) const;

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

private:

  float m_fRange;
  float m_fEffectiveRange;

  ezTexture2DResourceHandle m_hProjectedTexture;
};

/// \brief A special visualizer attribute for point lights
class EZ_RENDERERCORE_DLL ezPointLightVisualizerAttribute : public ezVisualizerAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPointLightVisualizerAttribute, ezVisualizerAttribute);

public:
  ezPointLightVisualizerAttribute();
  ezPointLightVisualizerAttribute(const char* szRangeProperty, const char* szIntensityProperty, const char* szColorProperty);

  const ezString& GetRangeProperty() const { return m_sProperty1; }
  const ezString& GetIntensityProperty() const { return m_sProperty2; }
  const ezString& GetColorProperty() const { return m_sProperty3; }
};
