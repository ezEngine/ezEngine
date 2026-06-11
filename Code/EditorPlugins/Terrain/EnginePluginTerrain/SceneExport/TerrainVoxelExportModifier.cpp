#include <EnginePluginTerrain/EnginePluginTerrainPCH.h>

#include <EnginePluginTerrain/SceneExport/TerrainVoxelExportModifier.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <JoltPlugin/Actors/JoltStaticActorComponent.h>
#include <JoltPlugin/Resources/JoltMeshResource.h>
#include <JoltPlugin/Resources/JoltMeshResourceWriter.h>
#include <TerrainPlugin/Components/TerrainVolumeComponent.h>
#include <TerrainPlugin/TerrainSystem.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneExportModifier_TerrainVoxelCollision, 1, ezRTTIDefaultAllocator<ezSceneExportModifier_TerrainVoxelCollision>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static bool CheckExistingVoxelMeshFileContentHash(ezStringView sPath, ezUInt64 uiExpectedHash)
{
  ezFileReader file;
  if (file.Open(sPath).Failed())
    return false;

  // JoltMeshResource files written via WriteMeshResource start with an asset file header,
  // followed by the version byte, compression mode, and content hash.
  ezAssetFileHeader header;
  if (header.Read(file).Failed())
    return false;

  ezUInt8 uiVersion = 0;
  ezUInt8 uiCompressionMode = 0;
  file >> uiVersion;
  file >> uiCompressionMode;

  if (uiVersion < 4)
    return false;

  ezUInt64 uiStoredHash = 0;
  file >> uiStoredHash;

  return uiStoredHash == uiExpectedHash;
}

