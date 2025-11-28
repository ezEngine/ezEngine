#pragma once

#include <JoltPlugin/JoltPluginDLL.h>

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
  bool m_bWriteAssetHeader = true;
  ezUInt32 m_uiMaxConvexPieces = 1;

  ezDynamicArray<ezVec3> m_Vertices;
  ezDynamicArray<ezUInt32> m_TriangleIndices;
  ezDynamicArray<ezUInt16> m_TriangleSurfaceID;

  ezDynamicArray<ezString> m_Surfaces;
};

/// \brief Interface for writing jolt mesh resources. Used internally by ezJoltMeshResourceUtils::WriteMeshResource.
class EZ_JOLTPLUGIN_DLL ezJoltMeshResourceWriterInterface
{
public:
  static ezResult WriteMeshResource(ezJoltMeshDesc&& meshDesc, ezStreamWriter& inout_stream, ezUInt64 uiAssetHash = 0);

protected:
  virtual ezResult WriteMeshResourceInternal(ezJoltMeshDesc&& meshDesc, ezStreamWriter& inout_stream, ezUInt64 uiAssetHash) = 0;
};
