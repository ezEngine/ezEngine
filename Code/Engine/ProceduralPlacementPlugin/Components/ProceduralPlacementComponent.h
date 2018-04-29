#pragma once

#include <ProceduralPlacementPlugin/Resources/ProceduralPlacementResource.h>
#include <ProceduralPlacementPlugin/Tasks/PlacementTask.h>
#include <Core/World/World.h>
#include <Foundation/Types/UniquePtr.h>

class ezProceduralPlacementComponent;
struct ezMsgUpdateLocalBounds;
struct ezMsgExtractRenderData;

//////////////////////////////////////////////////////////////////////////

class EZ_PROCEDURALPLACEMENTPLUGIN_DLL ezProceduralPlacementComponentManager : public ezComponentManager<ezProceduralPlacementComponent, ezBlockStorageType::Compact>
{
public:
  ezProceduralPlacementComponentManager(ezWorld* pWorld);
  ~ezProceduralPlacementComponentManager();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

private:
  friend class ezProceduralPlacementComponent;

  void Update(const ezWorldModule::UpdateContext& context);
  void PlaceObjects(const ezWorldModule::UpdateContext& context);

  void AddComponent(ezProceduralPlacementComponent* pComponent);
  void RemoveComponent(ezProceduralPlacementComponent* pComponent);

  ezUInt32 AllocateTile(const ezPPInternal::TileDesc& desc, ezSharedPtr<const ezPPInternal::Layer>& pLayer);
  void DeallocateTile(ezUInt32 uiTileIndex);

  ezUInt32 AllocatePlacementTask(ezUInt32 uiTileIndex);
  void DeallocatePlacementTask(ezUInt32 uiPlacementTaskIndex);
  ezUInt32 GetNumAllocatedPlacementTasks() const;

  void RemoveTilesForResource(ezUInt32 uiResourceIdHash);
  void OnResourceEvent(const ezResourceEvent& resourceEvent);

  void AddVisibleResource(const ezProceduralPlacementResourceHandle& hResource, const ezVec3& cameraPosition, const ezVec3& cameraDirection) const;
  void ClearVisibleResources();

  struct VisibleResource
  {
    ezProceduralPlacementResourceHandle m_hResource;
    ezVec3 m_vCameraPosition;
    ezVec3 m_vCameraDirection;
  };

  mutable ezMutex m_VisibleResourcesMutex;
  mutable ezDynamicArray<VisibleResource> m_VisibleResources;

  ezDynamicArray<ezProceduralPlacementResourceHandle> m_ResourcesToUpdate;

  struct ActiveLayer
  {
    ezSharedPtr<const ezPPInternal::Layer> m_pLayer;

    ezHashTable<ezUInt64, ezUInt32> m_TileIndices;
  };

  struct ActiveResource
  {
    struct Bounds
    {
      EZ_DECLARE_POD_TYPE();

      ezSimdBBox m_GlobalBoundingBox;
      ezSimdTransform m_LocalBoundingBox;
      ezComponentHandle m_hComponent;
    };

    ezDynamicArray<Bounds, ezAlignedAllocatorWrapper> m_Bounds;
    ezDynamicArray<ActiveLayer> m_Layers;
  };

  ezHashTable<ezUInt32, ActiveResource> m_ActiveResources;

  ezDynamicArray<ezPPInternal::ActiveTile, ezAlignedAllocatorWrapper> m_ActiveTiles;
  ezDynamicArray<ezUInt32> m_FreeTiles;

  struct PlacementTaskInfo
  {
    EZ_ALWAYS_INLINE bool IsValid() const { return m_uiTileIndex != ezInvalidIndex; }
    EZ_ALWAYS_INLINE bool IsScheduled() const { return m_taskGroupID.IsValid(); }
    EZ_ALWAYS_INLINE void Invalidate() { m_taskGroupID = ezTaskGroupID(); m_uiTileIndex = ezInvalidIndex; }

    ezUniquePtr<ezPPInternal::PlacementTask> m_pTask;
    ezTaskGroupID m_taskGroupID;
    ezUInt32 m_uiTileIndex;
  };

  ezDynamicArray<PlacementTaskInfo> m_PlacementTaskInfos;
  ezDynamicArray<ezUInt32> m_FreePlacementTasks;

  ezDynamicArray<ezPPInternal::TileDesc, ezAlignedAllocatorWrapper> m_NewTiles;
};

//////////////////////////////////////////////////////////////////////////

class EZ_PROCEDURALPLACEMENTPLUGIN_DLL ezProceduralPlacementComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezProceduralPlacementComponent, ezComponent, ezProceduralPlacementComponentManager);

public:
  ezProceduralPlacementComponent();
  ~ezProceduralPlacementComponent();

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void SetResourceFile(const char* szFile);
  const char* GetResourceFile() const;

  void SetResource(const ezProceduralPlacementResourceHandle& hResource);
  const ezProceduralPlacementResourceHandle& GetResource() const { return m_hResource; }

  void SetExtents(const ezVec3& value);
  const ezVec3& GetExtents() const { return m_vExtents; }

  void OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg);
  void OnExtractRenderData(ezMsgExtractRenderData& msg) const;

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

private:

  ezProceduralPlacementResourceHandle m_hResource;
  ezVec3 m_vExtents;
};
