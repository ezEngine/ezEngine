#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/Implementation/ReflectionPoolData.h>

#include <Core/Graphics/Geometry.h>
#include <RendererCore/Lights/BoxReflectionProbeComponent.h>
#include <RendererCore/Lights/SkyLightComponent.h>
#include <RendererCore/Lights/SphereReflectionProbeComponent.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/RenderGraph/RenderGraph.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>

ezCVarFloat cvar_RenderingReflectionPoolRefreshAgeWeight("Rendering.ReflectionPool.RefreshAgeWeight", 0.005f, ezCVarFlags::Default, "How much a reflection probe's update priority grows for every frame that it waits to be refreshed. Probe priority is roughly radius/distance, so at 0.005 a waiting probe overtakes a much closer one after about two seconds. Set to 0 to sort purely by distance, which can starve distant probes.");

//////////////////////////////////////////////////////////////////////////
/// ezReflectionPool::Data

ezReflectionPool::Data* ezReflectionPool::s_pData;

ezReflectionPool::Data::Data()
{
  m_SkyIrradianceStorage.SetCount(64);
}

ezReflectionPool::Data::~Data()
{
  ezGALDevice::GetDefaultDevice()->DestroyTexture(m_hFallbackReflectionSpecularTexture);

  ezUInt32 uiWorldReflectionCount = m_WorldReflectionData.GetCount();
  for (ezUInt32 i = 0; i < uiWorldReflectionCount; ++i)
  {
    WorldReflectionData* pData = m_WorldReflectionData[i].Borrow();
    EZ_ASSERT_DEV(!pData || pData->m_Probes.IsEmpty(), "Not all probes were deregistered.");
  }
  m_WorldReflectionData.Clear();

  ezGALDevice::GetDefaultDevice()->DestroyTexture(m_hSkyIrradianceTexture);
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
  WorldReflectionData& data = *s_pData->m_WorldReflectionData[uiWorldIndex];

  data.m_mapping.RemoveProbe(id);

  if (data.m_SkyLight == id)
  {
    data.m_SkyLight.Invalidate();
  }

  ProbeData probeData;
  data.m_Probes.Remove(id, &probeData);

  const ezRenderDataManager* pRenderDataManager = pWorld->GetModule<ezRenderDataManager>();
  pRenderDataManager->DeleteInstanceData(probeData.m_DebugInstanceDataOffset);

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

  if (pComponent != nullptr)
  {
    ref_probeData.m_Flags = ezProbeFlags::SkyLight;
    ref_probeData.m_hCubeMap = pComponent->GetCubeMap();
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

        // The probe may be in either queue.
        RemoveFromQueue(m_InitialBakeQueue, probeUpdate);
        RemoveFromQueue(m_RefreshQueue, probeUpdate);
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
      const ezReflectionProbeRef du = {uiWorldIndex, e.m_Id};

      ezDynamicArray<QueuedUpdate>& queue = e.m_bFirstBake ? m_InitialBakeQueue : m_RefreshQueue;

      bool bFound = false;
      for (QueuedUpdate& update : queue)
      {
        if (update.m_probe == du)
        {
          // Already pending: keep the highest priority seen rather than dropping the request. The frame it
          // was enqueued in is kept, so that waiting still ages the probe.
          update.m_fPriority = ezMath::Max(update.m_fPriority, e.m_fPriority);
          bFound = true;
          break;
        }
      }

      if (!bFound)
      {
        // A probe can change queues: one that lost its content must not stay queued for a mere refresh, and
        // one that just gained content moves the other way.
        RemoveFromQueue(e.m_bFirstBake ? m_RefreshQueue : m_InitialBakeQueue, du);

        m_PendingDynamicUpdate.Insert(du);
        queue.PushBack({du, e.m_fPriority, ezRenderWorld::GetFrameCounter()});
      }
    }
    break;
  }
}

//////////////////////////////////////////////////////////////////////////
/// Dynamic Update

void ezReflectionPool::Data::RemoveFromQueue(ezDynamicArray<QueuedUpdate>& ref_queue, const ezReflectionProbeRef& probe)
{
  for (ezUInt32 i = ref_queue.GetCount(); i-- > 0;)
  {
    if (ref_queue[i].m_probe == probe)
    {
      ref_queue.RemoveAtAndCopy(i);
    }
  }
}

