#pragma once

#include <JoltCooking/JoltCookingDLL.h>

#include <Foundation/Types/Status.h>

class ezStreamWriter;
class ezChunkStreamWriter;

struct EZ_JOLTCOOKING_DLL ezJoltCookingMesh
{
  ezDynamicArray<ezVec3> m_Vertices;
  ezDynamicArray<ezUInt32> m_TriangleIndices;
  ezDynamicArray<ezUInt16> m_TriangleSurfaceID;
};

class EZ_JOLTCOOKING_DLL ezJoltCooking
{
public:
  enum class MeshType
  {
    Triangle,
    ConvexHull,
    ConvexDecomposition,
    ConvexHullGroup,
  };

  static ezResult CookTriangleMesh(const ezJoltCookingMesh& mesh, ezStreamWriter& ref_outputStream);
  static ezResult CookConvexMesh(const ezJoltCookingMesh& mesh, ezStreamWriter& ref_outputStream);
  static ezResult ComputeConvexHull(const ezJoltCookingMesh& mesh, ezJoltCookingMesh& out_mesh);
  static ezStatus WriteResourceToStream(ezChunkStreamWriter& inout_stream, const ezJoltCookingMesh& mesh, const ezArrayPtr<ezString>& surfaces, MeshType meshType, ezUInt32 uiMaxConvexPieces = 1);
  static ezResult CookDecomposedConvexMesh(const ezJoltCookingMesh& mesh, ezStreamWriter& ref_outputStream, ezUInt32 uiMaxConvexPieces);
  static ezResult CookConvexHullGroup(const ezJoltCookingMesh& mesh, ezStreamWriter& ref_outputStream);

private:
  static ezResult CookSingleConvexJoltMesh(const ezJoltCookingMesh& mesh, ezStreamWriter& OutputStream);
};
