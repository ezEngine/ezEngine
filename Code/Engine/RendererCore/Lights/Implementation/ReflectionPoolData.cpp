#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/Implementation/ReflectionPoolData.h>

#include <Core/Graphics/Geometry.h>
#include <RendererCore/Lights/BoxReflectionProbeComponent.h>
#include <RendererCore/Lights/SkyLightComponent.h>
#include <RendererCore/Lights/SphereReflectionProbeComponent.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>

//////////////////////////////////////////////////////////////////////////
/// ezReflectionPool::Data

ezReflectionPool::Data* ezReflectionPool::s_pData;

ezReflectionPool::Data::Data()
{
  m_SkyIrradianceStorage.SetCount(64);
}

ezReflectionPool::Data::~Data()
{
  if (!m_hFallbackReflectionSpecularTexture.IsInvalidated())
  {
    ezGALDevice::GetDefaultDevice()->DestroyTexture(m_hFallbackReflectionSpecularTexture);
    m_hFallbackReflectionSpecularTexture.Invalidate();
  }

  ezUInt32 uiWorldReflectionCount = m_WorldReflectionData.GetCount();
  for (ezUInt32 i = 0; i < uiWorldReflectionCount; ++i)
  {
    WorldReflectionData* pData = m_WorldReflectionData[i].Borrow();
    EZ_ASSERT_DEV(!pData || pData->m_Probes.IsEmpty(), "Not all probes were deregistered.");
  }
  m_WorldReflectionData.Clear();

  if (!m_hSkyIrradianceTexture.IsInvalidated())
  {
    ezGALDevice::GetDefaultDevice()->DestroyTexture(m_hSkyIrradianceTexture);
    m_hSkyIrradianceTexture.Invalidate();
  }
}

ezReflectionProbeId ezReflectionPool::Data::AddProbe(const ezWorld* pWorld, ProbeData&& probeData)
{
  const ezUInt32 uiWorldIndex = pWorld->GetIndex();

  if (uiWorldIndex >= s_pData->m_WorldReflectionData.GetCount())
    s_pData->m_WorldReflectionData.SetCount(uiWorldIndex + 1);

  if (s_pData->m_WorldReflectionData[uiWorldIndex] == nullptr)
  {
    s_pData->m_WorldReflectionData[uiWorldIndex] = EZ_DEFAULT_NEW(WorldReflectionData);
    s_pData->m_WorldReflectionData[uiWorldIndex]->m_mappingSubscriptionId = s_pData->m_WorldReflectionData[uiWorldIndex]->m_mapping.m_Events.AddEventHandler([uiWorldIndex, this](const ezReflectionProbeMappingEvent& e)
      { OnReflectionProbeMappingEvent(uiWorldIndex, e); });
  }

  ezReflectionPool::Data::WorldReflectionData& data = *s_pData->m_WorldReflectionData[uiWorldIndex];

  const ezBitflags<ezProbeFlags> flags = probeData.m_Flags;
  ezReflectionProbeId id = data.m_Probes.Insert(std::move(probeData));

  if (probeData.m_Flags.IsSet(ezProbeFlags::SkyLight))
  {
    data.m_SkyLight = id;
  }
  data.m_mapping.AddProbe(id, flags);

  return id;
}

ezReflectionPool::Data::WorldReflectionData& ezReflectionPool::Data::GetWorldData(const ezWorld* pWorld)
{
  const ezUInt32 uiWorldIndex = pWorld->GetIndex();
  return *s_pData->m_WorldReflectionData[uiWorldIndex];
}