void ezReflectionPool::Data::SortQueue(ezDynamicArray<QueuedUpdate>& ref_queue, float fAgeWeight)
{
  const ezUInt64 uiCurrentFrame = ezRenderWorld::GetFrameCounter();

  ref_queue.Sort([=](const QueuedUpdate& a, const QueuedUpdate& b)
    {
      const float fA = a.m_fPriority + fAgeWeight * (float)(uiCurrentFrame - a.m_uiEnqueuedFrame);
      const float fB = b.m_fPriority + fAgeWeight * (float)(uiCurrentFrame - b.m_uiEnqueuedFrame);
      return fA > fB; });
}

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
    ezTempHybridArray<ezReflectionProbeRef, 4> updatesFinished;
    ezUInt32 uiFreeSlots = m_ReflectionProbeUpdater.GetFreeUpdateSlots(updatesFinished);
    for (const ezReflectionProbeRef& probe : updatesFinished)
    {
      m_ActiveDynamicUpdate.Remove(probe);

      if (s_pData->m_WorldReflectionData[probe.m_uiWorldIndex] == nullptr)
        continue;

      ezReflectionPool::Data::WorldReflectionData& data = *s_pData->m_WorldReflectionData[probe.m_uiWorldIndex];
      data.m_mapping.ProbeUpdateFinished(probe.m_Id);
    }

    // Probes without any content are updated before refreshes of probes that already look correct, and among
    // themselves in priority order, so that the most visible parts of the scene resolve first. They are only
    // in this queue until they have been rendered once, so they need no aging to avoid starvation.
    SortQueue(m_InitialBakeQueue, 0.0f);

    // Refreshes are also ordered by priority, so that the probes around the camera are corrected first after
    // a sky light update dirtied all of them. Here waiting does raise a probe's priority, as probes return to
    // this queue over and over and a distant one would otherwise never get its turn.
    SortQueue(m_RefreshQueue, cvar_RenderingReflectionPoolRefreshAgeWeight);

    ezUInt32 uiInitialBakeIndex = 0;
    ezUInt32 uiRefreshIndex = 0;
    while (uiFreeSlots > 0 && (uiInitialBakeIndex < m_InitialBakeQueue.GetCount() || uiRefreshIndex < m_RefreshQueue.GetCount()))
    {
      const bool bFirstBake = uiInitialBakeIndex < m_InitialBakeQueue.GetCount();

      ezReflectionProbeRef nextUpdate;
      if (bFirstBake)
      {
        nextUpdate = m_InitialBakeQueue[uiInitialBakeIndex].m_probe;
        ++uiInitialBakeIndex;
      }
      else
      {
        nextUpdate = m_RefreshQueue[uiRefreshIndex].m_probe;
        ++uiRefreshIndex;
      }

      m_PendingDynamicUpdate.Remove(nextUpdate);
      --uiFreeSlots;

      if (s_pData->m_WorldReflectionData[nextUpdate.m_uiWorldIndex] == nullptr)
        continue;

      ezReflectionPool::Data::WorldReflectionData& data = *s_pData->m_WorldReflectionData[nextUpdate.m_uiWorldIndex];
      ProbeData& probeData = data.m_Probes.GetValueUnchecked(nextUpdate.m_Id.m_InstanceIndex);

      ezReflectionProbeUpdater::TargetSlot target;
      target.m_hSpecularOutputTexture = data.m_mapping.GetTexture();
      target.m_iSpecularOutputIndex = data.m_mapping.GetReflectionIndex(nextUpdate.m_Id);

      const bool bSkyLight = probeData.m_Flags.IsSet(ezProbeFlags::SkyLight);
      if (bSkyLight)
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
        // The per frame budget is split when both kinds of work are present: probes without any content
        // get the larger share, refreshes a single face. Whichever kind is alone gets the whole budget.
        // A refresh also has to consider first bakes that are already running in the other update slot.
        const bool bSharingBudget = bFirstBake ? !m_RefreshQueue.IsEmpty() : (uiInitialBakeIndex < m_InitialBakeQueue.GetCount() || m_ReflectionProbeUpdater.IsFirstBakeInProgress());

        EZ_VERIFY(m_ReflectionProbeUpdater.StartDynamicUpdate(nextUpdate, probeData.m_desc, probeData.m_GlobalTransform, target, bFirstBake, bSkyLight, bSharingBudget).Succeeded(), "GetFreeUpdateSlots returned incorrect result");
      }
      m_ActiveDynamicUpdate.Insert(nextUpdate);
    }

    // Both queues are sorted, so the entries that were started this frame are the ones at the front.
    m_InitialBakeQueue.RemoveAtAndCopy(0, uiInitialBakeIndex);
    m_RefreshQueue.RemoveAtAndCopy(0, uiRefreshIndex);

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
    desc.m_TextureFlags = ezGALTextureUsageFlags::ShaderResource | ezGALTextureUsageFlags::UnorderedAccess;
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

  if (!m_hDebugMaterial.IsValid())
  {
    m_hDebugMaterial = ezResourceManager::LoadResource<ezMaterialResource>("{ 6f8067d0-ece8-44e1-af46-79b49266de41 }"); // ReflectionProbeVisualization.ezMaterialAsset
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
    desc.m_TextureFlags.Add(ezGALTextureUsageFlags::RenderTarget | ezGALTextureUsageFlags::UnorderedAccess);
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hSkyIrradianceTexture = pDevice->CreateTexture(desc);
    pDevice->GetTexture(m_hSkyIrradianceTexture)->SetDebugName("Sky Irradiance Texture");
  }
}
