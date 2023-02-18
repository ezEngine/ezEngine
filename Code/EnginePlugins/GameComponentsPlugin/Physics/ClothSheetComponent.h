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

struct EZ_GAMECOMPONENTS_DLL ezClothSheetFlags
{
  using StorageType = ezUInt16;

  enum Enum
  {
    FixedCornerTopLeft = EZ_BIT(0),
    FixedCornerTopRight = EZ_BIT(1),
    FixedCornerBottomRight = EZ_BIT(2),
    FixedCornerBottomLeft = EZ_BIT(3),
    FixedEdgeTop = EZ_BIT(4),
    FixedEdgeRight = EZ_BIT(5),
    FixedEdgeBottom = EZ_BIT(6),
    FixedEdgeLeft = EZ_BIT(7),

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

class EZ_GAMECOMPONENTS_DLL ezClothSheetComponent : public ezRenderComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezClothSheetComponent, ezRenderComponent, ezClothSheetComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

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

  void SetSize(ezVec2 vVal);                 // [ property ]
  ezVec2 GetSize() const { return m_vSize; } // [ property ]

  void SetSlack(ezVec2 vVal);                  // [ property ]
  ezVec2 GetSlack() const { return m_vSlack; } // [ property ]

  void SetSegments(ezVec2U32 vVal);                     // [ property ]
  ezVec2U32 GetSegments() const { return m_vSegments; } // [ property ]

  float m_fWindInfluence = 0.3f;    // [ property ]
  float m_fDamping = 0.5f;          // [ property ]
  ezColor m_Color = ezColor::White; // [ property ]

  void SetFlags(ezBitflags<ezClothSheetFlags> flags);                // [ property ]
  ezBitflags<ezClothSheetFlags> GetFlags() const { return m_Flags; } // [ property ]

  void SetMaterialFile(const char* szFile); // [ property ]
  const char* GetMaterialFile() const;      // [ property ]

  ezMaterialResourceHandle m_hMaterial; // [ property ]

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
