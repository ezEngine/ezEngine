#pragma once

#include <Foundation/Math/Color16f.h>
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
  ezUInt32 m_uiFlags;
  ezUInt32 m_uiAngleFadeParams;

  ezColorLinear16f m_Color;

  ezUInt32 m_uiBaseColorAtlasScale;
  ezUInt32 m_uiBaseColorAtlasOffset;

  ezUInt32 m_uiNormalAtlasScale;
  ezUInt32 m_uiNormalAtlasOffset;

  ezUInt32 m_uiORMAtlasScale;
  ezUInt32 m_uiORMAtlasOffset;
};

class EZ_RENDERERCORE_DLL ezDecalComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezDecalComponent, ezRenderComponent, ezDecalComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent

protected:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible) override;
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;


  //////////////////////////////////////////////////////////////////////////
  // ezDecalComponent

public:
  ezDecalComponent();
  ~ezDecalComponent();

  void SetExtents(const ezVec3& value); // [ property ]
  const ezVec3& GetExtents() const;     // [ property ]

  void SetSizeVariance(float fVariance); // [ property ]
  float GetSizeVariance() const;         // [ property ]

  void SetColor(ezColor color); // [ property ]
  ezColor GetColor() const;     // [ property ]

  void SetInnerFadeAngle(ezAngle fFadeAngle); // [ property ]
  ezAngle GetInnerFadeAngle() const;          // [ property ]

  void SetOuterFadeAngle(ezAngle fFadeAngle); // [ property ]
  ezAngle GetOuterFadeAngle() const;          // [ property ]

  void SetSortOrder(float fOrder); // [ property ]
  float GetSortOrder() const;      // [ property ]

  void SetWrapAround(bool bWrapAround); // [ property ]
  bool GetWrapAround() const;           // [ property ]

  void SetMapNormalToGeometry(bool bMapNormal); // [ property ]
  bool GetMapNormalToGeometry() const;           // [ property ]

  void SetDecal(const ezDecalResourceHandle& hResource); // [ property ]
  const ezDecalResourceHandle& GetDecal() const;         // [ property ]

  void SetDecalFile(const char* szFile); // [ property ]
  const char* GetDecalFile() const;      // [ property ]

  ezVarianceTypeTime m_FadeOutDelay;                      // [ property ]
  ezTime m_FadeOutDuration;                               // [ property ]
  ezEnum<ezOnComponentFinishedAction> m_OnFinishedAction; // [ property ]

  void SetApplyOnlyTo(ezGameObjectHandle hObject);
  ezGameObjectHandle GetApplyOnlyTo() const;

protected:
  void OnObjectCreated(const ezAbstractObjectNode& node);
  void OnTriggered(ezMsgComponentInternalTrigger& msg);
  void OnMsgDeleteGameObject(ezMsgDeleteGameObject& msg);
  void OnMsgOnlyApplyToObject(ezMsgOnlyApplyToObject& msg);
  void OnMsgSetColor(ezMsgSetColor& msg);

  ezVec3 m_vExtents = ezVec3(1.0f);
  float m_fSizeVariance = 0;
  ezColor m_Color = ezColor::White;
  ezAngle m_InnerFadeAngle = ezAngle::Degree(50.0f);
  ezAngle m_OuterFadeAngle = ezAngle::Degree(80.0f);
  float m_fSortOrder = 0;
  bool m_bWrapAround = false;
  bool m_bMapNormalToGeometry = false;
  ezDecalResourceHandle m_hDecal;

  ezGameObjectHandle m_hApplyOnlyToObject;
  ezUInt32 m_uiApplyOnlyToId = 0;

  ezTime m_StartFadeOutTime;
  ezUInt32 m_uiInternalSortKey;
  static ezUInt16 s_uiNextSortKey;
};
