#pragma once

#include <EditorPluginScene/EditorPluginSceneDLL.h>
#include <Foundation/Types/ArrayPtr.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/Uuid.h>

/// How a prefab created from a mesh should be set up for physics.
struct ezMeshPrefabPhysics
{
  using StorageType = ezUInt8;

  enum Enum
  {
    None,

    /// Generates an ezJoltCollisionMeshAsset next to the mesh asset, unless a suitable one exists.
    /// Triangle meshes cannot be used for dynamic actors.
    StaticTriangleMesh,

    /// Fills in concave parts of the shape.
    StaticConvexHull,

    /// Sized from the mesh bounds, so it needs no additional asset but requires ezMeshPrefabSource::m_bHasBounds.
    StaticBox,

    /// Generates an ezJoltConvexCollisionMeshAsset. Dynamic actors require a convex shape.
    DynamicConvexHull,

    DynamicBox,

    Default = None
  };
};

/// \see ezMeshPrefabCreator::CreateMeshPrefab()
struct ezMeshPrefabOptions
{
  /// Where to write the prefab. Absolute, or relative to the parent of a data directory
  /// ("Testing Chambers/Objects/Barrel.ezPrefab"). Empty means the suggested path, which is what
  /// creating prefabs for several meshes at once uses.
  /// \see ezMeshPrefabCreator::SuggestPrefabPath()
  ezString m_sPrefabPath;

  /// RTTI name of the component that renders the mesh. Empty falls back to
  /// ezMeshPrefabSource::GetDefaultRenderComponentType().
  ezString m_sRenderComponentType;

  ezEnum<ezMeshPrefabPhysics> m_Physics;
  ezUInt8 m_uiCollisionLayer = 0;
  ezString m_sSurfaceAsset;

  /// Overwrites a prefab that already exists instead of refusing.
  ///
  /// The contents are rewritten in place, so the prefab keeps its guid and references to it still
  /// resolve. Changes made to it by hand are lost.
  bool m_bOverwriteExisting = false;

  bool m_bOpenAfterCreate = true;
};

/// What a mesh asset offers for prefab creation. Filled by GatherMeshPrefabSource().
struct EZ_EDITORPLUGINSCENE_DLL ezMeshPrefabSource
{
  ezUuid m_MeshAssetGuid;
  ezString m_sMeshAssetPath;

  /// The mesh asset's "MeshFile" property. Empty if it could not be read, in which case no collision
  /// mesh asset can be generated.
  ezString m_sMeshFile;

  /// LOD-1 and up, in ascending order. Does not include the mesh asset itself, which is LOD 0.
  ezDynamicArray<ezUuid> m_LodGuids;

  bool m_bAnimated = false;

  /// An existing collision mesh asset built from the same source file, if there is one.
  ezUuid m_ExistingTriangleColMesh;
  ezUuid m_ExistingConvexColMesh;

  /// False when the asset was never transformed, in which case the box collider modes can't be used.
  bool m_bHasBounds = false;
  ezVec3 m_vBoundsCenter = ezVec3::MakeZero();
  ezVec3 m_vBoundsHalfExtents = ezVec3(0.5f);
  float m_fBoundsRadius = 1.0f;

  /// ezAnimatedMeshComponent for animated meshes, ezLodMeshComponent when LODs were found,
  /// otherwise ezMeshComponent.
  ezStringView GetDefaultRenderComponentType() const;

  /// Derived from an existing collision mesh asset built from the same source. None when there is
  /// none, so that generating a collider is a deliberate choice.
  ezEnum<ezMeshPrefabPhysics> GetDefaultPhysics() const;
};

/// Creates prefab documents that display a single mesh.
///
/// Components are added by RTTI name, so that this does not depend on the mesh or physics plugins.
/// Consequently the physics options only work while the Jolt plugin is loaded.
class EZ_EDITORPLUGINSCENE_DLL ezMeshPrefabCreator
{
public:
  /// Fails if the guid does not belong to a mesh asset. Missing bounds are not a failure, they are
  /// reported through ezMeshPrefabSource::m_bHasBounds.
  ///
  /// Opens the mesh asset document to read its source file, if it is not open already.
  static ezResult GatherMeshPrefabSource(const ezUuid& meshAssetGuid, ezMeshPrefabSource& out_source);

  /// Creates and saves the prefab document, plus a collision mesh asset if the physics option needs one.
  ///
  /// Fails if a file already exists at the target path, unless
  /// ezMeshPrefabOptions::m_bOverwriteExisting is set.
  static ezStatus CreateMeshPrefab(const ezMeshPrefabSource& source, const ezMeshPrefabOptions& options);

  /// Creates a prefab for each of the given mesh assets, each at its suggested path.
  ///
  /// Meshes that already have a prefab at that path, and ones the chosen options cannot be applied
  /// to, are skipped with a log message rather than failing the whole run. Only an outright error,
  /// such as a document that cannot be written, is reported back.
  ///
  /// The render component type is decided per mesh, so that a selection can mix animated and static
  /// meshes; ezMeshPrefabOptions::m_sRenderComponentType is ignored here.
  static ezStatus CreateMeshPrefabs(ezArrayPtr<const ezUuid> meshAssetGuids, const ezMeshPrefabOptions& options, ezUInt32& out_uiCreated, ezUInt32& out_uiSkipped);

  /// The default absolute path for a mesh asset's prefab, next to it.
  ///
  /// Appends a number if that file is already taken, unless bAllowExisting is set.
  static ezString SuggestPrefabPath(const ezMeshPrefabSource& source, bool bAllowExisting = false);

  /// Turns an absolute path into one relative to the parent of its data directory, for display.
  /// Returns the input unchanged if it is not inside a data directory.
  static ezString MakeDisplayPath(ezStringView sAbsolutePath);

  /// Resolves what MakeDisplayPath() produced, or any absolute path, back to an absolute path.
  /// Fails if the path names no known data directory.
  static ezResult ResolveDisplayPath(ezStringView sPath, ezStringBuilder& out_sAbsolutePath);

  /// Whether the Jolt plugin is loaded, i.e. whether the physics options can be used at all.
  static bool IsPhysicsAvailable();

  /// Whether the given guid refers to a mesh or animated mesh asset.
  static bool IsMeshAsset(const ezUuid& assetGuid);
};
