#pragma once

#include <Core/World/World.h>
#include <Foundation/Types/UniquePtr.h>
#include <ProceduralPlacementPlugin/Resources/ProceduralPlacementResource.h>

class ezProceduralPlacementComponent;
struct ezMsgUpdateLocalBounds;
struct ezMsgExtractRenderData;

//////////////////////////////////////////////////////////////////////////

class EZ_PROCEDURALPLACEMENTPLUGIN_DLL ezProceduralPlacementComponentManager
  : public ezComponentManager<ezProceduralPlacementComponent, ezBlockStorageType::Compact>
{
public:
  ezProceduralPlacementComponentManager(ezWorld* pWorld);
  ~ezProceduralPlacementComponentManager();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

private:
  friend class ezProceduralPlacementComponent;

  void UpdateTiles(const ezWorldModule::UpdateContext& context);
  void PreparePlace(const ezWorldModule::UpdateContext& context);
  void PlaceObjects(const ezWorldModule::UpdateContext& context);

  void AddComponent(ezProceduralPlacementComponent* pComponent);
  void RemoveComponent(ezProceduralPlacementComponent* pComponent);

  ezUInt32 AllocateTile(const ezPPInternal::TileDesc& desc, ezSharedPtr<const ezPPInternal::Layer>& pLayer);
  void DeallocateTile(ezUInt32 uiTileIndex);

  ezUInt32 AllocatePlacementTask(ezUInt32 uiTileIndex);
  void DeallocatePlacementTask(ezUInt32 uiPlacementTaskIndex);
  ezUInt32 GetNumAllocatedPlacementTasks() const;

  void RemoveTilesForComponent(ezProceduralPlacementComponent* pComponent, bool* out_bAnyObjectsRemoved = nullptr);
  void OnResourceEvent(const ezResourceEvent& resourceEvent);

  void AddVisibleComponent(const ezComponentHandle& hComponent, const ezVec3& cameraPosition, const ezVec3& cameraDirection) const;
  void ClearVisibleComponents();

  struct VisibleComponent
  {
    ezComponentHandle m_hComponent;
    ezVec3 m_vCameraPosition;
    ezVec3 m_vCameraDirection;
  };

  mutable ezMutex m_VisibleComponentsMutex;
  mutable ezDynamicArray<VisibleComponent> m_VisibleComponents;

  ezDynamicArray<ezComponentHandle> m_ComponentsToUpdate;

  ezDynamicArray<ezPPInternal::ActiveTile, ezAlignedAllocatorWrapper> m_ActiveTiles;
  ezDynamicArray<ezUInt32> m_FreeTiles;

  struct PlacementTaskInfo
  {
    EZ_ALWAYS_INLINE bool IsValid() const { return m_uiTileIndex != ezInvalidIndex; }
    EZ_ALWAYS_INLINE bool IsScheduled() const { return m_taskGroupID.IsValid(); }
    EZ_ALWAYS_INLINE void Invalidate()
    {
      m_taskGroupID = ezTaskGroupID();
      m_uiTileIndex = ezInvalidIndex;
    }

    ezUniquePtr<ezPPInternal::PlacementTask> m_pTask;
    ezTaskGroupID m_taskGroupID;
    ezUInt32 m_uiTileIndex;
  };

  ezDynamicArray<PlacementTaskInfo> m_PlacementTaskInfos;
  ezDynamicArray<ezUInt32> m_FreePlacementTasks;

  ezDynamicArray<ezPPInternal::TileDesc, ezAlignedAllocatorWrapper> m_NewTiles;
  ezTaskGroupID m_UpdateTilesTaskGroupID;
};

//////////////////////////////////////////////////////////////////////////

struct ezProcGenBoxExtents
{
  ezVec3 m_vOffset = ezVec3::ZeroVector();
  ezQuat m_Rotation = ezQuat::IdentityQuaternion();
  ezVec3 m_vExtents = ezVec3(10);

  ezResult Serialize(ezStreamWriter& stream) const;
  ezResult Deserialize(ezStreamReader& stream);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PROCEDURALPLACEMENTPLUGIN_DLL, ezProcGenBoxExtents);

class EZ_PROCEDURALPLACEMENTPLUGIN_DLL ezProceduralPlacementComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezProceduralPlacementComponent, ezComponent, ezProceduralPlacementComponentManager);

public:
  ezProceduralPlacementComponent();
  ~ezProceduralPlacementComponent();

  ezProceduralPlacementComponent& operator=(ezProceduralPlacementComponent&& other);

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void SetResourceFile(const char* szFile);
  const char* GetResourceFile() const;

  void SetResource(const ezProceduralPlacementResourceHandle& hResource);
  const ezProceduralPlacementResourceHandle& GetResource() const { return m_hResource; }

  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg);
  void OnExtractRenderData(ezMsgExtractRenderData& msg) const;

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

private:
  ezUInt32 BoxExtents_GetCount() const;
  const ezProcGenBoxExtents& BoxExtents_GetValue(ezUInt32 uiIndex) const;
  void BoxExtents_SetValue(ezUInt32 uiIndex, const ezProcGenBoxExtents& value);
  void BoxExtents_Insert(ezUInt32 uiIndex, const ezProcGenBoxExtents& value);
  void BoxExtents_Remove(ezUInt32 uiIndex);

  void UpdateBoundsAndTiles();

  ezProceduralPlacementResourceHandle m_hResource;

  ezDynamicArray<ezProcGenBoxExtents> m_BoxExtents;

  // runtime data
  friend class ezPPInternal::UpdateTilesTask;

  struct Bounds
  {
    EZ_DECLARE_POD_TYPE();

    ezSimdBBox m_GlobalBoundingBox;
    ezSimdTransform m_GlobalToLocalBoxTransform;
  };

  ezDynamicArray<Bounds, ezAlignedAllocatorWrapper> m_Bounds;

  struct ActiveLayer
  {
    ezSharedPtr<const ezPPInternal::Layer> m_pLayer;

    struct TileIndexAndAge
    {
      EZ_DECLARE_POD_TYPE();

      ezUInt32 m_uiIndex;
      ezUInt64 m_uiLastSeenFrame;
    };

    ezHashTable<ezUInt64, TileIndexAndAge> m_TileIndices;

    ezUniquePtr<ezPPInternal::UpdateTilesTask> m_pUpdateTilesTask;
  };

  ezDynamicArray<ActiveLayer> m_Layers;
};
