#pragma once

#include <JoltPlugin/JoltPluginDLL.h>

#include <Foundation/Basics.h>

class ezStreamWriter;

struct ezJoltMeshDesc
{
  enum class Type : ezUInt8
  {
    Triangle,
    ConvexHull,
    ConvexDecomposition,
    ConvexHullGroup,
  };

  Type m_Type = Type::Triangle;
  bool m_bFlipNormals = false;
  ezUInt32 m_uiMaxConvexPieces = 1;

  ezDynamicArray<ezVec3> m_Vertices;
  ezDynamicArray<ezUInt32> m_TriangleIndices;
  ezDynamicArray<ezUInt16> m_TriangleSurfaceID;

  ezDynamicArray<ezString> m_Surfaces;
};

/// \brief Helper class for writing a ezJoltMeshResource to a stream.
class EZ_JOLTPLUGIN_DLL ezJoltMeshResourceWriter
{
public:
  /// \brief Writes the given mesh description to the provided stream so that it can be loaded as an ezJoltMeshResource.
  ///
  /// Set bWriteAssetHeader to false if the asset header has already been written to the stream, e.g. in case of an asset transformation.
  static ezResult WriteMeshResource(const ezJoltMeshDesc& meshDesc, ezStreamWriter& inout_stream, bool bWriteAssetHeader = true, ezUInt64 uiAssetHash = 0);

private:
  static ezResult ComputeConvexHull(const ezDynamicArray<ezVec3>& vertices, ezDynamicArray<ezVec3>& out_hullVertices);

  static ezResult CookSingleConvexJoltMesh(const ezDynamicArray<ezVec3>& vertices, ezStreamWriter& inout_stream);

  static ezResult CookTriangleMesh(const ezJoltMeshDesc& meshDesc, ezStreamWriter& inout_stream);
  static ezResult CookConvexMesh(const ezJoltMeshDesc& meshDesc, ezStreamWriter& inout_stream);
  static ezResult CookDecomposedConvexMesh(const ezJoltMeshDesc& meshDesc, ezStreamWriter& inout_stream);
  static ezResult CookConvexHullGroup(const ezJoltMeshDesc& meshDesc, ezStreamWriter& inout_stream);
};
