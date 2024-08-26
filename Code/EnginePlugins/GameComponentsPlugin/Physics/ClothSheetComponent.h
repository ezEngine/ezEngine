#pragma once

#include <Core/World/ComponentManager.h>
#include <GameEngine/Physics/ClothSheetSimulator.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/Renderer.h>

using ezMaterialResourceHandle = ezTypedResourceHandle<class ezMaterialResource>;
using ezDynamicMeshBufferResourceHandle = ezTypedResourceHandle<class ezDynamicMeshBufferResource>;

//////////////////////////////////////////////////////////////////////////

class EZ_GAMECOMPONENTS_DLL ezClothSheetComponentManager : public ezComponentManager<class ezClothSheetComponent, ezBlockStorageType::FreeList>
{
public:
  ezClothSheetComponentManager(ezWorld* pWorld);
  ~ezClothSheetComponentManager();

  virtual void Initialize() override;

private:
  void Update(const ezWorldModule::UpdateContext& context);
  void UpdateBounds(const ezWorldModule::UpdateContext& context);
};

//////////////////////////////////////////////////////////////////////////

class EZ_GAMECOMPONENTS_DLL ezClothSheetRenderData final : public ezRenderData
{
  EZ_ADD_DYNAMIC_REFLECTION(ezClothSheetRenderData, ezRenderData);

public:
  ezUInt32 m_uiUniqueID = 0;
  ezArrayPtr<ezVec3> m_Positions;
  ezArrayPtr<ezUInt16> m_Indices;
  ezUInt16 m_uiVerticesX;
  ezUInt16 m_uiVerticesY;
  ezColor m_Color;

  ezMaterialResourceHandle m_hMaterial;
};

class EZ_GAMECOMPONENTS_DLL ezClothSheetRenderer : public ezRenderer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezClothSheetRenderer, ezRenderer);
  EZ_DISALLOW_COPY_AND_ASSIGN(ezClothSheetRenderer);

public:
  ezClothSheetRenderer();
  ~ezClothSheetRenderer();

  virtual void GetSupportedRenderDataCategories(ezHybridArray<ezRenderData::Category, 8>& ref_categories) const override;
  virtual void GetSupportedRenderDataTypes(ezHybridArray<const ezRTTI*, 8>& ref_types) const override;
  virtual void RenderBatch(const ezRenderViewContext& renderContext, const ezRenderPipelinePass* pPass, const ezRenderDataBatch& batch) const override;


protected:
  void CreateVertexBuffer();

  ezDynamicMeshBufferResourceHandle m_hDynamicMeshBuffer;
};

/// \brief Flags for how a piece of cloth should be simulated.
struct EZ_GAMECOMPONENTS_DLL ezClothSheetFlags
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

EZ_DECLARE_REFLECTABLE_TYPE(EZ_GAMECOMPONENTS_DLL, ezClothSheetFlags);

/// \brief Simulates a rectangular piece of cloth.
///
/// The cloth doesn't interact with the environment and doesn't collide with any geometry.
/// The component samples the wind simulation and applies wind forces to the cloth.
///
/// Cloth sheets can be used as decorative elements like flags that blow in the wind.
class EZ_GAMECOMPONENTS_DLL ezClothSheetComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezClothSheetComponent, ezRenderComponent, ezClothSheetComponentManager);

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
  // ezClothSheetComponent

public:
  ezClothSheetComponent();
  ~ezClothSheetComponent();

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

  /// How much sag the cloth should have along each axis.
  void SetSlack(ezVec2 vVal);                  // [ property ]
  ezVec2 GetSlack() const { return m_vSlack; } // [ property ]

  /// A factor to tweak how strong the wind can push the cloth.
  float m_fWindInfluence = 0.3f; // [ property ]

  /// Damping slows down cloth movement over time. Higher values make it stop sooner and also improve performance.
  float m_fDamping = 0.5f; // [ property ]

  /// Tint color for the cloth material.
  ezColor m_Color = ezColor::White; // [ property ]

  /// Sets where the cloth is attached to the world.
  void SetFlags(ezBitflags<ezClothSheetFlags> flags);                // [ property ]
  ezBitflags<ezClothSheetFlags> GetFlags() const { return m_Flags; } // [ property ]

  ezMaterialResourceHandle m_hMaterial;                              // [ property ]

private:
  void Update();
  void SetupCloth();

  ezVec2 m_vSize;
  ezVec2 m_vSlack;
  ezVec2U32 m_vSegments;
  ezBitflags<ezClothSheetFlags> m_Flags;

  ezUInt8 m_uiSleepCounter = 0;
  mutable ezUInt8 m_uiVisibleCounter = 0;
  ezUInt8 m_uiCheckEquilibriumCounter = 0;
  ezClothSimulator m_Simulator;

  ezBoundingBox m_Bbox;
};
