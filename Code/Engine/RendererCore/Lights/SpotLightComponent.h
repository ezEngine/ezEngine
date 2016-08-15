#pragma once

#include <RendererCore/Lights/LightComponent.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Textures/TextureResource.h>

class ezSpotLightComponent;
typedef ezComponentManager<ezSpotLightComponent> ezSpotLightComponentManager;

/// \brief The render data object for spot lights.
class EZ_RENDERERCORE_DLL ezSpotLightRenderData : public ezLightRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSpotLightRenderData, ezLightRenderData);

public:
  float m_fRange;
  ezAngle m_SpotAngle;
  ezTextureResourceHandle m_hProjectedTexture;
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

  void SetSpotAngle(ezAngle fSpotAngle);
  ezAngle GetSpotAngle() const;

  void SetProjectedTexture(const ezTextureResourceHandle& hProjectedTexture);
  const ezTextureResourceHandle& GetProjectedTexture() const;

  void SetProjectedTextureFile(const char* szFile);
  const char* GetProjectedTextureFile() const;

  void OnExtractRenderData(ezExtractRenderDataMessage& msg) const;

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

private:

  float m_fRange;
  ezAngle m_SpotAngle;

  ezTextureResourceHandle m_hProjectedTexture;  
};
