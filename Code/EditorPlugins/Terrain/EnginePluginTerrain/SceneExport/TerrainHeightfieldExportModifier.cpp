#include <EnginePluginTerrain/EnginePluginTerrainPCH.h>

#include <EnginePluginTerrain/SceneExport/TerrainHeightfieldExportModifier.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <JoltPlugin/Actors/JoltHeightfieldColliderComponent.h>
#include <JoltPlugin/Resources/JoltHeightfieldResource.h>
#include <JoltPlugin/Resources/JoltMeshResourceWriter.h>
#include <TerrainPlugin/Components/TerrainPatchComponent.h>
#include <TerrainPlugin/TerrainSystem.h>

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

    if (pPatch->GetCollider() == ezTerrainPatchColliderMode::None)
      continue;

    const ezUInt32 uiHeightfieldIdx = pPatch->GetHeightfieldIndex();
    if (uiHeightfieldIdx == ezInvalidIndex)
      continue;

    const ezUInt32 numCellsPerSide = pTerrain->GetHeightfieldCellsPerSide(uiHeightfieldIdx);
    PatchGeometry patchGeo;
    ComputePatchGeometry(pPatch, numCellsPerSide, patchGeo);

    const ezUInt64 uiContentHash = pPatch->ComputeColliderContentHash(pTerrain->GetHeightfieldBrushOverlapHash(uiHeightfieldIdx));

    ezStringBuilder sPath;
    sPath.SetFormat(":project/AssetCache/Generated/TerrainPatch_{}.ezBinJoltHeightfield", ezArgU(pPatch->GetStableId(), 16, true, 16, true));

    const bool bFileUpToDate = CheckExistingHeightfieldFileContentHash(sPath, uiContentHash);

    if (!bFileUpToDate)
    {
      // Issue synchronous GPU->CPU readback.
      ezTempArray<float> bakedHeights;
      ezTempArray<ezUInt8> dominantIndices;
      if (pTerrain->ReadbackHeightfieldData(uiHeightfieldIdx, bakedHeights, dominantIndices).Failed())
      {
        ezLog::Warning("TerrainHeightfieldExportModifier: ReadbackHeightfieldData failed for patch (stableId={}), skipping baked collider.", pPatch->GetStableId());
        continue;
      }

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
