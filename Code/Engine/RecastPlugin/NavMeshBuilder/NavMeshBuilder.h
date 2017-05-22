#pragma once

#include <RecastPlugin/Basics.h>
#include <GameEngine/AI/NavMesh/NavMeshDescription.h>
#include <Foundation/Types/UniquePtr.h>

class ezRcBuildContext;
struct rcPolyMesh;
struct rcPolyMeshDetail;

class ezRecastNavMeshBuilder
{
public:
  ezRecastNavMeshBuilder();
  ~ezRecastNavMeshBuilder();

  void Build(const ezNavMeshDescription& desc);

  rcPolyMesh* m_polyMesh = nullptr;
  rcPolyMeshDetail* m_detailMesh = nullptr;

private:
  void ReserveMemory(const ezNavMeshDescription& desc);
  void GenerateTriangleMeshFromDescription(const ezNavMeshDescription& desc);
  void ComputeBoundingBox();
  void BuildRecastNavMesh();
  void FillOutConfig(struct rcConfig& cfg);


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

  ezRcBuildContext* m_pRecastContext;

};

