#pragma once

#include <AiPlugin/Navigation/NavigationConfig.h>
#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>
#include <Recast.h>
#include <RendererCore/Debug/DebugRendererContext.h>

using ezDataBuffer = ezDynamicArray<ezUInt8>;

class ezPhysicsWorldModuleInterface;
class dtNavMesh;

/// \brief Stores indices for a triangle.
struct ezAiNavMeshTriangle final
{
  EZ_DECLARE_POD_TYPE();

  ezAiNavMeshTriangle() = default;
  ezAiNavMeshTriangle(ezInt32 a, ezInt32 b, ezInt32 c)
  {
    m_VertexIdx[0] = a;
    m_VertexIdx[1] = b;
    m_VertexIdx[2] = c;
  }

  ezInt32 m_VertexIdx[3];
};

/// \brief Stores the geometry from which a navmesh should be generated.
struct ezAiNavMeshInputGeo final
{
  ezDynamicArray<ezVec3> m_Vertices;
  ezDynamicArray<ezAiNavMeshTriangle> m_Triangles;
  ezDynamicArray<ezUInt8> m_TriangleAreaIDs;
};

/// \brief State about a single sector (tile / cell) of an ezAiNavMesh
struct ezAiNavMeshSector final
{
  ezAiNavMeshSector();
  ~ezAiNavMeshSector();

  ezUInt8 m_FlagRequested : 1;
  ezUInt8 m_FlagInvalidate : 1;
  ezUInt8 m_FlagUpdateAvailable : 1;
  ezUInt8 m_FlagUsable : 1;

  ezDataBuffer m_NavmeshDataCur;
  ezDataBuffer m_NavmeshDataNew;
  dtTileRef m_TileRef = 0;
};

/// \brief A navmesh generated with a specific configuration.
///
/// Each game may use multiple navmeshes for different character types (large, small, etc).
/// All navmeshes always exist, but only some may contain data.
/// You get access to a navmesh through the ezAiNavMeshWorldModule.
///
/// To do a path search, use ezAiNavigation.
/// Since the navmesh is built in the background, a path search may need to run for multiple frames,
/// before it can return any result.
class EZ_AIPLUGIN_DLL ezAiNavMesh final
{
  EZ_DISALLOW_COPY_AND_ASSIGN(ezAiNavMesh);

public:
  ezAiNavMesh(ezUInt32 uiNumSectorsX, ezUInt32 uiNumSectorsY, float fSectorMetersXY, const ezAiNavmeshConfig& navmeshConfig);
  ~ezAiNavMesh();

  using SectorID = ezUInt32;

  ezVec2I32 CalculateSectorCoord(float fPositionX, float fPositionY) const;
  ezVec2 GetSectorPositionOffset(ezVec2I32 coord) const;
  ezBoundingBox GetSectorBounds(ezVec2I32 coord, float fMinZ = 0.0f, float fMaxZ = 1.0f) const;
  float GetSectorSize() const { return m_fSectorMetersXY; }
  SectorID CalculateSectorID(ezVec2I32 coord) const { return coord.y * m_uiNumSectorsX + coord.x; }
  ezVec2I32 CalculateSectorCoord(SectorID sectorID) const;

  const ezAiNavMeshSector* GetSector(SectorID sectorID) const;

  /// \brief Marks the sector as requested.
  ///
  /// Returns true, if the sector is already available, false when it needs to be built first.
  bool RequestSector(SectorID sectorID);

  /// \brief Marks all sectors within the given rectangle as requested.
  ///
  /// Returns true, if all the sectors are already available, false when any of them needs to be built first.
  bool RequestSector(const ezVec2& vCenter, const ezVec2& vHalfExtents);

  // void InvalidateSector(SectorID sectorID);

  void FinalizeSectorUpdates();

  SectorID RetrieveRequestedSector();
  void BuildSector(SectorID sectorID, const ezPhysicsWorldModuleInterface* pPhysics);

  const dtNavMesh* GetDetourNavMesh() const { return m_pNavMesh; }

  void DebugDraw(ezDebugRendererContext context, const ezAiNavigationConfig& config);

private:
  void DebugDrawSector(ezDebugRendererContext context, const ezAiNavigationConfig& config, int iTileIdx);

  ezAiNavmeshConfig m_NavmeshConfig;

  ezUInt32 m_uiNumSectorsX = 0;
  ezUInt32 m_uiNumSectorsY = 0;
  float m_fSectorMetersXY = 0;
  float m_fInvSectorMetersXY = 0;

  dtNavMesh* m_pNavMesh = nullptr;
  ezMap<SectorID, ezAiNavMeshSector> m_Sectors;
  ezDeque<SectorID> m_RequestedSectors;
  // ezHybridArray<SectorID, 32> m_InvalidatedSectors;

  ezMutex m_Mutex;
  ezDynamicArray<SectorID> m_UpdatingSectors;
};
