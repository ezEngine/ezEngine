#pragma once

#include <RendererCore/Lights/LightComponent.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Textures/TextureResource.h>

class ezSpotLightComponent;
typedef ezComponentManager<ezSpotLightComponent> ezSpotLightComponentManager;

/// \brief The render data object for spot lights.
class EZ_RENDERERCORE_DLL ezSpotLightRenderData : public ezRenderData
{
	EZ_ADD_DYNAMIC_REFLECTION(ezSpotLightRenderData, ezRenderData);

public:
	ezTransform m_GlobalTransform;
	ezColor m_LightColor;
  float m_fIntensity;
	float m_fRange;
  ezAngle m_SpotAngle;
  ezTextureResourceHandle m_hProjectedTexture;
  bool m_bCastShadows;
};

/// \brief The standard spot light component.
/// This component represents spot lights with various properties (e.g. a projected cube map, range, spot angle, etc.)
class EZ_RENDERERCORE_DLL ezSpotLightComponent : public ezLightComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezSpotLightComponent, ezLightComponent, ezSpotLightComponentManager);

public:
  ezSpotLightComponent();

  void SetRange(float fRange);
  float GetRange() const;

  void SetSpotAngle(ezAngle fSpotAngle);
  ezAngle GetSpotAngle() const;

  void SetProjectedTexture(const ezTextureResourceHandle& hProjectedTexture);
  const ezTextureResourceHandle& GetProjectedTexture() const;

  void SetProjectedTextureFile(const char* szFile);
  const char* GetProjectedTextureFile() const;

  virtual void OnAfterAttachedToObject() override;
  virtual void OnBeforeDetachedFromObject() override;

  void OnUpdateLocalBounds(ezUpdateLocalBoundsMessage& msg) const;
  void OnExtractRenderData(ezExtractRenderDataMessage& msg) const;

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:

  void SetSpotAngle_Ui(float fSpotAngleInDegrees);
  float GetSpotAngle_Ui() const;

private:

  float m_fRange;
  ezAngle m_SpotAngle;

  ezTextureResourceHandle m_hProjectedTexture;  
};
