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

using ezDecalComponentManager = ezComponentManager<class ezDecalComponent, ezBlockStorageType::Compact>;

class EZ_RENDERERCORE_DLL ezDecalRenderData : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezDecalRenderData, ezRenderData);

public:
  ezUInt32 m_uiApplyOnlyToId;
  ezUInt32 m_uiFlags;
  ezUInt32 m_uiAngleFadeParams;

  ezColorLinearUB m_BaseColor;
  ezColorLinear16f m_EmissiveColor;

  ezUInt32 m_uiBaseColorAtlasScale;
  ezUInt32 m_uiBaseColorAtlasOffset;

  ezUInt32 m_uiNormalAtlasScale;
  ezUInt32 m_uiNormalAtlasOffset;

  ezUInt32 m_uiORMAtlasScale;
  ezUInt32 m_uiORMAtlasOffset;
};

/// \brief Projects a decal texture onto geometry within a box volume.
///
/// This is used to add dirt, scratches, signs and other surface imperfections to geometry.
/// The component uses a box shape to define the position and volume and projection direction.
/// This can be set up in a level to add detail, but it can also be used by dynamic effects such as bullet hits,
/// to visualize the impact. To add variety a prefab may use different textures and vary in size.
class EZ_RENDERERCORE_DLL ezDecalComponent final : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezDecalComponent, ezRenderComponent, ezDecalComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnActivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent

protected:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible, ezMsgUpdateLocalBounds& msg) override;
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;


  //////////////////////////////////////////////////////////////////////////
  // ezDecalComponent

public:
  ezDecalComponent();
  ~ezDecalComponent();

  /// \brief Sets the extents of the box inside which to project the decal.
  void SetExtents(const ezVec3& value); // [ property ]
  const ezVec3& GetExtents() const;     // [ property ]

  /// \brief The size variance defines how much the size may randomly deviate, such that the decals look different.
  void SetSizeVariance(float fVariance); // [ property ]
  float GetSizeVariance() const;         // [ property ]

  /// \brief An additional tint color for the decal.
  void SetColor(ezColorGammaUB color); // [ property ]
  ezColorGammaUB GetColor() const;     // [ property ]

  /// \brief An additional emissive color to make the decal glow.
  void SetEmissiveColor(ezColor color); // [ property ]
  ezColor GetEmissiveColor() const;     // [ property ]

  /// \brief At which angle between the decal orientation and the surface it is projected onto, to start fading the decal out.
  void SetInnerFadeAngle(ezAngle fadeAngle); // [ property ]
  ezAngle GetInnerFadeAngle() const;         // [ property ]

  /// \brief At which angle between the decal orientation and the surface it is projected onto, to fully fade out the decal.
  void SetOuterFadeAngle(ezAngle fadeAngle); // [ property ]
  ezAngle GetOuterFadeAngle() const;         // [ property ]

  /// \brief If multiple decals are in the same location, this allows to tweak which one is rendered on top.
  void SetSortOrder(float fOrder); // [ property ]
  float GetSortOrder() const;      // [ property ]

  /// \brief Whether the decal projection should use a kind of three-way texture mapping to wrap the image around curved geometry.
  void SetWrapAround(bool bWrapAround);         // [ property ]
  bool GetWrapAround() const;                   // [ property ]

  void SetMapNormalToGeometry(bool bMapNormal); // [ property ]
  bool GetMapNormalToGeometry() const;          // [ property ]

  /// \brief Sets the decal resource to use. If more than one is set, a random one will be chosen.
  ///
  /// Indices that are written to will be created on-demand.
  void SetDecal(ezUInt32 uiIndex, const ezDecalResourceHandle& hResource); // [ property ]
  const ezDecalResourceHandle& GetDecal(ezUInt32 uiIndex) const;           // [ property ]

  /// If non-zero, the decal fades out after this time and then vanishes.
  ezVarianceTypeTime m_FadeOutDelay; // [ property ]

  /// How much time the fade out takes.
  ezTime m_FadeOutDuration; // [ property ]

  /// If fade-out is used, the decal may delete itself afterwards.
  ezEnum<ezOnComponentFinishedAction> m_OnFinishedAction; // [ property ]

  /// \brief Sets the cardinal axis into which the decal projection should be.
  void SetProjectionAxis(ezEnum<ezBasisAxis> projectionAxis); // [ property ]
  ezEnum<ezBasisAxis> GetProjectionAxis() const;              // [ property ]

  /// \brief If set, the decal only appears on the given object.
  ///
  /// This is typically used to limit the decal to a single dynamic object, such that damage decals don't project
  /// onto static geometry and other objects.
  void SetApplyOnlyTo(ezGameObjectHandle hObject);
  ezGameObjectHandle GetApplyOnlyTo() const;

  // TODO: Using ezStringView for the array accessors doesn't work (currently)

  ezUInt32 DecalFile_GetCount() const;                     // [ property ]
  ezString DecalFile_Get(ezUInt32 uiIndex) const;          // [ property ]
  void DecalFile_Set(ezUInt32 uiIndex, ezString sFile);    // [ property ]
  void DecalFile_Insert(ezUInt32 uiIndex, ezString sFile); // [ property ]
  void DecalFile_Remove(ezUInt32 uiIndex);                 // [ property ]


protected:
  void SetApplyToRef(const char* szReference); // [ property ]
  void UpdateApplyTo();

  void OnTriggered(ezMsgComponentInternalTrigger& msg);
  void OnMsgDeleteGameObject(ezMsgDeleteGameObject& msg);
  void OnMsgOnlyApplyToObject(ezMsgOnlyApplyToObject& msg);
  void OnMsgSetColor(ezMsgSetColor& msg);

  ezVec3 m_vExtents = ezVec3(1.0f);
  float m_fSizeVariance = 0;
  ezColorGammaUB m_Color = ezColor::White;
  ezColor m_EmissiveColor = ezColor::Black;
  ezAngle m_InnerFadeAngle = ezAngle::MakeFromDegree(50.0f);
  ezAngle m_OuterFadeAngle = ezAngle::MakeFromDegree(80.0f);
  float m_fSortOrder = 0;
  bool m_bWrapAround = false;
  bool m_bMapNormalToGeometry = false;
  ezUInt8 m_uiRandomDecalIdx = 0xFF;
  ezEnum<ezBasisAxis> m_ProjectionAxis;
  ezHybridArray<ezDecalResourceHandle, 1> m_Decals;

  ezGameObjectHandle m_hApplyOnlyToObject;
  ezUInt32 m_uiApplyOnlyToId = 0;

  ezTime m_StartFadeOutTime;
  ezUInt32 m_uiInternalSortKey = 0;

private:
  const char* DummyGetter() const { return nullptr; }
};
