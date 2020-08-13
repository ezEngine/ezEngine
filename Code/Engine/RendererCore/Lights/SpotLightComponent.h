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

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;


  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent

public:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) override;


  //////////////////////////////////////////////////////////////////////////
  // ezSpotLightComponent

public:
  ezSpotLightComponent();
  ~ezSpotLightComponent();

  void SetRange(float fRange); // [ property ]
  float GetRange() const;      // [ property ]

  float GetEffectiveRange() const;

  void SetInnerSpotAngle(ezAngle fSpotAngle); // [ property ]
  ezAngle GetInnerSpotAngle() const;          // [ property ]

  void SetOuterSpotAngle(ezAngle fSpotAngle); // [ property ]
  ezAngle GetOuterSpotAngle() const;          // [ property ]

  void SetProjectedTextureFile(const char* szFile); // [ property ]
  const char* GetProjectedTextureFile() const;      // [ property ]

  void SetProjectedTexture(const ezTexture2DResourceHandle& hProjectedTexture);
  const ezTexture2DResourceHandle& GetProjectedTexture() const;

protected:
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;
  ezBoundingSphere CalculateBoundingSphere(const ezTransform& t, float fRange) const;

  float m_fRange = 0.0f;
  float m_fEffectiveRange = 0.0f;

  ezAngle m_InnerSpotAngle = ezAngle::Degree(15.0f);
  ezAngle m_OuterSpotAngle = ezAngle::Degree(30.0f);

  ezTexture2DResourceHandle m_hProjectedTexture;
};

/// \brief A special visualizer attribute for spot lights
class EZ_RENDERERCORE_DLL ezSpotLightVisualizerAttribute : public ezVisualizerAttribute
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSpotLightVisualizerAttribute, ezVisualizerAttribute);

public:
  ezSpotLightVisualizerAttribute();
  ezSpotLightVisualizerAttribute(
    const char* szAngleProperty, const char* szRangeProperty, const char* szIntensityProperty, const char* szColorProperty);

  const ezUntrackedString& GetAngleProperty() const { return m_sProperty1; }
  const ezUntrackedString& GetRangeProperty() const { return m_sProperty2; }
  const ezUntrackedString& GetIntensityProperty() const { return m_sProperty3; }
  const ezUntrackedString& GetColorProperty() const { return m_sProperty4; }
};
