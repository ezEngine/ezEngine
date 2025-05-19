#pragma once

#include <Core/World/ComponentManager.h>
#include <GameEngine/Physics/ClothSheetSimulator.h>
#include <JoltPlugin/JoltPluginDLL.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/Renderer.h>

using ezMaterialResourceHandle = ezTypedResourceHandle<class ezMaterialResource>;
using ezDynamicMeshBufferResourceHandle = ezTypedResourceHandle<class ezDynamicMeshBufferResource>;

//////////////////////////////////////////////////////////////////////////

class EZ_JOLTPLUGIN_DLL ezJoltClothSheetComponentManager : public ezComponentManager<class ezJoltClothSheetComponent, ezBlockStorageType::FreeList>
{
public:
  ezJoltClothSheetComponentManager(ezWorld* pWorld);
  ~ezJoltClothSheetComponentManager();

  virtual void Initialize() override;

private:
  ezUInt64 m_uiLastJoltUpdateCounter = 0;
  void Update(const ezWorldModule::UpdateContext& context);
  void UpdateBounds(const ezWorldModule::UpdateContext& context);
};

//////////////////////////////////////////////////////////////////////////

class EZ_JOLTPLUGIN_DLL ezJoltClothSheetRenderData final : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezJoltClothSheetRenderData, ezRenderData);

public:
  ezUInt32 m_uiUniqueID = 0;
  ezVec2 m_vTextureScale = ezVec2(1.0f);
  ezArrayPtr<ezVec3> m_Positions;
  ezArrayPtr<ezUInt16> m_Indices;
  ezUInt16 m_uiVerticesX;
  ezUInt16 m_uiVerticesY;
  ezColor m_Color = ezColor::White;

  ezMaterialResourceHandle m_hMaterial;
};

class EZ_JOLTPLUGIN_DLL ezJoltClothSheetRenderer : public ezRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezJoltClothSheetRenderer, ezRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezJoltClothSheetRenderer);

public:
  ezJoltClothSheetRenderer();
  ~ezJoltClothSheetRenderer();

  virtual void GetSupportedRenderDataCategories(ezHybridArray<ezRenderData::Category, 8>& ref_categories) const override;
  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& ref_types) const override;
  virtual void RenderBatch(const ezRenderViewContext& renderContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const override;


protected:
  void CreateVertexBuffer();

  ezDynamicMeshBufferResourceHandle m_hDynamicMeshBuffer;
};

/// \brief Flags for how a piece of cloth should be simulated.
struct EZ_JOLTPLUGIN_DLL ezJoltClothSheetFlags
{
  using StorageType = ezUInt16;

  enum Enum
  {
    FixedCornerTopLeft = EZ_BIT(0),     ///< This corner can't move.
    FixedCornerTopRight = EZ_BIT(1),    ///< This corner can't move.
    FixedCornerBottomRight = EZ_BIT(2), ///< This corner can't move.
    FixedCornerBottomLeft = EZ_BIT(3),  ///< This corner can't move.
    FixedEdgeTop = EZ_BIT(4),           ///< This entire edge can't move.
    FixedEdgeRight = EZ_BIT(5),         ///< This entire edge can't move.
    FixedEdgeBottom = EZ_BIT(6),        ///< This entire edge can't move.
    FixedEdgeLeft = EZ_BIT(7),          ///< This entire edge can't move.

    Default = FixedEdgeTop
  };

  struct Bits
  {
    StorageType FixedCornerTopLeft : 1;
    StorageType FixedCornerTopRight : 1;
    StorageType FixedCornerBottomRight : 1;
    StorageType FixedCornerBottomLeft : 1;
    StorageType FixedEdgeTop : 1;
    StorageType FixedEdgeRight : 1;
    StorageType FixedEdgeBottom : 1;
    StorageType FixedEdgeLeft : 1;
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_JOLTPLUGIN_DLL, ezJoltClothSheetFlags);

/// \brief Simulates a rectangular piece of cloth.
///
/// The cloth doesn't interact with the environment and doesn't collide with any geometry.
/// The component samples the wind simulation and applies wind forces to the cloth.
///
/// Cloth sheets can be used as decorative elements like flags that blow in the wind.
class EZ_JOLTPLUGIN_DLL ezJoltClothSheetComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltClothSheetComponent, ezRenderComponent, ezJoltClothSheetComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // ezRenderComponent

public:
  virtual ezResult GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg) override;

private:
  void OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const;

  //////////////////////////////////////////////////////////////////////////
  // ezJoltClothSheetComponent

public:
  ezJoltClothSheetComponent();
  ~ezJoltClothSheetComponent();

  /// Sets the world-space size of the cloth.
  void SetSize(ezVec2 vVal);                 // [ property ]
  ezVec2 GetSize() const { return m_vSize; } // [ property ]

  /// Sets of how many pieces the cloth is made up.
  ///
  /// More pieces cost more performance to simulate the cloth.
  /// A size of 32x32 is already quite performance intensive. USe as few segments as possible.
  /// For many cases 8x8 or 12x12 should already be good enough.
  /// Also the more segments there are, the more the cloth will sag.
  void SetSegments(ezVec2U32 vVal);                     // [ property ]
  ezVec2U32 GetSegments() const { return m_vSegments; } // [ property ]

  /// The collision layer determines with which other actors this actor collides. \see ezJoltActorComponent
  ezUInt8 m_uiCollisionLayer = 0; // [ property ]

  ezUInt8 m_uiWeightCategory = 0; // [ property ]
  float m_fWeightValue = 1.0f;    // [ property ]

  /// \brief Adjusts how strongly gravity affects the soft body.
  float m_fGravityFactor = 1.0f; // [ property ]

  /// A factor to tweak how strong the wind can push the cloth.
  float m_fWindInfluence = 0.3f; // [ property ]

  /// Damping slows down cloth movement over time. Higher values make it stop sooner and also improve performance.
  float m_fDamping = 0.5f; // [ property ]

  /// How thick the cloth is, to prevent it from intersecting with other geometry.
  float m_fThickness = 0.05f; // [ property ]

  /// Tint color for the cloth material.
  ezColor m_Color = ezColor::White; // [ property ]

  /// Sets where the cloth is attached to the world.
  void SetFlags(ezBitflags<ezJoltClothSheetFlags> flags);                // [ property ]
  ezBitflags<ezJoltClothSheetFlags> GetFlags() const { return m_Flags; } // [ property ]

  ezMaterialResourceHandle m_hMaterial;                                  // [ property ]

private:
  void Update();

  void ApplyWind();

  void SetupCloth();
  void RemoveBody();
  void UpdateBodyBounds();

  float GetWeightValue() const { return m_fWeightValue; }
  void SetWeightValue_Scale(float fValue);
  void SetWeightValue_Mass(float fValue);

  ezVec2 m_vSize = ezVec2(1.0f, 1.0f);
  ezVec2 m_vTextureScale = ezVec2(1.0f);
  ezVec2U32 m_vSegments = ezVec2U32(16, 16);
  ezBitflags<ezJoltClothSheetFlags> m_Flags;
  mutable ezRenderData::Category m_RenderDataCategory;
  ezUInt8 m_uiSleepCounter = 0;

  ezUInt32 m_uiJoltBodyID = ezInvalidIndex;
  ezBoundingSphere m_BSphere;
};
