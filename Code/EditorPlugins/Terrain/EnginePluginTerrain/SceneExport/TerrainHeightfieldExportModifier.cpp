#include <EnginePluginTerrain/EnginePluginTerrainPCH.h>

#include <EnginePluginTerrain/SceneExport/TerrainHeightfieldExportModifier.h>

#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <JoltPlugin/Actors/JoltHeightfieldColliderComponent.h>
#include <JoltPlugin/Resources/JoltHeightfieldResource.h>
#include <JoltPlugin/Resources/JoltMeshResourceWriter.h>
#include <TerrainPlugin/Components/TerrainPatchComponent.h>
#include <TerrainPlugin/TerrainSystem.h>
#include <meshoptimizer/meshoptimizer.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneExportModifier_TerrainHeightfieldCollision, 1, ezRTTIDefaultAllocator<ezSceneExportModifier_TerrainHeightfieldCollision>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static bool CheckExistingHeightfieldFileContentHash(ezStringView sPath, ezUInt64 uiExpectedHash)
{
  ezFileReader file;
  if (file.Open(sPath).Failed())
    return false;

  ezAssetFileHeader header;
  if (header.Read(file).Failed())
    return false;

  ezUInt8 uiVersion = 0;
  ezUInt8 uiCompressionMode = 0;
  file >> uiVersion;
  file >> uiCompressionMode;

  ezUInt64 uiStoredHash = 0;
  file >> uiStoredHash;

  return uiStoredHash == uiExpectedHash;
}

/// Bump this when BuildOccluderMesh() changes, to discard cached meshes from the older algorithm.
static constexpr ezUInt8 g_uiOccluderAlgorithmVersion = 1;
static constexpr ezUInt8 g_uiOccluderFileVersion = 1;

/// Fails when the cache file doesn't exist or was built from different inputs.
static ezResult ReadCachedOccluder(ezStringView sPath, ezUInt64 uiExpectedHash, ezDynamicArray<ezVec3>& out_vertices, ezDynamicArray<ezUInt32>& out_indices)
{
  ezFileReader file;
  if (file.Open(sPath).Failed())
    return EZ_FAILURE;

  ezUInt8 uiVersion = 0;
  file >> uiVersion;

  if (uiVersion != g_uiOccluderFileVersion)
    return EZ_FAILURE;

  ezUInt64 uiStoredHash = 0;
  file >> uiStoredHash;

  if (uiStoredHash != uiExpectedHash)
    return EZ_FAILURE;

  EZ_SUCCEED_OR_RETURN(file.ReadArray(out_vertices));
  EZ_SUCCEED_OR_RETURN(file.ReadArray(out_indices));

  return EZ_SUCCESS;
}

static ezResult WriteCachedOccluder(ezStringView sPath, ezUInt64 uiContentHash, const ezDynamicArray<ezVec3>& vertices, const ezDynamicArray<ezUInt32>& indices)
{
  ezDeferredFileWriter file;
  file.SetOutput(sPath);

  file << g_uiOccluderFileVersion;
  file << uiContentHash;

  EZ_SUCCEED_OR_RETURN(file.WriteArray(vertices));
  EZ_SUCCEED_OR_RETURN(file.WriteArray(indices));

  return file.Close();
}

struct PatchGeometry
{
  ezUInt32 uiVertexCount = 0; ///< Number of vertices per side of the collider grid
  float fJoltHalfExtent = 0.0f;
  float fColliderCenter = 0.0f;
  ezUInt32 uiStoredStart = 0;
  ezUInt32 uiSubsampleStride = 0; ///< Step size in full-res cells between collider vertices
};

