#pragma once

#include <Core/World/World.h>
#include <Foundation/Types/UniquePtr.h>
#include <ProcGenPlugin/Resources/ProcGenGraphResource.h>

class ezProcPlacementComponent;
struct ezMsgUpdateLocalBounds;
struct ezMsgExtractRenderData;

//////////////////////////////////////////////////////////////////////////

class EZ_PROCGENPLUGIN_DLL ezProcPlacementComponentManager
  : public ezComponentManager<ezProcPlacementComponent, ezBlockStorageType::Compact>
{
public:
  ezProcPlacementComponentManager(ezWorld* pWorld);
  ~ezProcPlacementComponentManager();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

private:
  friend class ezProcPlacementComponent;

  void FindTiles(const ezWorldModule::UpdateContext& context);
  void PreparePlace(const ezWorldModule::UpdateContext& context);
  void PlaceObjects(const ezWorldModule::UpdateContext& context);

  void AddComponent(ezProcPlacementComponent* pComponent);
  void RemoveComponent(ezProcPlacementComponent* pComponent);

  ezUInt32 AllocateTile(const ezProcGenInternal::PlacementTileDesc& desc, ezSharedPtr<const ezProcGenInternal::PlacementOutput>& pOutput);
  void DeallocateTile(ezUInt32 uiTileIndex);

  ezUInt32 AllocateProcessingTask(ezUInt32 uiTileIndex);
  void DeallocateProcessingTask(ezUInt32 uiTaskIndex);
  ezUInt32 GetNumAllocatedProcessingTasks() const;

  void RemoveTilesForComponent(ezProcPlacementComponent* pComponent, bool* out_bAnyObjectsRemoved = nullptr);
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

  ezDynamicArray<ezProcGenInternal::PlacementTile, ezAlignedAllocatorWrapper> m_ActiveTiles;
  ezDynamicArray<ezUInt32> m_FreeTiles;

  struct ProcessingTask
  {
    EZ_ALWAYS_INLINE bool IsValid() const { return m_uiTileIndex != ezInvalidIndex; }
    EZ_ALWAYS_INLINE bool IsScheduled() const { return m_PlacementTaskGroupID.IsValid(); }
    EZ_ALWAYS_INLINE void Invalidate()
    {
      m_uiScheduledFrame = -1;
      m_PlacementTaskGroupID.Invalidate();
      m_uiTileIndex = ezInvalidIndex;
    }

    ezUInt64 m_uiScheduledFrame;
    ezUniquePtr<ezProcGenInternal::PreparePlacementTask> m_pPrepareTask;
    ezUniquePtr<ezProcGenInternal::PlacementTask> m_pPlacementTask;
    ezTaskGroupID m_PlacementTaskGroupID;
    ezUInt32 m_uiTileIndex;
  };

  ezDynamicArray<ProcessingTask> m_ProcessingTasks;
  ezDynamicArray<ezUInt32> m_FreeProcessingTasks;

  struct SortedProcessingTask
  {
    ezUInt64 m_uiScheduledFrame = 0;
    ezUInt32 m_uiTaskIndex = 0;
  };

  ezDynamicArray<SortedProcessingTask> m_SortedProcessingTasks;

  ezDynamicArray<ezProcGenInternal::PlacementTileDesc, ezAlignedAllocatorWrapper> m_NewTiles;
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

EZ_DECLARE_REFLECTABLE_TYPE(EZ_PROCGENPLUGIN_DLL, ezProcGenBoxExtents);

class EZ_PROCGENPLUGIN_DLL ezProcPlacementComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezProcPlacementComponent, ezComponent, ezProcPlacementComponentManager);

public:
  ezProcPlacementComponent();
  ~ezProcPlacementComponent();

  ezProcPlacementComponent& operator=(ezProcPlacementComponent&& other);

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void SetResourceFile(const char* szFile);
  const char* GetResourceFile() const;

  void SetResource(const ezProcGenGraphResourceHandle& hResource);
  const ezProcGenGraphResourceHandle& GetResource() const { return m_hResource; }

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

  ezProcGenGraphResourceHandle m_hResource;

  ezDynamicArray<ezProcGenBoxExtents> m_BoxExtents;

  // runtime data
  friend class ezProcGenInternal::FindPlacementTilesTask;

  struct Bounds
  {
    EZ_DECLARE_POD_TYPE();

    ezSimdBBox m_GlobalBoundingBox;
    ezSimdMat4f m_GlobalToLocalBoxTransform;
  };

  ezDynamicArray<Bounds, ezAlignedAllocatorWrapper> m_Bounds;

  struct OutputContext
  {
    ezSharedPtr<const ezProcGenInternal::PlacementOutput> m_pOutput;

    struct TileIndexAndAge
    {
      EZ_DECLARE_POD_TYPE();

      ezUInt32 m_uiIndex;
      ezUInt64 m_uiLastSeenFrame;
    };

    ezHashTable<ezUInt64, TileIndexAndAge> m_TileIndices;

    ezUniquePtr<ezProcGenInternal::FindPlacementTilesTask> m_pUpdateTilesTask;
  };

  ezDynamicArray<OutputContext> m_OutputContexts;
};
