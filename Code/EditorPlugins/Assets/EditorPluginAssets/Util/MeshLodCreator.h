#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Types/ArrayPtr.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Types/Uuid.h>

/// \see ezMeshLodCreator::CreateMeshLods()
struct ezMeshLodOptions
{
  /// How many LOD assets to create, starting at LOD-1. LOD 0 is the mesh asset itself, which is not
  /// touched. Clamped to ezMeshLodCreator::s_uiMaxLods.
  ezUInt32 m_uiLodCount = 2;

  /// Overwrites LOD assets that already exist instead of leaving them alone.
  bool m_bOverwriteExisting = false;

  bool m_bOpenAfterCreate = false;
};

/// What a mesh asset offers for LOD creation. Filled by GatherMeshLodSource().
struct EZ_EDITORPLUGINASSETS_DLL ezMeshLodSource
{
  ezUuid m_MeshAssetGuid;
  ezString m_sMeshAssetPath;

  /// The mesh asset's "MeshFile". Empty for a procedural primitive, which cannot get LODs.
  ezString m_sMeshFile;

  /// The mesh asset's "MeshIncludeTags". When set, the mesh is one sub-object of the model file
  /// rather than all of it, which decides where its LODs may go - see GetLodFolderCandidates().
  ezString m_sMeshIncludeTags;

  bool m_bIsPrimitive = false;
  bool m_bAnimated = false;

  /// The folder the LOD assets belong in, absolute. This is the folder the prefab tool looks in, so
  /// that a prefab created afterwards picks the LODs up by itself.
  ezString m_sLodFolder;

  /// How much the mesh asset itself is already simplified, as the percentage of triangles removed.
  /// 0 when it does not simplify at all. The generated LODs continue past this.
  ezUInt8 m_uiBaseSimplification = 0;

  /// The import settings every LOD has to share with the mesh asset, keyed by property name.
  ezVariantDictionary m_ImportProperties;

  /// The mesh asset's material slots, so that the LODs render with the same materials. Each entry
  /// holds the "Label" and "Resource" of one slot.
  ezDynamicArray<ezVariantDictionary> m_MaterialSlots;

  /// Guids of LOD-1..N that already exist, in order, with an invalid uuid for each gap. Shorter than
  /// the requested count when the trailing LODs do not exist.
  ezDynamicArray<ezUuid> m_ExistingLods;

  /// Whether LOD-N already exists. uiLod is 1 based.
  bool HasLod(ezUInt32 uiLod) const;
};

/// Creates the LOD mesh assets that sit next to a mesh asset.
///
/// The LODs are the same model imported again with progressively stronger mesh simplification, the
/// same way ezMeshAssetDocumentGenerator does it for a model that is imported with LODs enabled.
///
/// Names and locations match what ezMeshPrefabCreator looks for, so that a prefab created from the
/// mesh afterwards finds the LODs and builds an ezLodMeshComponent.
class EZ_EDITORPLUGINASSETS_DLL ezMeshLodCreator
{
public:
  /// The most LODs the mesh import generates, and therefore the most the prefab tool looks for.
  static constexpr ezUInt32 s_uiMaxLods = 4;

  /// Fails if the guid does not belong to a mesh asset.
  ///
  /// Opens the mesh asset document to read its properties, if it is not open already.
  static ezResult GatherMeshLodSource(const ezUuid& meshAssetGuid, ezMeshLodSource& out_source);

  /// Creates and saves the LOD documents.
  ///
  /// Existing LOD assets are left alone unless ezMeshLodOptions::m_bOverwriteExisting is set.
  static ezStatus CreateMeshLods(const ezMeshLodSource& source, const ezMeshLodOptions& options, ezUInt32& out_uiCreated, ezUInt32& out_uiSkipped);

  /// Creates LODs for each of the given mesh assets.
  ///
  /// Meshes that cannot get LODs are skipped with a log message rather than failing the whole run.
  static ezStatus CreateMeshLodsForAll(ezArrayPtr<const ezUuid> meshAssetGuids, const ezMeshLodOptions& options, ezUInt32& out_uiCreated, ezUInt32& out_uiSkipped);

  /// Whether the given guid refers to a mesh or animated mesh asset.
  static bool IsMeshAsset(const ezUuid& assetGuid);

  /// The simplification for LOD uiLod (1 based), as the percentage of triangles removed.
  ///
  /// Each level takes the midpoint between the level before it and 100: from an unsimplified mesh
  /// that is 50, 75, 88, 94; from a base of 50 it is 75, 88, 94. Never reaches 100, which would
  /// remove the whole mesh.
  static ezUInt8 GetLodSimplification(ezUInt8 uiBaseSimplification, ezUInt32 uiLod);

  /// The tolerated error for LOD uiLod (1 based), as a percentage. Grows with the level. Mirrors
  /// what the mesh import uses.
  static ezUInt8 GetLodSimplificationError(ezUInt32 uiLod);

  /// The absolute path of LOD uiLod (1 based) for that source.
  static ezString GetLodPath(const ezMeshLodSource& source, ezUInt32 uiLod);

  /// The folders that LODs of that mesh asset can sit in, most likely first.
  ///
  /// The mesh import writes them to "<Name>_data", using the mesh asset's name at import time. After
  /// a rename that folder still has the old name, which was derived from the source model file, hence
  /// the second candidate. sMeshFile may be empty, which yields one candidate.
  ///
  /// sMeshIncludeTags is the mesh asset's "MeshIncludeTags". When it is set, the mesh is only one
  /// sub-object of the model file, and several mesh assets typically share that file - each picking a
  /// different part of it. The folder named after the source file is then dropped, because all of
  /// those assets would resolve to it and claim each other's LODs, which renders the wrong geometry
  /// at distance rather than merely a worse one. Only the mesh asset's own name remains.
  static void GetLodFolderCandidates(ezStringView sMeshAssetPath, ezStringView sMeshFile, ezStringView sMeshIncludeTags, ezDynamicArray<ezString>& out_folders);
};