static void ComputePatchGeometry(const ezTerrainPatchComponent* pPatch, ezUInt32 uiCellsPerSide, PatchGeometry& out_patchGeo)
{
  const float fFullGridSpacing = pPatch->GetSize() / static_cast<float>(uiCellsPerSide);
  out_patchGeo.uiSubsampleStride = static_cast<ezUInt32>(pPatch->GetCollider().GetValue());
  const ezUInt32 uiSubsampledCells = uiCellsPerSide / out_patchGeo.uiSubsampleStride;
  out_patchGeo.uiVertexCount = uiSubsampledCells + 2;
  const float fGridSpacing = fFullGridSpacing * static_cast<float>(out_patchGeo.uiSubsampleStride);
  out_patchGeo.fJoltHalfExtent = static_cast<float>(out_patchGeo.uiVertexCount - 1) * fGridSpacing * 0.5f;
  // iHalfExtra: how many extra subsampled vertices the collider grid extends beyond the render
  // patch on each side (subsampled coords). uiStoredStart adjusts from the stored border (offset 4)
  // so the collider vertices align with the center of the patch.
  const ezInt32 iHalfExtra = static_cast<ezInt32>((out_patchGeo.uiVertexCount - 1) * out_patchGeo.uiSubsampleStride - uiCellsPerSide) / 2;
  out_patchGeo.uiStoredStart = static_cast<ezUInt32>(ezMath::Max(0, 4 - iHalfExtra));
  const float fLocalStart = static_cast<float>(static_cast<ezInt32>(out_patchGeo.uiStoredStart) - 4) * fFullGridSpacing;
  out_patchGeo.fColliderCenter = fLocalStart + out_patchGeo.fJoltHalfExtent;
}

