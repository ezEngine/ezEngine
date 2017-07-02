#pragma once

#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <Core/ResourceManager/ResourceHandle.h>

typedef ezTypedResourceHandle<class ezDecalResource> ezDecalResourceHandle;
typedef ezComponentManager<class ezDecalComponent, ezBlockStorageType::Compact> ezDecalComponentManager;

class EZ_RENDERERCORE_DLL ezDecalRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDecalRenderData, ezRenderData);

public:
  ezVec3 m_vHalfExtents;
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

  void SetExtents(const ezVec3& value);
  const ezVec3& GetExtents() const;

  void SetDecal(const ezDecalResourceHandle& hResource);
  const ezDecalResourceHandle& GetDecal() const;

  void SetDecalFile(const char* szFile);
  const char* GetDecalFile() const;

  void OnExtractRenderData(ezExtractRenderDataMessage& msg) const;

protected:

  ezVec3 m_vExtents;
  ezDecalResourceHandle m_hDecal;
};
