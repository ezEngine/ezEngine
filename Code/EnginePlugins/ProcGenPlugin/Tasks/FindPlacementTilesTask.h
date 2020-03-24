#pragma once

#include <Foundation/Threading/TaskSystem.h>
#include <ProcGenPlugin/Declarations.h>

#ifndef EmptyTileIndex
#  define EmptyTileIndex ezInvalidIndex
#endif

class ezProcPlacementComponent;

namespace ezProcGenInternal
{
  class FindPlacementTilesTask final : public ezTask
  {
  public:
    FindPlacementTilesTask(ezProcPlacementComponent* pComponent, ezUInt32 uiOutputIndex);
    ~FindPlacementTilesTask();

    void AddCameraPosition(const ezVec3& vCameraPosition) { m_vCameraPositions.PushBack(vCameraPosition); }

    ezArrayPtr<const PlacementTileDesc> GetNewTiles() const { return m_NewTiles; }
    ezArrayPtr<const ezUInt64> GetOldTiles() const { return m_OldTileKeys; }

  private:
    virtual void Execute() override;

    ezProcPlacementComponent* m_pComponent = nullptr;
    ezUInt32 m_uiOutputIndex = 0;

    ezHybridArray<ezVec3, 2> m_vCameraPositions;

    ezDynamicArray<PlacementTileDesc, ezAlignedAllocatorWrapper> m_NewTiles;
    ezDynamicArray<ezUInt64> m_OldTileKeys;

    struct TileByAge
    {
      EZ_DECLARE_POD_TYPE();

      ezUInt64 m_uiTileKey;
      ezUInt64 m_uiLastSeenFrame;
    };

    ezDynamicArray<TileByAge> m_TilesByAge;
  };

  EZ_ALWAYS_INLINE ezUInt64 GetTileKey(ezInt32 x, ezInt32 y)
  {
    ezUInt64 sx = (ezUInt32)x;
    ezUInt64 sy = (ezUInt32)y;

    return (sx << 32) | sy;
  }
} // namespace ezPPInternal
