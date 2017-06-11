#pragma once

#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>

typedef ezComponentManager<class ezDecalComponent, ezBlockStorageType::Compact> ezDecalComponentManager;

class EZ_RENDERERCORE_DLL ezDecalRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDecalRenderData, ezRenderData);

public:
  //ezColor m_LightColor;
  //float m_fIntensity;
  //ezUInt32 m_uiShadowDataOffset;
};

class EZ_RENDERERCORE_DLL ezDecalComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezDecalComponent, ezRenderComponent, ezDecalComponentManager);

public:
  ezDecalComponent();
  ~ezDecalComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) override;

  /// \todo Size properties (box?)

  //void SetDecal(const ezTextureCubeResourceHandle& hProjectedTexture);
  //const ezTextureCubeResourceHandle& GetDecal() const;

  void SetDecalFile(const char* szFile);
  const char* GetDecalFile() const;

  void OnExtractRenderData(ezExtractRenderDataMessage& msg) const;

protected:

  //ezTextureCubeResourceHandle m_hDecal;
};