void ezReflectionPool::Data::RemoveProbe(const ezWorld* pWorld, ezReflectionProbeId id)
{
  const ezUInt32 uiWorldIndex = pWorld->GetIndex();
  ezReflectionPool::Data::WorldReflectionData& data = *s_pData->m_WorldReflectionData[uiWorldIndex];

  data.m_mapping.RemoveProbe(id);

  if (data.m_SkyLight == id)
  {
    data.m_SkyLight.Invalidate();
  }

  data.m_Probes.Remove(id);

  if (data.m_Probes.IsEmpty())
  {
    s_pData->m_WorldReflectionData[uiWorldIndex]->m_mapping.m_Events.RemoveEventHandler(s_pData->m_WorldReflectionData[uiWorldIndex]->m_mappingSubscriptionId);
    s_pData->m_WorldReflectionData[uiWorldIndex].Clear();
  }
}

void ezReflectionPool::Data::UpdateProbeData(ProbeData& ref_probeData, const ezReflectionProbeDesc& desc, const ezReflectionProbeComponentBase* pComponent)
{
  ref_probeData.m_desc = desc;
  ref_probeData.m_GlobalTransform = pComponent->GetOwner()->GetGlobalTransform();

  if (const ezSphereReflectionProbeComponent* pSphere = ezDynamicCast<const ezSphereReflectionProbeComponent*>(pComponent))
  {
    ref_probeData.m_Flags = ezProbeFlags::Sphere;
  }
  else if (const ezBoxReflectionProbeComponent* pBox = ezDynamicCast<const ezBoxReflectionProbeComponent*>(pComponent))
  {
    ref_probeData.m_Flags = ezProbeFlags::Box;
  }

  if (ref_probeData.m_desc.m_Mode == ezReflectionProbeMode::Dynamic)
  {
    ref_probeData.m_Flags |= ezProbeFlags::Dynamic;
  }
  else
  {
    ezStringBuilder sComponentGuid, sCubeMapFile;
    ezConversionUtils::ToString(ref_probeData.m_desc.m_uniqueID, sComponentGuid);

    // this is where the editor will put the file for this probe
    sCubeMapFile.SetFormat(":project/AssetCache/Generated/{0}.ezTexture", sComponentGuid);

    ref_probeData.m_hCubeMap = ezResourceManager::LoadResource<ezTextureCubeResource>(sCubeMapFile);
  }
}

bool ezReflectionPool::Data::UpdateSkyLightData(ProbeData& ref_probeData, const ezReflectionProbeDesc& desc, const ezSkyLightComponent* pComponent)
{
  bool bProbeTypeChanged = false;
  if (ref_probeData.m_desc.m_Mode != desc.m_Mode)
  {
    // #TODO any other reason to unmap a probe.
    bProbeTypeChanged = true;
  }

  ref_probeData.m_desc = desc;
  ref_probeData.m_GlobalTransform = pComponent->GetOwner()->GetGlobalTransform();

  if (auto pSkyLight = ezDynamicCast<const ezSkyLightComponent*>(pComponent))
  {
    ref_probeData.m_Flags = ezProbeFlags::SkyLight;
    ref_probeData.m_hCubeMap = pSkyLight->GetCubeMap();
    if (ref_probeData.m_desc.m_Mode == ezReflectionProbeMode::Dynamic)
    {
      ref_probeData.m_Flags |= ezProbeFlags::Dynamic;
    }
    else
    {
      if (ref_probeData.m_hCubeMap.IsValid())
      {
        ref_probeData.m_Flags |= ezProbeFlags::HasCustomCubeMap;
      }
      else
      {
        ezStringBuilder sComponentGuid, sCubeMapFile;
        ezConversionUtils::ToString(ref_probeData.m_desc.m_uniqueID, sComponentGuid);

        // this is where the editor will put the file for this probe
        sCubeMapFile.SetFormat(":project/AssetCache/Generated/{0}.ezTexture", sComponentGuid);

        ref_probeData.m_hCubeMap = ezResourceManager::LoadResource<ezTextureCubeResource>(sCubeMapFile);
      }
    }
  }
  return bProbeTypeChanged;
}