void ezSceneExportModifier_TerrainVoxelCollision::ModifyWorld(ezWorld& ref_world, ezStringView sDocumentType, const ezUuid& documentGuid, bool bForExport)
{
  EZ_LOCK(ref_world.GetWriteMarker());

  ezTerrainSystem* pTerrain = ref_world.GetOrCreateModule<ezTerrainSystem>();
  if (pTerrain == nullptr)
    return;

  auto* pVolumeMan = ref_world.GetComponentManager<ezTerrainVolumeComponentManager>();
  if (pVolumeMan == nullptr)
    return;

  auto* pActorMan = ref_world.GetOrCreateComponentManager<ezJoltStaticActorComponentManager>();

  for (auto it = pVolumeMan->GetComponents(); it.IsValid(); ++it)
  {
    ezTerrainVolumeComponent* pVolume = it;

    if (!pVolume->IsActive())
      continue;

    if (!pVolume->GetEnableCollider())
      continue;

    const ezUInt32 uiVoxelIdx = pVolume->GetVoxelIndex();
    if (uiVoxelIdx == ezInvalidIndex)
      continue;

    const ezUInt64 uiContentHash = pVolume->ComputeColliderContentHash(pTerrain->GetVoxelBrushOverlapHash(uiVoxelIdx));

    ezStringBuilder sPath;
    sPath.SetFormat(":project/AssetCache/Generated/TerrainVolume_{}.ezBinJoltTriangleMesh", ezArgU(pVolume->GetStableId(), 16, true, 16, true));

    const bool bFileUpToDate = CheckExistingVoxelMeshFileContentHash(sPath, uiContentHash);

    if (!bFileUpToDate)
    {
      ezTempArray<VoxelGpuVertex> cpuVerts;
      ezTempArray<ezUInt32> cpuIdxs;
      ezUInt32 uiVertCount = 0;
      ezUInt32 uiTriCount = 0;
      if (pTerrain->ReadbackVoxelData(uiVoxelIdx, cpuVerts, cpuIdxs, uiVertCount, uiTriCount).Failed())
      {
        ezLog::Warning("TerrainVoxelExportModifier: ReadbackVoxelData failed for volume (stableId={}), skipping baked collider.", pVolume->GetStableId());
        continue;
      }

      if (uiVertCount == 0 || uiTriCount == 0)
        continue;

      ezJoltMeshDesc meshDesc;
      meshDesc.m_uiContentHash = uiContentHash;
      meshDesc.m_Type = ezJoltMeshDesc::Type::Triangle;

      meshDesc.m_Vertices.SetCountUninitialized(uiVertCount);
      for (ezUInt32 i = 0; i < uiVertCount; ++i)
      {
        meshDesc.m_Vertices[i] = cpuVerts[i].Position;
      }

      const ezUInt32 uiIdxCount = uiTriCount * 3;
      meshDesc.m_TriangleIndices.SetCountUninitialized(uiIdxCount);
      ezMemoryUtils::Copy(meshDesc.m_TriangleIndices.GetData(), cpuIdxs.GetData(), uiIdxCount);

      const ezUInt32 uiNumSurfaces = pVolume->Surfaces_GetCount();
      if (uiNumSurfaces > 0)
      {
        meshDesc.m_Surfaces.SetCount(uiNumSurfaces);
        for (ezUInt32 s = 0; s < uiNumSurfaces; ++s)
        {
          meshDesc.m_Surfaces[s] = pVolume->Surfaces_GetValue(s);
        }

        const ezUInt8 uiFallback = pVolume->GetBaseMaterialIndex();
        meshDesc.m_TriangleSurfaceID.SetCountUninitialized(uiTriCount);

        for (ezUInt32 tri = 0; tri < uiTriCount; ++tri)
        {
          ezUInt8 votes[3];
          for (ezUInt32 v = 0; v < 3; ++v)
          {
            const ezUInt32 idx = cpuIdxs[tri * 3 + v];
            const VoxelGpuVertex& vert = cpuVerts[idx];
            const ezUInt8 raw = (vert.Material != 0xFFFFFFFFu) ? static_cast<ezUInt8>(vert.Material) : 0;
            votes[v] = (vert.MaterialStrength > 0.5f && raw < uiNumSurfaces) ? raw : uiFallback;
          }
          ezUInt8 chosen = votes[0];
          if (votes[1] == votes[2] && votes[1] != votes[0])
          {
            chosen = votes[1];
          }

          meshDesc.m_TriangleSurfaceID[tri] = static_cast<ezUInt16>(chosen);
        }
      }

      ezDeferredFileWriter fileWriter;
      fileWriter.SetOutput(sPath);

      if (ezJoltMeshResourceWriter::WriteMeshResource(meshDesc, fileWriter, true, 0).Failed())
      {
        ezLog::Error("TerrainVoxelExportModifier: failed to cook mesh for '{}'.", sPath);
        continue;
      }

      if (fileWriter.Close().Failed())
      {
        ezLog::Error("TerrainVoxelExportModifier: failed to write file '{}'.", sPath);
        continue;
      }
    }

    pVolume->SetEnableCollider(false);

    // Create or reuse the "VoxelCollider" child game object.
    ezGameObject* pColliderObject = nullptr;
    ezGameObject* pVolumeOwner = pVolume->GetOwner();
    for (auto childIt = pVolumeOwner->GetChildren(); childIt.IsValid(); ++childIt)
    {
      if (childIt->GetNameHashed() == ezTempHashedString("VoxelCollider"))
      {
        pColliderObject = &(*childIt);
        break;
      }
    }

    if (pColliderObject == nullptr)
    {
      ezGameObjectDesc objDesc;
      objDesc.m_sName.Assign("VoxelCollider");
      objDesc.m_hParent = pVolumeOwner->GetHandle();
      ref_world.CreateObject(objDesc, pColliderObject);
    }

    ezJoltStaticActorComponent* pActor = nullptr;
    if (!pColliderObject->TryGetComponentOfBaseType(pActor))
    {
      pActorMan->CreateComponent(pColliderObject, pActor);
    }

    pActor->SetMesh(ezResourceManager::LoadResource<ezJoltMeshResource>(sPath));
  }
}

EZ_STATICLINK_FILE(EnginePluginTerrain, EnginePluginTerrain_SceneExport_TerrainVoxelExportModifier);
