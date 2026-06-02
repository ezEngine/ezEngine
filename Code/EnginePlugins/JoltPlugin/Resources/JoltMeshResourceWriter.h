#pragma once

#include <JoltPlugin/JoltPluginDLL.h>

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Strings/String.h>

class ezStreamWriter;

struct EZ_JOLTPLUGIN_DLL ezJoltHeightfieldWriteDesc
{
  /// Hash of the source data. Written into the file so the export modifier can skip
  /// regeneration when the file is already up to date.
  ezUInt64 uiContentHash = 0;

  /// Grid vertex counts. Must satisfy Jolt requirements: even, >= 4, and uiSizeX == uiSizeY.
  ezUInt32 uiSizeX = 0;
  ezUInt32 uiSizeY = 0;

  /// X/Y half-extents of the heightfield in world units.
  ezVec2 vHalfExtent;

  /// Height samples, row-major with uiSizeX * uiSizeY entries.
  ezArrayPtr<const float> heights;

  /// Per-quad material indices, row-major with (uiSizeX-1)*(uiSizeY-1) entries.
  /// May be empty if there are no materials.
  ezArrayPtr<const ezUInt8> matIndices;

  /// Surface resource paths indexed by material index values.
  ezArrayPtr<const ezString> surfacePaths;

  /// Jolt collision layer stored in the file and forwarded to the collider component.
  ezUInt8 uiCollisionLayer = 0;
};

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

  /// Hash of the source data. Written uncompressed at the front of the file so the export
  /// modifier can check whether the file is up to date without decompressing it.
  ezUInt64 m_uiContentHash = 0;

  ezDynamicArray<ezVec3> m_Vertices;
  ezDynamicArray<ezUInt32> m_TriangleIndices;
  ezDynamicArray<ezUInt16> m_TriangleSurfaceID;

  ezDynamicArray<ezString> m_Surfaces;
};

/// \brief Helper class for writing ezJoltMeshResource and ezJoltHeightfieldResource files.
class EZ_JOLTPLUGIN_DLL ezJoltMeshResourceWriter
{
public:
  /// \brief Writes the given mesh description to the provided stream so that it can be loaded as an ezJoltMeshResource.
  ///
  /// Set bWriteAssetHeader to false if the asset header has already been written to the stream, e.g. in case of an asset transformation.
  static ezResult WriteMeshResource(const ezJoltMeshDesc& meshDesc, ezStreamWriter& inout_stream, bool bWriteAssetHeader = true, ezUInt64 uiAssetHash = 0);

  /// Cooks the Jolt heightfield shape and writes it to the provided stream so that it can be loaded as an ezJoltHeightfieldResource.
  ///
  /// Set bWriteAssetHeader to false if the asset header has already been written to the stream.
  static ezResult WriteHeightfieldResource(const ezJoltHeightfieldWriteDesc& desc, ezStreamWriter& inout_stream, bool bWriteAssetHeader = true, ezUInt64 uiAssetHash = 0);

private:
  static ezResult ComputeConvexHull(const ezDynamicArray<ezVec3>& vertices, ezDynamicArray<ezVec3>& out_hullVertices);

  static ezResult CookSingleConvexJoltMesh(const ezDynamicArray<ezVec3>& vertices, ezStreamWriter& inout_stream);

  static ezResult CookTriangleMesh(const ezJoltMeshDesc& meshDesc, ezStreamWriter& inout_stream);
  static ezResult CookConvexMesh(const ezJoltMeshDesc& meshDesc, ezStreamWriter& inout_stream);
  static ezResult CookDecomposedConvexMesh(const ezJoltMeshDesc& meshDesc, ezStreamWriter& inout_stream);
  static ezResult CookConvexHullGroup(const ezJoltMeshDesc& meshDesc, ezStreamWriter& inout_stream);
};
