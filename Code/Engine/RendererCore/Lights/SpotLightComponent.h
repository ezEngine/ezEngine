#pragma once

#include <RendererCore/Lights/LightComponent.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Textures/Texture2DResource.h>

typedef ezComponentManager<class ezSpotLightComponent, ezBlockStorageType::Compact> ezSpotLightComponentManager;

/// \brief The render data object for spot lights.
class EZ_RENDERERCORE_DLL ezSpotLightRenderData : public ezLightRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSpotLightRenderData, ezLightRenderData);

public:
  float m_fRange;
  ezAngle m_InnerSpotAngle;
  ezAngle m_OuterSpotAngle;
  ezTexture2DResourceHandle m_hProjectedTexture;
};

/// \brief The standard spot light component.
/// This component represents spot lights with various properties (e.g. a projected texture, range, spot angle, etc.)
class EZ_RENDERERCORE_DLL ezSpotLightComponent : public ezLightComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSpotLightComponent, ezLightComponent, ezSpotLightComponentManager);

public:
  ezSpotLightComponent();
  ~ezSpotLightComponent();

  // ezRenderComponent interface
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds) override;

  void SetRange(float fRange);
  float GetRange() const;

  void SetInnerSpotAngle(ezAngle fSpotAngle);
  ezAngle GetInnerSpotAngle() const;

  void SetOuterSpotAngle(ezAngle fSpotAngle);
  ezAngle GetOuterSpotAngle() const;

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

  ezAngle m_InnerSpotAngle;
  ezAngle m_OuterSpotAngle;

  ezTexture2DResourceHandle m_hProjectedTexture;
};

/// \brief A special visualizer attribute for spot lights
class EZ_RENDERERCORE_DLL ezSpotLightVisualizerAttribute : public ezVisualizerAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSpotLightVisualizerAttribute, ezVisualizerAttribute);

public:
  ezSpotLightVisualizerAttribute();
  ezSpotLightVisualizerAttribute(const char* szAngleProperty, const char* szRangeProperty, const char* szIntensityProperty, const char* szColorProperty);

  const ezUntrackedString& GetAngleProperty() const { return m_sProperty1; }
  const ezUntrackedString& GetRangeProperty() const { return m_sProperty2; }
  const ezUntrackedString& GetIntensityProperty() const { return m_sProperty3; }
  const ezUntrackedString& GetColorProperty() const { return m_sProperty4; }
};