void ezReflectionPool::Data::OnReflectionProbeMappingEvent(const ezUInt32 uiWorldIndex, const ezReflectionProbeMappingEvent& e)
{
  switch (e.m_Type)
  {
    case ezReflectionProbeMappingEvent::Type::ProbeMapped:
      break;
    case ezReflectionProbeMappingEvent::Type::ProbeUnmapped:
    {
      ezReflectionProbeRef probeUpdate = {uiWorldIndex, e.m_Id};
      if (m_PendingDynamicUpdate.Contains(probeUpdate))
      {
        m_PendingDynamicUpdate.Remove(probeUpdate);
        m_DynamicUpdateQueue.RemoveAndCopy(probeUpdate);
      }

      if (m_ActiveDynamicUpdate.Contains(probeUpdate))
      {
        m_ActiveDynamicUpdate.Remove(probeUpdate);
        m_ReflectionProbeUpdater.CancelUpdate(probeUpdate);
      }
    }
    break;
    case ezReflectionProbeMappingEvent::Type::ProbeUpdateRequested:
    {
      // For now, we just manage a FIFO queue of all dynamic probes that have a high enough priority.
      const ezReflectionProbeRef du = {uiWorldIndex, e.m_Id};
      ezReflectionPool::Data::WorldReflectionData& data = *s_pData->m_WorldReflectionData[uiWorldIndex];
      if (!m_PendingDynamicUpdate.Contains(du))
      {
        m_PendingDynamicUpdate.Insert(du);
        m_DynamicUpdateQueue.PushBack(du);
      }
    }
    break;
  }
}

//////////////////////////////////////////////////////////////////////////
/// Dynamic Update

void ezReflectionPool::Data::PreExtraction()
{
  EZ_LOCK(s_pData->m_Mutex);
  const ezUInt32 uiWorldCount = s_pData->m_WorldReflectionData.GetCount();

  for (ezUInt32 uiWorld = 0; uiWorld < uiWorldCount; uiWorld++)
  {
    if (s_pData->m_WorldReflectionData[uiWorld] == nullptr)
      continue;

    ezReflectionPool::Data::WorldReflectionData& data = *s_pData->m_WorldReflectionData[uiWorld];
    data.m_mapping.PreExtraction();
  }


  // Schedule new dynamic updates
  {
    ezHybridArray<ezReflectionProbeRef, 4> updatesFinished;
    const ezUInt32 uiCount = ezMath::Min(m_ReflectionProbeUpdater.GetFreeUpdateSlots(updatesFinished), m_DynamicUpdateQueue.GetCount());
    for (const ezReflectionProbeRef& probe : updatesFinished)
    {
      m_ActiveDynamicUpdate.Remove(probe);

      if (s_pData->m_WorldReflectionData[probe.m_uiWorldIndex] == nullptr)
        continue;

      ezReflectionPool::Data::WorldReflectionData& data = *s_pData->m_WorldReflectionData[probe.m_uiWorldIndex];
      data.m_mapping.ProbeUpdateFinished(probe.m_Id);
    }

    for (ezUInt32 i = 0; i < uiCount; i++)
    {
      ezReflectionProbeRef nextUpdate = m_DynamicUpdateQueue.PeekFront();
      m_DynamicUpdateQueue.PopFront();
      m_PendingDynamicUpdate.Remove(nextUpdate);

      if (s_pData->m_WorldReflectionData[nextUpdate.m_uiWorldIndex] == nullptr)
        continue;

      ezReflectionPool::Data::WorldReflectionData& data = *s_pData->m_WorldReflectionData[nextUpdate.m_uiWorldIndex];
      ProbeData& probeData = data.m_Probes.GetValueUnchecked(nextUpdate.m_Id.m_InstanceIndex);

      ezReflectionProbeUpdater::TargetSlot target;
      target.m_hSpecularOutputTexture = data.m_mapping.GetTexture();
      target.m_iSpecularOutputIndex = data.m_mapping.GetReflectionIndex(nextUpdate.m_Id);

      if (probeData.m_Flags.IsSet(ezProbeFlags::SkyLight))
      {
        target.m_hIrradianceOutputTexture = m_hSkyIrradianceTexture;
        target.m_iIrradianceOutputIndex = nextUpdate.m_uiWorldIndex;
      }

      if (probeData.m_Flags.IsSet(ezProbeFlags::HasCustomCubeMap))
      {
        EZ_ASSERT_DEBUG(probeData.m_hCubeMap.IsValid(), "");
        EZ_VERIFY(m_ReflectionProbeUpdater.StartFilterUpdate(nextUpdate, probeData.m_desc, probeData.m_hCubeMap, target).Succeeded(), "GetFreeUpdateSlots returned incorrect result");
      }
      else
      {
        EZ_VERIFY(m_ReflectionProbeUpdater.StartDynamicUpdate(nextUpdate, probeData.m_desc, probeData.m_GlobalTransform, target).Succeeded(), "GetFreeUpdateSlots returned incorrect result");
      }
      m_ActiveDynamicUpdate.Insert(nextUpdate);
    }
    m_ReflectionProbeUpdater.GenerateUpdateSteps();
  }
}

