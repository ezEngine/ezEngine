#pragma once

#include <Foundation/Types/Status.h>
#include <JoltCooking/JoltCookingDLL.h>

class ezStreamWriter;
class ezChunkStreamWriter;

struct EZ_JOLTCOOKING_DLL ezJoltCookingMesh
{
  bool m_bFlipNormals = false;
  ezDynamicArray<ezVec3> m_Vertices;
  ezDynamicArray<ezUInt8> m_VerticesInPolygon;
  ezDynamicArray<ezUInt32> m_PolygonIndices;
  ezDynamicArray<ezUInt16> m_PolygonSurfaceID;
};

class EZ_JOLTCOOKING_DLL ezJoltCooking
{
public:
  enum class MeshType
  {
    Triangle,
    ConvexHull,
    ConvexDecomposition
  };

  static ezResult CookTriangleMesh(const ezJoltCookingMesh& mesh, ezStreamWriter& OutputStream);
  static ezResult CookConvexMesh(const ezJoltCookingMesh& mesh, ezStreamWriter& OutputStream);
  static ezResult ComputeConvexHull(const ezJoltCookingMesh& mesh, ezJoltCookingMesh& outMesh);
  static ezStatus WriteResourceToStream(ezChunkStreamWriter& stream, const ezJoltCookingMesh& mesh, const ezArrayPtr<ezString>& surfaces, MeshType meshType, ezUInt32 uiMaxConvexPieces = 1);

#ifdef BUILDSYSTEM_ENABLE_VHACD_SUPPORT
  static ezResult CookDecomposedConvexMesh(const ezJoltCookingMesh& mesh, ezStreamWriter& OutputStream, ezUInt32 uiMaxConvexPieces);
#endif

private:
  static ezResult CookSingleConvexJoltMesh(const ezJoltCookingMesh& mesh, ezStreamWriter& OutputStream);
};
