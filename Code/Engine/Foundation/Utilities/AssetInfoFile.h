#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Variant.h>
#include <Foundation/Utilities/AssetFileHeader.h>

/// Key/value pairs describing the result of an asset transform, e.g. the vertex count of a mesh or the format of a texture.
///
/// Written as OpenDDL text next to the transformed output in the AssetCache folder. The file records the hash and type
/// version of the output it belongs to, so that a stale file can be detected like a stale output.
///
/// Most assets record nothing and therefore have no such file at all, so a missing file is not an error.
///
/// Values are untyped, because the file is written by several tools (editor, editor processor, TexConv) and consumed
/// generically. Use the key names below where they apply.
class EZ_FOUNDATION_DLL ezAssetInfoFile
{
public:
  /// Key names used across multiple asset types. Asset types may add arbitrary further keys.
  ///
  /// Must be valid DDL identifiers, because that is what they become in the file. For display they are run through
  /// ezTranslate, which splits CamelCase into words, so "ConvexParts" shows up as "Convex Parts".
  struct Keys
  {
    static constexpr ezStringView NumVertices = "Vertices"_ezsv;            ///< ezUInt32
    static constexpr ezStringView NumTriangles = "Triangles"_ezsv;          ///< ezUInt32
    static constexpr ezStringView NumSubMeshes = "Meshes"_ezsv;             ///< ezUInt32
    static constexpr ezStringView NumSurfaces = "Surfaces"_ezsv;            ///< ezUInt32
    static constexpr ezStringView NumBones = "Bones"_ezsv;                  ///< ezUInt32
    static constexpr ezStringView BoundsCenter = "Center"_ezsv;             ///< ezVec3, shown together with Extents as the resulting range
    static constexpr ezStringView BoundsHalfExtents = "Extents"_ezsv;       ///< ezVec3, so the full size is twice this
    static constexpr ezStringView BoundsRadius = "Radius"_ezsv;             ///< float
    static constexpr ezStringView ImageWidth = "Width"_ezsv;                ///< ezUInt32, shown together with Height as the resolution
    static constexpr ezStringView ImageHeight = "Height"_ezsv;              ///< ezUInt32
    static constexpr ezStringView Format = "Format"_ezsv;                   ///< ezString
    static constexpr ezStringView CollisionMeshType = "CollisionMesh"_ezsv; ///< ezString, e.g. "Triangle" or "ConvexHull"
    static constexpr ezStringView NumConvexParts = "ConvexParts"_ezsv;      ///< ezUInt32, only recorded when there is more than one
    static constexpr ezStringView AvailableClips = "Clips"_ezsv;            ///< ezVariantArray of ezString: animation clip names in the source file
    static constexpr ezStringView AvailableMeshes = "MeshesInSource"_ezsv;  ///< ezVariantArray of ezString: mesh names in the source file
  };

  /// Adds or overwrites a value. An invalid value removes the key.
  void SetValue(ezStringView sKey, const ezVariant& value);

  /// Returns an invalid variant if the key does not exist.
  ezVariant GetValue(ezStringView sKey) const;

  bool IsEmpty() const { return m_Values.IsEmpty(); }
  void Clear() { m_Values.Clear(); }

  const ezMap<ezString, ezVariant>& GetValues() const { return m_Values; }

  /// Writes the values and the header as OpenDDL.
  ///
  /// Always writes, even when the map is empty. Prefer WriteToFile(), which skips empty maps.
  ezResult Write(ezStreamWriter& inout_stream, const ezAssetFileHeader& header) const;

  /// Discards previous content. Values whose type this build cannot represent are skipped individually.
  ezResult Read(ezStreamReader& inout_stream, ezAssetFileHeader& out_header);

  /// Writes the file, or deletes any existing one if there is nothing to write.
  ezResult WriteToFile(ezStringView sAbsolutePath, const ezAssetFileHeader& header) const;

  /// Fails if the file does not exist, or if it was written for a different hash or type version, ie. if it is stale.
  ezResult ReadFromFile(ezStringView sAbsolutePath, ezUInt64 uiExpectedHash, ezUInt16 uiExpectedTypeVersion);

  /// Returns the path of the info file that belongs to the given transform output.
  static ezStringBuilder GetInfoFilePathForOutput(ezStringView sAbsoluteOutputPath);

  /// Appends all values, one "Name: value" per line, for display in the UI.
  ///
  /// Nothing is appended when there are no values, not even a separator. This shows everything, which is a lot for some
  /// asset types; for a short summary use ezAssetDocumentManager::AppendAssetInfoSummary() instead.
  void AppendToDisplayString(ezStringBuilder& ref_sOut, ezStringView sLinePrefix = "\n"_ezsv) const;

  /// Appends only the given keys, in the given order. Keys that have no value are skipped.
  void AppendValuesToDisplayString(ezStringBuilder& ref_sOut, ezArrayPtr<const ezStringView> keys, ezStringView sLinePrefix = "\n"_ezsv) const;

  /// Appends a single value. Returns false if there is nothing to show for that key.
  ///
  /// Some keys are formatted together with another one, e.g. ImageWidth prints the full resolution. The absorbed key
  /// (here ImageHeight) returns false on its own, so iterating over all keys does not print it twice.
  bool AppendValueToDisplayString(ezStringBuilder& ref_sOut, ezStringView sKey, ezStringView sLinePrefix = "\n"_ezsv) const;

private:
  // A map, not a hash table, so that the written files don't change just because the insertion order did.
  ezMap<ezString, ezVariant> m_Values;
};