void ezReflectionPool::Data::PostExtraction()
{
  EZ_LOCK(s_pData->m_Mutex);
  const ezUInt32 uiWorldCount = s_pData->m_WorldReflectionData.GetCount();
  for (ezUInt32 uiWorld = 0; uiWorld < uiWorldCount; uiWorld++)
  {
    if (s_pData->m_WorldReflectionData[uiWorld] == nullptr)
      continue;
    ezReflectionPool::Data::WorldReflectionData& data = *s_pData->m_WorldReflectionData[uiWorld];
    data.m_mapping.PostExtraction();
  }
}

//////////////////////////////////////////////////////////////////////////
/// Resource Creation

void ezReflectionPool::Data::CreateReflectionViewsAndResources()
{
  if (m_hFallbackReflectionSpecularTexture.IsInvalidated())
  {
    ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

    ezGALTextureCreationDescription desc;
    desc.m_uiWidth = s_uiReflectionCubeMapSize;
    desc.m_uiHeight = s_uiReflectionCubeMapSize;
    desc.m_uiMipLevelCount = GetMipLevels();
    desc.m_uiArraySize = 1;
    desc.m_Format = ezGALResourceFormat::RGBAHalf;
    desc.m_Type = ezGALTextureType::TextureCubeArray;
    desc.m_bAllowRenderTargetView = false;
    desc.m_bAllowUAV = true;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hFallbackReflectionSpecularTexture = pDevice->CreateTexture(desc);
    if (!m_hFallbackReflectionSpecularTexture.IsInvalidated())
    {
      pDevice->GetTexture(m_hFallbackReflectionSpecularTexture)->SetDebugName("Reflection Fallback Specular Texture");
    }
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  if (!m_hDebugSphere.IsValid())
  {
    ezGeometry geom;
    geom.AddStackedSphere(s_fDebugSphereRadius, 32, 16);

    const char* szBufferResourceName = "ReflectionProbeDebugSphereBuffer";
    ezMeshBufferResourceHandle hMeshBuffer = ezResourceManager::GetExistingResource<ezMeshBufferResource>(szBufferResourceName);
    if (!hMeshBuffer.IsValid())
    {
      ezMeshBufferResourceDescriptor desc;
      desc.AddCommonStreams();
      desc.AllocateStreamsFromGeometry(geom, ezGALPrimitiveTopology::Triangles);

      hMeshBuffer = ezResourceManager::GetOrCreateResource<ezMeshBufferResource>(szBufferResourceName, std::move(desc), szBufferResourceName);
    }

    const char* szMeshResourceName = "ReflectionProbeDebugSphere";
    m_hDebugSphere = ezResourceManager::GetExistingResource<ezMeshResource>(szMeshResourceName);
    if (!m_hDebugSphere.IsValid())
    {
      ezMeshResourceDescriptor desc;
      desc.UseExistingMeshBuffer(hMeshBuffer);
      desc.AddSubMesh(geom.CalculateTriangleCount(), 0, 0);
      desc.ComputeBounds();

      m_hDebugSphere = ezResourceManager::GetOrCreateResource<ezMeshResource>(szMeshResourceName, std::move(desc), szMeshResourceName);
    }
  }

  if (m_hDebugMaterial.IsEmpty())
  {
    const ezUInt32 uiMipLevelCount = GetMipLevels();

    ezMaterialResourceHandle hDebugMaterial = ezResourceManager::LoadResource<ezMaterialResource>(
      "{ 6f8067d0-ece8-44e1-af46-79b49266de41 }"); // ReflectionProbeVisualization.ezMaterialAsset
    ezResourceLock<ezMaterialResource> pMaterial(hDebugMaterial, ezResourceAcquireMode::BlockTillLoaded);
    if (pMaterial->GetLoadingState() != ezResourceState::Loaded)
      return;

    ezMaterialResourceDescriptor desc = pMaterial->GetCurrentDesc();
    ezUInt32 uiMipLevel = desc.m_Parameters.GetCount();
    ezUInt32 uiReflectionProbeIndex = desc.m_Parameters.GetCount();
    ezTempHashedString sMipLevelParam = "MipLevel";
    ezTempHashedString sReflectionProbeIndexParam = "ReflectionProbeIndex";
    for (ezUInt32 i = 0; i < desc.m_Parameters.GetCount(); ++i)
    {
      if (desc.m_Parameters[i].m_Name == sMipLevelParam)
      {
        uiMipLevel = i;
      }
      if (desc.m_Parameters[i].m_Name == sReflectionProbeIndexParam)
      {
        uiReflectionProbeIndex = i;
      }
    }

    if (uiMipLevel >= desc.m_Parameters.GetCount() || uiReflectionProbeIndex >= desc.m_Parameters.GetCount())
      return;

    m_hDebugMaterial.SetCount(uiMipLevelCount * s_uiNumReflectionProbeCubeMaps);
    for (ezUInt32 iReflectionProbeIndex = 0; iReflectionProbeIndex < s_uiNumReflectionProbeCubeMaps; iReflectionProbeIndex++)
    {
      for (ezUInt32 iMipLevel = 0; iMipLevel < uiMipLevelCount; iMipLevel++)
      {
        desc.m_Parameters[uiMipLevel].m_Value = iMipLevel;
        desc.m_Parameters[uiReflectionProbeIndex].m_Value = iReflectionProbeIndex;
        ezStringBuilder sMaterialName;
        sMaterialName.SetFormat("ReflectionProbeVisualization - MipLevel {}, Index {}", iMipLevel, iReflectionProbeIndex);

        ezMaterialResourceDescriptor desc2 = desc;
        m_hDebugMaterial[iReflectionProbeIndex * uiMipLevelCount + iMipLevel] = ezResourceManager::GetOrCreateResource<ezMaterialResource>(sMaterialName, std::move(desc2));
      }
    }
  }
#endif
}

void ezReflectionPool::Data::CreateSkyIrradianceTexture()
{
  if (m_hSkyIrradianceTexture.IsInvalidated())
  {
    ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

    ezGALTextureCreationDescription desc;
    desc.m_uiWidth = 6;
    desc.m_uiHeight = 64;
    desc.m_Format = ezGALResourceFormat::RGBAHalf;
    desc.m_Type = ezGALTextureType::Texture2D;
    desc.m_bAllowRenderTargetView = true;
    desc.m_bAllowUAV = true;

    m_hSkyIrradianceTexture = pDevice->CreateTexture(desc);
    pDevice->GetTexture(m_hSkyIrradianceTexture)->SetDebugName("Sky Irradiance Texture");
  }
}
