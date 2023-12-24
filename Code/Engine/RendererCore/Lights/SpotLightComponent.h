#pragma once

#include <RendererCore/Lights/LightComponent.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Textures/Texture2DResource.h>

using ezSpotLightComponentManager = ezComponentManager<class ezSpotLightComponent, ezBlockStorageType::Compact>;

/// \brief The render data object for spot lights.
class EZ_RENDERERCORE_DLL ezSpotLightRenderData : public ezLightRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSpotLightRenderData, ezLightRenderData);

public:
  float m_fRange;
  ezAngle m_InnerSpotAngle;
  ezAngle m_OuterSpotAngle;
  // ezTexture2DResourceHandle m_hProjectedTexture;
};

/// \brief Adds a spotlight to the scene, optionally casting shadows.
class EZ_RENDERERCORE_DLL ezSpotLightComponent : public ezLightComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSpotLightComponent, ezLightComponent, ezSpotLightComponentManager);

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
  // ezSpotLightComponent

public:
  ezSpotLightComponent();
  ~ezSpotLightComponent();

  /// \brief Sets the radius (or length of the cone) of the lightsource. If zero, it is automatically determined from the intensity.
  void SetRange(float fRange); // [ property ]
  float GetRange() const;      // [ property ]

  /// \brief Returns the final radius of the lightsource.
  float GetEffectiveRange() const;

  /// \brief Sets the inner angle where the spotlight has equal brightness.
  void SetInnerSpotAngle(ezAngle spotAngle); // [ property ]
  ezAngle GetInnerSpotAngle() const;         // [ property ]

  /// \brief Sets the outer angle of the spotlight's cone. The light will fade out between the inner and outer angle.
  void SetOuterSpotAngle(ezAngle spotAngle); // [ property ]
  ezAngle GetOuterSpotAngle() const;         // [ property ]

  // void SetProjectedTextureFile(const char* szFile); // [ property ]
  // const char* GetProjectedTextureFile() const;      // [ property ]

  // void SetProjectedTexture(const ezTexture2DResourceHandle& hProjectedTexture);
  // const ezTexture2DResourceHandle& GetProjectedTexture() const;

protected:
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;
  ezBoundingSphere CalculateBoundingSphere(const ezTransform& t, float fRange) const;

  float m_fRange = 0.0f;
  float m_fEffectiveRange = 0.0f;

  ezAngle m_InnerSpotAngle = ezAngle::MakeFromDegree(15.0f);
  ezAngle m_OuterSpotAngle = ezAngle::MakeFromDegree(30.0f);

  // ezTexture2DResourceHandle m_hProjectedTexture;
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