/// Builds the coarse occluder mesh for one patch.
///
/// The heights are reduced to a grid of about fCellSize meters per cell, where every vertex takes the minimum
/// of all full resolution heights of the cells it belongs to. Since a triangle never rises above its highest
/// corner, the result stays below the real terrain, so it can't cull objects standing on it. Cells containing
/// carved (tunnel, cave) samples are dropped; the resulting holes can only lose occlusion, never add any.
///
/// Decimation is the one step that isn't strictly conservative: meshopt only removes vertices, so the surface
/// can rise across a narrow gully. The error budget (a tenth of the cell size) stays well below the pushdown
/// that the min filter already produces on any real slope (roughly cell size * tan(slope)).
static void BuildOccluderMesh(ezUInt32 uiCellsPerSide, float fPatchSize, float fCellSize, ezArrayPtr<const float> bakedHeights, ezArrayPtr<const ezUInt8> dominantMat, ezDynamicArray<ezVec3>& out_vertices, ezDynamicArray<ezUInt32>& out_indices)
{
  out_vertices.Clear();
  out_indices.Clear();

  const ezInt32 iStoredRowStride = static_cast<ezInt32>(uiCellsPerSide) + 9;
  const float fFullSpacing = fPatchSize / static_cast<float>(uiCellsPerSide);

  // Safety net only, the caller clamps to the collider grid spacing, which is coarser than this.
  fCellSize = ezMath::Clamp(fCellSize, fFullSpacing, fPatchSize);
  const ezUInt32 uiStride = ezMath::Clamp(static_cast<ezUInt32>(ezMath::RoundToInt(fCellSize / fFullSpacing)), 1u, uiCellsPerSide);

  // Rounded up, so the grid spans the full patch. A stride that doesn't divide the patch evenly leaves the
  // last row and column narrower than the rest.
  const ezUInt32 uiCells = (uiCellsPerSide + uiStride - 1) / uiStride;
  const ezUInt32 uiVerts = uiCells + 1;

  const float fZSnap = fCellSize / 5.0f;
  const bool bHasDominantIndices = dominantMat.GetCount() == bakedHeights.GetCount();

  // Full resolution grid coordinate of each coarse grid line, clamped to the patch edge.
  ezTempArray<ezUInt32> lineToFine;
  lineToFine.SetCountUninitialized(uiVerts);
  for (ezUInt32 i = 0; i < uiVerts; ++i)
  {
    lineToFine[i] = ezMath::Min(i * uiStride, uiCellsPerSide);
  }

  ezTempArray<float> heights;
  heights.SetCountUninitialized(uiVerts * uiVerts);

  for (ezUInt32 cy = 0; cy < uiVerts; ++cy)
  {
    for (ezUInt32 cx = 0; cx < uiVerts; ++cx)
    {
      // The stored grid has a 4 vertex border ring around the rendered patch, hence the +4.
      const ezInt32 iCenterX = 4 + static_cast<ezInt32>(lineToFine[cx]);
      const ezInt32 iCenterY = 4 + static_cast<ezInt32>(lineToFine[cy]);
      const ezInt32 iRadius = static_cast<ezInt32>(uiStride);

      float fMin = ezMath::MaxValue<float>();
      bool bCarved = false;

      for (ezInt32 dy = -iRadius; dy <= iRadius && !bCarved; ++dy)
      {
        const ezInt32 iY = ezMath::Clamp(iCenterY + dy, 0, iStoredRowStride - 1);

        for (ezInt32 dx = -iRadius; dx <= iRadius; ++dx)
        {
          const ezInt32 iX = ezMath::Clamp(iCenterX + dx, 0, iStoredRowStride - 1);
          const ezUInt32 uiIdx = static_cast<ezUInt32>(iY * iStoredRowStride + iX);

          if (bHasDominantIndices && dominantMat[uiIdx] == 0xFFu)
          {
            bCarved = true;
            break;
          }

          fMin = ezMath::Min(fMin, bakedHeights[uiIdx]);
        }
      }

      // Snapping downwards stays conservative and makes near-flat regions exactly planar, so that the
      // decimation can collapse them completely.
      if (!bCarved)
      {
        fMin = ezMath::Floor(fMin / fZSnap) * fZSnap;
      }

      heights[cy * uiVerts + cx] = bCarved ? ezMath::NaN<float>() : fMin;
    }
  }

  // Emit only the vertices that a kept cell actually uses.
  ezTempArray<ezUInt32> vertexRemap;
  vertexRemap.SetCount(uiVerts * uiVerts, ezInvalidIndex);

  auto GetVertex = [&](ezUInt32 x, ezUInt32 y) -> ezUInt32
  {
    ezUInt32& uiRemapped = vertexRemap[y * uiVerts + x];

    if (uiRemapped == ezInvalidIndex)
    {
      uiRemapped = out_vertices.GetCount();
      out_vertices.PushBack(ezVec3(lineToFine[x] * fFullSpacing, lineToFine[y] * fFullSpacing, heights[y * uiVerts + x]));
    }

    return uiRemapped;
  };

  for (ezUInt32 y = 0; y + 1 < uiVerts; ++y)
  {
    for (ezUInt32 x = 0; x + 1 < uiVerts; ++x)
    {
      if (ezMath::IsNaN(heights[y * uiVerts + x]) || ezMath::IsNaN(heights[y * uiVerts + x + 1]) ||
          ezMath::IsNaN(heights[(y + 1) * uiVerts + x]) || ezMath::IsNaN(heights[(y + 1) * uiVerts + x + 1]))
        continue;

      const ezUInt32 i00 = GetVertex(x, y);
      const ezUInt32 i10 = GetVertex(x + 1, y);
      const ezUInt32 i01 = GetVertex(x, y + 1);
      const ezUInt32 i11 = GetVertex(x + 1, y + 1);

      // Counter-clockwise seen from +Z, so the faces point up and are backface culled from below.
      out_indices.PushBack(i00);
      out_indices.PushBack(i10);
      out_indices.PushBack(i11);

      out_indices.PushBack(i00);
      out_indices.PushBack(i11);
      out_indices.PushBack(i01);
    }
  }

  const ezUInt32 uiGridTriangles = out_indices.GetCount() / 3;

  const float fError = fCellSize / 10.0f;

  if (uiGridTriangles > 0)
  {
    ezTempArray<ezUInt32> simplified;
    simplified.SetCountUninitialized(out_indices.GetCount());

    float fResultError = 0.0f;
    const size_t uiNumIndices = meshopt_simplify(simplified.GetData(), out_indices.GetData(), out_indices.GetCount(),
      &out_vertices[0].x, out_vertices.GetCount(), sizeof(ezVec3), 0, fError, meshopt_SimplifyErrorAbsolute, &fResultError);

    simplified.SetCount(static_cast<ezUInt32>(uiNumIndices));
    out_indices = simplified;

    // meshopt keeps referencing the original vertex buffer, so drop the vertices that no triangle uses anymore.
    ezTempArray<ezVec3> compacted;
    compacted.SetCountUninitialized(out_vertices.GetCount());

    const size_t uiNumVertices = meshopt_optimizeVertexFetch(compacted.GetData(), out_indices.GetData(), out_indices.GetCount(),
      out_vertices.GetData(), out_vertices.GetCount(), sizeof(ezVec3));

    compacted.SetCount(static_cast<ezUInt32>(uiNumVertices));
    out_vertices = compacted;

    ezLog::Info("Terrain occluder: {} cells ({} m), {} -> {} triangles, error {} m", uiCells, ezArgF(fCellSize, 2), uiGridTriangles, out_indices.GetCount() / 3, ezArgF(fResultError, 3));
  }
}

