#pragma once

#include <Foundation/Types/VarianceTypes.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderData.h>

class ezAbstractObjectNode;
struct ezMsgComponentInternalTrigger;
struct ezMsgOnlyApplyToObject;
struct ezMsgSetColor;

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
  ezUInt32 m_uiApplyOnlyToId;
  ezUInt8 m_uiDecalMode;
  bool m_bWrapAround;
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
  void OnExtractRenderData(ezMsgExtractRenderData& msg) const;

  //////////////////////////////////////////////////////////////////////////
  // Editor Interface

  void OnObjectCreated(const ezAbstractObjectNode& node);

  //////////////////////////////////////////////////////////////////////////
  // Properties

public:
  void SetExtents(const ezVec3& value);
  const ezVec3& GetExtents() const;

  void SetSizeVariance(float fVariance);
  float GetSizeVariance() const;

  void SetColor(ezColor color);
  ezColor GetColor() const;

  void SetInnerFadeAngle(ezAngle fFadeAngle);
  ezAngle GetInnerFadeAngle() const;

  void SetOuterFadeAngle(ezAngle fFadeAngle);
  ezAngle GetOuterFadeAngle() const;

  void SetSortOrder(float fOrder);
  float GetSortOrder() const;

  void SetWrapAround(bool bWrapAround);
  bool GetWrapAround() const;

  void SetDecal(const ezDecalResourceHandle& hResource);
  const ezDecalResourceHandle& GetDecal() const;

  void SetDecalFile(const char* szFile);
  const char* GetDecalFile() const;

  void SetApplyOnlyTo(ezGameObjectHandle hObject);
  ezGameObjectHandle GetApplyOnlyTo() const;

  ezVarianceTypeTime m_FadeOutDelay;
  ezTime m_FadeOutDuration;
  ezEnum<ezOnComponentFinishedAction> m_OnFinishedAction;

protected:
  ezVec3 m_vExtents;
  float m_fSizeVariance;
  ezColor m_Color;
  ezAngle m_InnerFadeAngle;
  ezAngle m_OuterFadeAngle;
  float m_fSortOrder;
  bool m_bWrapAround;
  ezDecalResourceHandle m_hDecal;

  ezGameObjectHandle m_hApplyOnlyToObject;
  ezUInt32 m_uiApplyOnlyToId;

  //////////////////////////////////////////////////////////////////////////
  // Internal

  void OnTriggered(ezMsgComponentInternalTrigger& msg);
  void OnDeleteObject(ezMsgDeleteGameObject& msg);
  void OnApplyOnlyTo(ezMsgOnlyApplyToObject& msg);
  void OnSetColor(ezMsgSetColor& msg);

  ezTime m_StartFadeOutTime;
  ezUInt32 m_uiInternalSortKey;
  static ezUInt16 s_uiNextSortKey;
};
