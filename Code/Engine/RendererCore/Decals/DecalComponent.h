#pragma once

#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <Core/ResourceManager/ResourceHandle.h>

typedef ezTypedResourceHandle<class ezDecalResource> ezDecalResourceHandle;
typedef ezComponentManager<class ezDecalComponent, ezBlockStorageType::Compact> ezDecalComponentManager;
class ezAbstractObjectNode;

class EZ_RENDERERCORE_DLL ezDecalRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDecalRenderData, ezRenderData);

public:
  ezVec3 m_vHalfExtents;
  ezColor m_Color;
  ezVec2 m_vBaseAtlasScale;
  ezVec2 m_vBaseAtlasOffset;
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

  void SetColor(ezColorGammaUB color);
  ezColorGammaUB GetColor() const;

  void SetSortOrder(float fOrder);
  float GetSortOrder() const;

  void SetDecal(const ezDecalResourceHandle& hResource);
  const ezDecalResourceHandle& GetDecal() const;

  void SetDecalFile(const char* szFile);
  const char* GetDecalFile() const;

  void OnExtractRenderData(ezExtractRenderDataMessage& msg) const;

  void OnObjectCreated(const ezAbstractObjectNode& node);

protected:

  ezVec3 m_vExtents;
  ezColorGammaUB m_Color;
  float m_fSortOrder;
  ezDecalResourceHandle m_hDecal;
};