void ezSceneExportModifier_TerrainHeightfieldCollision::ModifyWorld(ezWorld& ref_world, ezStringView sDocumentType, const ezUuid& documentGuid, bool bForExport)
{
  EZ_LOCK(ref_world.GetWriteMarker());

  ezTerrainSystem* pTerrain = ref_world.GetOrCreateModule<ezTerrainSystem>();
  if (pTerrain == nullptr)
    return;

  auto* pPatchMan = ref_world.GetComponentManager<ezTerrainPatchComponentManager>();
  if (pPatchMan == nullptr)
    return;

  auto* pColliderMan = ref_world.GetOrCreateComponentManager<ezJoltHeightfieldColliderComponentManager>();

  for (auto it = pPatchMan->GetComponents(); it.IsValid(); ++it)
  {
    ezTerrainPatchComponent* pPatch = it;

    if (!pPatch->IsActive())
      continue;

    const bool bWantsCollider = pPatch->GetCollider() != ezTerrainPatchColliderMode::None;
    const bool bWantsOccluder = pPatch->GetOcclusionCellSize() > 0.0f;

    if (!bWantsCollider)
    {
      if (bWantsOccluder)
      {
        ezLog::Warning("TerrainHeightfieldExportModifier: patch (stableId={}) has an occlusion cell size but no collider. The occluder is baked from the collider data and is skipped as well.", pPatch->GetStableId());
      }

      continue;
    }

    const ezUInt32 uiHeightfieldIdx = pPatch->GetHeightfieldIndex();
    if (uiHeightfieldIdx == ezInvalidIndex)
      continue;

    const ezUInt32 numCellsPerSide = pTerrain->GetHeightfieldCellsPerSide(uiHeightfieldIdx);
    const float fFullGridSpacing = pPatch->GetSize() / static_cast<float>(numCellsPerSide);

    PatchGeometry patchGeo;
    ComputePatchGeometry(pPatch, numCellsPerSide, patchGeo);

    const ezUInt64 uiContentHash = pPatch->ComputeColliderContentHash(pTerrain->GetHeightfieldBrushOverlapHash(uiHeightfieldIdx));

    ezStringBuilder sPath;
    sPath.SetFormat(":project/AssetCache/Generated/TerrainPatch_{}.ezBinJoltHeightfield", ezArgU(pPatch->GetStableId(), 16, true, 16, true));

    const bool bColliderFileUpToDate = CheckExistingHeightfieldFileContentHash(sPath, uiContentHash);

    // A detail level that's good enough for collision is also good enough for occlusion.
    const float fOccluderCellSize = ezMath::Clamp(pPatch->GetOcclusionCellSize(), fFullGridSpacing * static_cast<float>(patchGeo.uiSubsampleStride), pPatch->GetSize());

    ezStringBuilder sOccluderPath;
    ezUInt64 uiOccluderHash = 0;
    bool bOccluderCacheUpToDate = false;

    ezTempArray<ezVec3> occluderVertices;
    ezTempArray<ezUInt32> occluderIndices;

    if (bWantsOccluder)
    {
      ezHashStreamWriter64 hashWriter;
      hashWriter << uiContentHash;
      hashWriter << fOccluderCellSize;
      hashWriter << numCellsPerSide;
      hashWriter << g_uiOccluderAlgorithmVersion;
      uiOccluderHash = hashWriter.GetHashValue();

      sOccluderPath.SetFormat(":project/AssetCache/Generated/TerrainOccluder_{}.ezBinTerrainOccluder", ezArgU(pPatch->GetStableId(), 16, true, 16, true));

      bOccluderCacheUpToDate = ReadCachedOccluder(sOccluderPath, uiOccluderHash, occluderVertices, occluderIndices).Succeeded();
    }

    // One readback serves both, it forces a full re-bake of the patch.
    const bool bNeedsReadback = !bColliderFileUpToDate || (bWantsOccluder && !bOccluderCacheUpToDate);

    ezTempArray<float> bakedHeights;
    ezTempArray<ezUInt8> dominantIndices;

    if (bNeedsReadback && pTerrain->ReadbackHeightfieldData(uiHeightfieldIdx, bakedHeights, dominantIndices).Failed())
    {
      ezLog::Warning("TerrainHeightfieldExportModifier: ReadbackHeightfieldData failed for patch (stableId={}), skipping baked collider and occluder.", pPatch->GetStableId());
      continue;
    }

    if (bWantsOccluder)
    {
      if (!bOccluderCacheUpToDate)
      {
        BuildOccluderMesh(numCellsPerSide, pPatch->GetSize(), fOccluderCellSize, bakedHeights, dominantIndices, occluderVertices, occluderIndices);

        if (WriteCachedOccluder(sOccluderPath, uiOccluderHash, occluderVertices, occluderIndices).Failed())
        {
          // Not fatal, only costs bake time on the next run.
          ezLog::Warning("TerrainHeightfieldExportModifier: failed to write occluder cache '{}'.", sOccluderPath);
        }
      }

      pPatch->SetBakedOccluder(occluderVertices, occluderIndices);
    }

    if (!bColliderFileUpToDate)
    {
      const ezUInt32 uiStoredRowStride = numCellsPerSide + 9;
      const bool bHasDominantIndices = dominantIndices.GetCount() == bakedHeights.GetCount();
      constexpr float fJoltNoCollision = std::numeric_limits<float>::max();

      ezTempArray<float> heights;
      heights.SetCountUninitialized(patchGeo.uiVertexCount * patchGeo.uiVertexCount);
      for (ezUInt32 row = 0; row < patchGeo.uiVertexCount; ++row)
      {
        const ezUInt32 srcRow = ezMath::Min(patchGeo.uiStoredStart + row * patchGeo.uiSubsampleStride, uiStoredRowStride - 1);
        for (ezUInt32 col = 0; col < patchGeo.uiVertexCount; ++col)
        {
          const ezUInt32 srcCol = ezMath::Min(patchGeo.uiStoredStart + col * patchGeo.uiSubsampleStride, uiStoredRowStride - 1);
          const ezUInt32 srcIdx = srcRow * uiStoredRowStride + srcCol;

          if (bHasDominantIndices && dominantIndices[srcIdx] == 0xFFu)
          {
            heights[row * patchGeo.uiVertexCount + col] = fJoltNoCollision;
          }
          else
          {
            heights[row * patchGeo.uiVertexCount + col] = bakedHeights[srcIdx];
          }
        }
      }

      ezTempArray<ezUInt8> matIndices;
      ezTempArray<ezString> surfacePaths;

      const ezUInt32 uiNumSurfaces = pPatch->Surfaces_GetCount();
      if (uiNumSurfaces > 0 && bHasDominantIndices)
      {
        surfacePaths.SetCount(uiNumSurfaces);

        for (ezUInt32 surfaceIdx = 0; surfaceIdx < uiNumSurfaces; ++surfaceIdx)
        {
          surfacePaths[surfaceIdx] = pPatch->Surfaces_GetValue(surfaceIdx);
        }

        const ezUInt32 uiNumQuads = (patchGeo.uiVertexCount - 1) * (patchGeo.uiVertexCount - 1);
        matIndices.SetCountUninitialized(uiNumQuads);
        for (ezUInt32 row = 0; row < patchGeo.uiVertexCount - 1; ++row)
        {
          const ezUInt32 srcSubRow = patchGeo.uiVertexCount - 1 - row;
          const ezUInt32 srcFullRow = ezMath::Min(patchGeo.uiStoredStart + srcSubRow * patchGeo.uiSubsampleStride, uiStoredRowStride - 1);
          for (ezUInt32 col = 0; col < patchGeo.uiVertexCount - 1; ++col)
          {
            const ezUInt32 srcFullCol = ezMath::Min(patchGeo.uiStoredStart + col * patchGeo.uiSubsampleStride, uiStoredRowStride - 1);
            const ezUInt8 matIdx = dominantIndices[srcFullRow * uiStoredRowStride + srcFullCol];
            matIndices[row * (patchGeo.uiVertexCount - 1) + col] = (matIdx < uiNumSurfaces) ? matIdx : 0;
          }
        }
      }

      ezJoltHeightfieldWriteDesc desc;
      desc.uiContentHash = uiContentHash;
      desc.uiSizeX = patchGeo.uiVertexCount;
      desc.uiSizeY = patchGeo.uiVertexCount;
      desc.vHalfExtent = ezVec2(patchGeo.fJoltHalfExtent, patchGeo.fJoltHalfExtent);
      desc.heights = heights.GetArrayPtr();
      desc.matIndices = matIndices.GetArrayPtr();
      desc.surfacePaths = surfacePaths.GetArrayPtr();
      desc.uiCollisionLayer = 0;

      {
        ezDeferredFileWriter fileWriter;
        fileWriter.SetOutput(sPath);
        if (ezJoltMeshResourceWriter::WriteHeightfieldResource(desc, fileWriter).Failed() || fileWriter.Close().Failed())
        {
          ezLog::Error("TerrainHeightfieldExportModifier: failed to write '{}'.", sPath);
          continue;
        }
      }
    }

    pPatch->SetCollider(ezTerrainPatchColliderMode::None);

    // Create or reuse the "HeightfieldCollider" child game object at the correct local offset.
    ezGameObject* pColliderObject = nullptr;
    ezGameObject* pPatchOwner = pPatch->GetOwner();
    for (auto childIt = pPatchOwner->GetChildren(); childIt.IsValid(); ++childIt)
    {
      if (childIt->GetNameHashed() == ezTempHashedString("HeightfieldCollider"))
      {
        pColliderObject = &(*childIt);
        break;
      }
    }

    if (pColliderObject == nullptr)
    {
      ezGameObjectDesc objDesc;
      objDesc.m_sName.Assign("HeightfieldCollider");
      objDesc.m_hParent = pPatchOwner->GetHandle();
      objDesc.m_LocalPosition = ezVec3(patchGeo.fColliderCenter, patchGeo.fColliderCenter, 0.0f);
      ref_world.CreateObject(objDesc, pColliderObject);
    }
    else
    {
      pColliderObject->SetLocalPosition(ezVec3(patchGeo.fColliderCenter, patchGeo.fColliderCenter, 0.0f));
    }

    ezJoltHeightfieldColliderComponent* pCollider = nullptr;
    if (!pColliderObject->TryGetComponentOfBaseType(pCollider))
    {
      pColliderMan->CreateComponent(pColliderObject, pCollider);
    }

    pCollider->m_hHeightfield = ezResourceManager::LoadResource<ezJoltHeightfieldResource>(sPath);
  }
}

EZ_STATICLINK_FILE(EnginePluginTerrain, EnginePluginTerrain_SceneExport_TerrainHeightfieldExportModifier);
