#pragma once

#include <RendererCore/Lights/LightComponent.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Textures/TextureResource.h>

class ezPointLightComponent;
typedef ezComponentManager<ezPointLightComponent> ezPointLightComponentManager;

/// \brief The render data object for point lights.
class EZ_RENDERERCORE_DLL ezPointLightRenderData : public ezLightRenderData
{
	EZ_ADD_DYNAMIC_REFLECTION(ezPointLightRenderData, ezLightRenderData);

public:
	ezColor m_LightColor;
  float m_fIntensity;
	float m_fRange;
  ezTextureResourceHandle m_hProjectedTexture;
  bool m_bCastShadows;
};

/// \brief The standard point light component.
/// This component represents point lights with various properties (e.g. a projected cube map, range, etc.)
class EZ_RENDERERCORE_DLL ezPointLightComponent : public ezLightComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezPointLightComponent, ezLightComponent, ezPointLightComponentManager);

public:
  ezPointLightComponent();

  void SetRange(float fRange);
  float GetRange() const;

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

private:

  float m_fRange;

  ezTextureResourceHandle m_hProjectedTexture;  
};
