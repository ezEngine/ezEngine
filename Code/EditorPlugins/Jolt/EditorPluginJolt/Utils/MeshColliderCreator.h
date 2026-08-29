#pragma once

#include <EditorPluginJolt/EditorPluginJoltDLL.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/ArrayPtr.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/Uuid.h>
#include <Foundation/Types/Variant.h>

/// Which kind of collision mesh asset to generate from a mesh asset.
struct ezMeshColliderKind
{
  using StorageType = ezUInt8;

  enum Enum
  {
    /// ezJoltConvexCollisionMeshAsset. Required for dynamic actors.
    ConvexHull,

    /// ezJoltCollisionMeshAsset. Concave, but only usable for static geometry.
    TriangleMesh,

    Default = ConvexHull
  };
};

/// \see ezMeshColliderCreator::CreateMeshCollider()
struct ezMeshColliderOptions
{
  /// Where to write the asset. Absolute, or relative to the parent of a data directory
  /// ("Testing Chambers/Objects/Barrel.ezJoltCollisionMeshAsset"). Empty means the suggested path,
  /// which is what creating colliders for several meshes at once uses.
  /// \see ezMeshColliderCreator::SuggestColliderPath()
  ezString m_sColliderPath;

  ezEnum<ezMeshColliderKind> m_Kind;

  /// The surface asset to assign, as a guid or an asset path. Only convex meshes have a single
  /// surface; a triangle mesh gets one per material slot at transform time, so this is ignored there.
  ezString m_sSurface;

  /// Overwrites a collision mesh asset that already exists instead of refusing.
  ///
  /// The contents are rewritten in place, so the asset keeps its guid and references to it still
  /// resolve. Hand-tuned values on it are lost.
  bool m_bOverwriteExisting = false;

  bool m_bOpenAfterCreate = false;
};

/// The import settings of a mesh asset, as far as a collision mesh asset can reproduce them.
///
/// Every value is stored as a variant that is written to the collision mesh asset property of the
/// same name, so that neither side's C++ type has to be known here. Values that the mesh asset does
/// not have (an animated mesh has no transform options) are invalid variants and are then left at
/// the collision mesh asset's own default.
struct EZ_EDITORPLUGINJOLT_DLL ezMeshColliderSource
{
  ezUuid m_MeshAssetGuid;
  ezString m_sMeshAssetPath;

  /// The mesh asset's "MeshFile". Empty if it could not be read or the mesh is a primitive rather
  /// than an imported file, in which case no collision mesh can be generated from it.
  ezString m_sMeshFile;

  /// True for a primitive mesh (ezMeshPrimitive other than File), which has no source file to import.
  /// Kept separate from an empty m_sMeshFile so that the reason can be reported.
  bool m_bIsPrimitive = false;

  bool m_bAnimated = false;

  /// The import options shared with the collision mesh asset, keyed by property name.
  ezVariantDictionary m_ImportProperties;

  /// An existing collision mesh asset of that kind built from the same source file, if there is one.
  ezUuid m_ExistingTriangleColMesh;
  ezUuid m_ExistingConvexColMesh;

  /// The existing collision mesh asset of the given kind, or an invalid uuid.
  ezUuid GetExisting(ezEnum<ezMeshColliderKind> kind) const;
};

/// Creates Jolt collision mesh assets from mesh assets. \see ezMeshColliderUtils
///
/// Only settings that both asset types have are transferred, everything else stays at the collision
/// mesh asset's default.
class EZ_EDITORPLUGINJOLT_DLL ezMeshColliderCreator
{
public:
  /// Fails if the guid does not belong to a mesh asset.
  ///
  /// Opens the mesh asset document to read its properties, if it is not open already.
  static ezResult GatherMeshColliderSource(const ezUuid& meshAssetGuid, ezMeshColliderSource& out_source);

  /// Creates and saves the collision mesh asset document.
  ///
  /// Fails if a file already exists at the target path, unless ezMeshColliderOptions::m_bOverwriteExisting
  /// is set. Reusing an existing collider is otherwise up to the caller.
  /// \see ezMeshColliderSource::GetExisting()
  static ezStatus CreateMeshCollider(const ezMeshColliderSource& source, const ezMeshColliderOptions& options);

  /// Creates a collider for each of the given mesh assets, each at its suggested path.
  ///
  /// Meshes that already have a collider at that path, and ones no collider can be built from, are
  /// skipped with a log message rather than failing the whole run. Only an outright error, such as a
  /// document that cannot be written, is reported back.
  static ezStatus CreateMeshColliders(ezArrayPtr<const ezUuid> meshAssetGuids, const ezMeshColliderOptions& options, ezUInt32& out_uiCreated, ezUInt32& out_uiSkipped);

  /// Whether the given guid refers to a mesh or animated mesh asset.
  static bool IsMeshAsset(const ezUuid& assetGuid);

  /// The default absolute path for a collider of that kind, next to the mesh asset.
  ///
  /// Appends a number if that file is already taken, unless bAllowExisting is set.
  static ezString SuggestColliderPath(const ezMeshColliderSource& source, ezEnum<ezMeshColliderKind> kind, bool bAllowExisting = false);

  /// Turns an absolute path into one relative to the parent of its data directory, for display.
  /// Returns the input unchanged if it is not inside a data directory.
  static ezString MakeDisplayPath(ezStringView sAbsolutePath);

  /// Resolves what MakeDisplayPath() produced, or any absolute path, back to an absolute path.
  /// Fails if the path names no known data directory.
  static ezResult ResolveDisplayPath(ezStringView sPath, ezStringBuilder& out_sAbsolutePath);
};
