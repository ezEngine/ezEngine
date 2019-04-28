#pragma once

#include <Foundation/Threading/TaskSystem.h>
#include <ProceduralPlacementPlugin/ProceduralPlacementPluginDLL.h>

#ifndef EmptyTileIndex
#  define EmptyTileIndex ezInvalidIndex
#endif

class ezProceduralPlacementComponent;

namespace ezPPInternal
{
  class UpdateTilesTask : public ezTask
  {
  public:
    UpdateTilesTask(ezProceduralPlacementComponent* pComponent, ezUInt32 uiLayerIndex);
    ~UpdateTilesTask();

    void AddCameraPosition(const ezVec3& vCameraPosition) { m_vCameraPositions.PushBack(vCameraPosition); }

    ezArrayPtr<const TileDesc> GetNewTiles() const { return m_NewTiles; }

  private:
    virtual void Execute() override;

    ezProceduralPlacementComponent* m_pComponent = nullptr;
    ezUInt32 m_uiLayerIndex = 0;

    ezHybridArray<ezVec3, 2> m_vCameraPositions;

    ezDynamicArray<TileDesc, ezAlignedAllocatorWrapper> m_NewTiles;
  };

  EZ_ALWAYS_INLINE ezUInt64 GetTileKey(ezInt32 x, ezInt32 y)
  {
    ezUInt64 sx = (ezUInt32)x;
    ezUInt64 sy = (ezUInt32)y;
    
    return (sx << 32) | sy;
  }
} // namespace ezPPInternal
