#pragma once

#include <RendererCore/Declarations.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <Foundation/Types/VarianceTypes.h>

class ezAbstractObjectNode;
struct ezInternalComponentMessage;

class EZ_RENDERERCORE_DLL ezDecalComponentManager : public ezComponentManager<class ezDecalComponent, ezBlockStorageType::Compact>
{
public:
  ezDecalComponentManager(ezWorld* pWorld);

  virtual void Initialize() override;

private:
  friend class ezDecalComponent;
  ezDecalAtlasResourceHandle m_hDecalAtlas;
};

class EZ_RENDERERCORE_DLL ezDecalRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDecalRenderData, ezRenderData);

public:
  ezVec3 m_vHalfExtents;
  ezColor m_Color;
  ezAngle m_InnerFadeAngle;
  ezAngle m_OuterFadeAngle;
  ezVec2 m_vBaseAtlasScale;
  ezVec2 m_vBaseAtlasOffset;
};

class EZ_RENDERERCORE_DLL ezDecalComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezDecalComponent, ezRenderComponent, ezDecalComponentManager);

public:
  ezDecalComponent();
  ~ezDecalComponent();

  //////////////////////////////////////////////////////////////////////////
  // ezComponent Interface

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent Interface

  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) override;
  void OnExtractRenderData(ezExtractRenderDataMessage& msg) const;

  //////////////////////////////////////////////////////////////////////////
  // Editor Interface

  void OnObjectCreated(const ezAbstractObjectNode& node);

  //////////////////////////////////////////////////////////////////////////
  // Properties

public:
  void SetExtents(const ezVec3& value);
  const ezVec3& GetExtents() const;
  float m_fSizeVariance;

  void SetColor(ezColorGammaUB color);
  ezColorGammaUB GetColor() const;

  void SetInnerFadeAngle(ezAngle fFadeAngle);
  ezAngle GetInnerFadeAngle() const;

  void SetOuterFadeAngle(ezAngle fFadeAngle);
  ezAngle GetOuterFadeAngle() const;

  void SetSortOrder(float fOrder);
  float GetSortOrder() const;

  void SetDecal(const ezDecalResourceHandle& hResource);
  const ezDecalResourceHandle& GetDecal() const;

  void SetDecalFile(const char* szFile);
  const char* GetDecalFile() const;

  ezVarianceTypeTime m_FadeOutDelay;
  ezTime m_FadeOutDuration;
  ezEnum<ezOnComponentFinishedAction> m_OnFinishedAction;

protected:

  ezVec3 m_vExtents;
  ezColorGammaUB m_Color;
  ezAngle m_InnerFadeAngle;
  ezAngle m_OuterFadeAngle;
  float m_fSortOrder;
  ezDecalResourceHandle m_hDecal;

  //////////////////////////////////////////////////////////////////////////
  // Internal

  void OnTriggered(ezInternalComponentMessage& msg);

  ezTime m_StartFadeOutTime;
  ezUInt32 m_uiInternalSortKey;
  static ezUInt16 s_uiNextSortKey;
};
