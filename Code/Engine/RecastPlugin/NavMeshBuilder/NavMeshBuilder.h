#pragma once

#include <RecastPlugin/Basics.h>
#include <GameEngine/AI/NavMesh/NavMeshDescription.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Reflection/Reflection.h>

class ezRcBuildContext;
struct rcPolyMesh;
struct rcPolyMeshDetail;
class ezWorld;
class dtNavMesh;

struct ezRecastConfig
{
  float m_fAgentHeight = 1.5f;
  float m_fAgentRadius = 0.3f;
  float m_fAgentClimbHeight = 0.4f;
  ezAngle m_WalkableSlope = ezAngle::Degree(45);
  float m_fCellSize = 0.2f;
  float m_fCellHeight = 0.2f;
  float m_fMaxEdgeLength = 4.0f;
  float m_fMaxSimplificationError = 1.3f;
  float m_fMinRegionSize = 3.0f;
  float m_fRegionMergeSize = 20.0f;
  float m_fDetailMeshSampleDistanceFactor = 1.0f;
  float m_fDetailMeshSampleErrorFactor = 1.0f;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RECASTPLUGIN_DLL, ezRecastConfig);



class ezRecastNavMeshBuilder
{
public:
  ezRecastNavMeshBuilder();
  ~ezRecastNavMeshBuilder();

  ezResult Build(const ezRecastConfig& config, const ezWorld& world);
  ezResult Build(const ezRecastConfig& config, const ezNavMeshDescription& desc);

  rcPolyMesh* m_polyMesh = nullptr;
  dtNavMesh* m_pNavMesh = nullptr;

private:
  void ReserveMemory(const ezNavMeshDescription& desc);
  void GenerateTriangleMeshFromDescription(const ezNavMeshDescription& desc);
  void ComputeBoundingBox();
  ezResult BuildRecastNavMesh(const ezRecastConfig& config);
  void FillOutConfig(struct rcConfig& cfg, const ezRecastConfig& config);


  struct Triangle
  {
    Triangle() {}
    Triangle(ezInt32 a, ezInt32 b, ezInt32 c)
    {
      m_VertexIdx[0] = a;
      m_VertexIdx[1] = b;
      m_VertexIdx[2] = c;
    }

    ezInt32 m_VertexIdx[3];
  };

  ezBoundingBox m_BoundingBox;
  ezDynamicArray<ezVec3> m_Vertices;
  ezDynamicArray<Triangle> m_Triangles;
  ezDynamicArray<ezUInt8> m_TriangleAreaIDs;

  ezRcBuildContext* m_pRecastContext = nullptr;

  ezResult CreateDetourNavMesh(const ezRecastConfig& config);
};

