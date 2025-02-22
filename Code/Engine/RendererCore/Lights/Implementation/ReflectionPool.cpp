#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/Graphics/Geometry.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Lights/Implementation/ReflectionPoolData.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, ReflectionPool)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core",
    "RenderWorld"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    ezReflectionPool::OnEngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    ezReflectionPool::OnEngineShutdown();
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

//////////////////////////////////////////////////////////////////////////
/// ezReflectionPool

ezReflectionProbeId ezReflectionPool::RegisterReflectionProbe(const ezWorld* pWorld, const ezReflectionProbeDesc& desc, const ezReflectionProbeComponentBase* pComponent)
{
  EZ_LOCK(s_pData->m_Mutex);

  Data::ProbeData probe;
  s_pData->UpdateProbeData(probe, desc, pComponent);
  return s_pData->AddProbe(pWorld, std::move(probe));
}

void ezReflectionPool::DeregisterReflectionProbe(const ezWorld* pWorld, ezReflectionProbeId id)
{
  EZ_LOCK(s_pData->m_Mutex);
  s_pData->RemoveProbe(pWorld, id);
}

void ezReflectionPool::UpdateReflectionProbe(const ezWorld* pWorld, ezReflectionProbeId id, const ezReflectionProbeDesc& desc, const ezReflectionProbeComponentBase* pComponent)
{
  EZ_LOCK(s_pData->m_Mutex);
  ezReflectionPool::Data::WorldReflectionData& data = s_pData->GetWorldData(pWorld);
  Data::ProbeData& probeData = data.m_Probes.GetValueUnchecked(id.m_InstanceIndex);
  s_pData->UpdateProbeData(probeData, desc, pComponent);
  data.m_mapping.UpdateProbe(id, probeData.m_Flags);
}

void ezReflectionPool::ExtractReflectionProbe(const ezComponent* pComponent, ezMsgExtractRenderData& ref_msg, ezReflectionProbeRenderData* pRenderData0, const ezWorld* pWorld, ezReflectionProbeId id, float fPriority)
{
  EZ_LOCK(s_pData->m_Mutex);
  s_pData->m_ReflectionProbeUpdater.ScheduleUpdateSteps();

  const ezUInt32 uiWorldIndex = pWorld->GetIndex();
  ezReflectionPool::Data::WorldReflectionData& data = *s_pData->m_WorldReflectionData[uiWorldIndex];
  data.m_mapping.AddWeight(id, fPriority);
  const ezInt32 iMappedIndex = data.m_mapping.GetReflectionIndex(id, true);

  Data::ProbeData& probeData = data.m_Probes.GetValueUnchecked(id.m_InstanceIndex);

  if (pComponent->GetOwner()->IsDynamic())
  {
    ezTransform globalTransform = pComponent->GetOwner()->GetGlobalTransform();
    if (!probeData.m_Flags.IsSet(ezProbeFlags::Dynamic) && probeData.m_GlobalTransform != globalTransform)
    {
      data.m_mapping.UpdateProbe(id, probeData.m_Flags);
    }
    probeData.m_GlobalTransform = globalTransform;
  }

  // The sky light is always active and not added to the render data (always passes in nullptr as pRenderData).
  if (pRenderData0 && iMappedIndex > 0)
  {
    // Index and flags are stored in m_uiIndex so we can't just overwrite it.
    pRenderData0->m_uiIndex |= (ezUInt32)iMappedIndex;
    ref_msg.AddRenderData(pRenderData0, ezDefaultRenderDataCategories::ReflectionProbe, ezRenderData::Caching::Never);
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  const ezUInt32 uiMipLevels = GetMipLevels();
  if (probeData.m_desc.m_bShowDebugInfo && s_pData->m_hDebugMaterial.GetCount() == uiMipLevels * s_uiNumReflectionProbeCubeMaps)
  {
    if (ref_msg.m_OverrideCategory == ezInvalidRenderDataCategory)
    {
      ezInt32 activeIndex = 0;
      if (s_pData->m_ActiveDynamicUpdate.Contains(ezReflectionProbeRef{uiWorldIndex, id}))
      {
        activeIndex = 1;
      }

      ezStringBuilder sEnum;
      ezReflectionUtils::BitflagsToString(probeData.m_Flags, sEnum, ezReflectionUtils::EnumConversionMode::ValueNameOnly);
      ezStringBuilder s;
      s.SetFormat("\n RefIdx: {}\nUpdating: {}\nFlags: {}\n", iMappedIndex, activeIndex, sEnum);
      ezDebugRenderer::Draw3DText(pWorld, s, pComponent->GetOwner()->GetGlobalPosition(), ezColorScheme::LightUI(ezColorScheme::Violet));
    }

    // Not mapped in the atlas - cannot render it.
    if (iMappedIndex < 0)
      return;

    const ezGameObject* pOwner = pComponent->GetOwner();
    const ezTransform ownerTransform = pOwner->GetGlobalTransform();

    ezUInt32 uiMipLevelsToRender = probeData.m_desc.m_bShowMipMaps ? uiMipLevels : 1;
    for (ezUInt32 i = 0; i < uiMipLevelsToRender; i++)
    {
      ezMeshRenderData* pRenderData = ezCreateRenderDataForThisFrame<ezMeshRenderData>(pOwner);
      pRenderData->m_GlobalTransform.m_vPosition = ownerTransform * probeData.m_desc.m_vCaptureOffset;
      pRenderData->m_GlobalTransform.m_vScale = ezVec3(1.0f);
      if (!probeData.m_Flags.IsSet(ezProbeFlags::SkyLight))
      {
        pRenderData->m_GlobalTransform.m_qRotation = ownerTransform.m_qRotation;
      }
      pRenderData->m_GlobalTransform.m_vPosition.z += s_fDebugSphereRadius * i * 2;
      pRenderData->m_GlobalBounds = pOwner->GetGlobalBounds();
      pRenderData->m_hMesh = s_pData->m_hDebugSphere;
      pRenderData->m_hMaterial = s_pData->m_hDebugMaterial[iMappedIndex * uiMipLevels + i];
      pRenderData->m_Color = ezColor::White;
      pRenderData->m_uiSubMeshIndex = 0;
      pRenderData->m_uiUniqueID = ezRenderComponent::GetUniqueIdForRendering(*pComponent, 0);

      pRenderData->FillSortingKey();
      ref_msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::LitOpaque, ezRenderData::Caching::Never);
    }
  }
#endif
}

//////////////////////////////////////////////////////////////////////////
/// SkyLight

ezReflectionProbeId ezReflectionPool::RegisterSkyLight(const ezWorld* pWorld, ezReflectionProbeDesc& ref_desc, const ezSkyLightComponent* pComponent)
{
  EZ_LOCK(s_pData->m_Mutex);
  const ezUInt32 uiWorldIndex = pWorld->GetIndex();
  s_pData->m_uiWorldHasSkyLight |= EZ_BIT(uiWorldIndex);
  s_pData->m_uiSkyIrradianceChanged |= EZ_BIT(uiWorldIndex);

  Data::ProbeData probe;
  s_pData->UpdateSkyLightData(probe, ref_desc, pComponent);

  ezReflectionProbeId id = s_pData->AddProbe(pWorld, std::move(probe));
  return id;
}

void ezReflectionPool::DeregisterSkyLight(const ezWorld* pWorld, ezReflectionProbeId id)
{
  EZ_LOCK(s_pData->m_Mutex);

  s_pData->RemoveProbe(pWorld, id);

  const ezUInt32 uiWorldIndex = pWorld->GetIndex();
  s_pData->m_uiWorldHasSkyLight &= ~EZ_BIT(uiWorldIndex);
  s_pData->m_uiSkyIrradianceChanged |= EZ_BIT(uiWorldIndex);
}

void ezReflectionPool::UpdateSkyLight(const ezWorld* pWorld, ezReflectionProbeId id, const ezReflectionProbeDesc& desc, const ezSkyLightComponent* pComponent)
{
  EZ_LOCK(s_pData->m_Mutex);
  ezReflectionPool::Data::WorldReflectionData& data = s_pData->GetWorldData(pWorld);
  Data::ProbeData& probeData = data.m_Probes.GetValueUnchecked(id.m_InstanceIndex);
  if (s_pData->UpdateSkyLightData(probeData, desc, pComponent))
  {
    // s_pData->UnmapProbe(pWorld->GetIndex(), data, id);
  }
  data.m_mapping.UpdateProbe(id, probeData.m_Flags);
}

//////////////////////////////////////////////////////////////////////////
/// Misc

// static
void ezReflectionPool::SetConstantSkyIrradiance(const ezWorld* pWorld, const ezAmbientCube<ezColor>& skyIrradiance)
{
  EZ_LOCK(s_pData->m_Mutex);
  ezUInt32 uiWorldIndex = pWorld->GetIndex();
  ezAmbientCube<ezColorLinear16f> skyIrradiance16f = skyIrradiance;

  auto& skyIrradianceStorage = s_pData->m_SkyIrradianceStorage;
  if (skyIrradianceStorage[uiWorldIndex] != skyIrradiance16f)
  {
    skyIrradianceStorage[uiWorldIndex] = skyIrradiance16f;

    s_pData->m_uiSkyIrradianceChanged |= EZ_BIT(uiWorldIndex);
  }
}

void ezReflectionPool::ResetConstantSkyIrradiance(const ezWorld* pWorld)
{
  EZ_LOCK(s_pData->m_Mutex);
  ezUInt32 uiWorldIndex = pWorld->GetIndex();

  auto& skyIrradianceStorage = s_pData->m_SkyIrradianceStorage;
  if (skyIrradianceStorage[uiWorldIndex] != ezAmbientCube<ezColorLinear16f>())
  {
    skyIrradianceStorage[uiWorldIndex] = ezAmbientCube<ezColorLinear16f>();

    s_pData->m_uiSkyIrradianceChanged |= EZ_BIT(uiWorldIndex);
  }
}

// static
ezUInt32 ezReflectionPool::GetReflectionCubeMapSize()
{
  return s_uiReflectionCubeMapSize;
}

// static
ezGALTextureHandle ezReflectionPool::GetReflectionSpecularTexture(ezUInt32 uiWorldIndex, ezEnum<ezCameraUsageHint> cameraUsageHint)
{
  if (uiWorldIndex < s_pData->m_WorldReflectionData.GetCount() && cameraUsageHint != ezCameraUsageHint::Reflection)
  {
    Data::WorldReflectionData* pData = s_pData->m_WorldReflectionData[uiWorldIndex].Borrow();
    if (pData)
      return pData->m_mapping.GetTexture();
  }
  return s_pData->m_hFallbackReflectionSpecularTexture;
}

// static
ezGALTextureHandle ezReflectionPool::GetSkyIrradianceTexture()
{
  return s_pData->m_hSkyIrradianceTexture;
}

//////////////////////////////////////////////////////////////////////////
/// Private Functions

// static
void ezReflectionPool::OnEngineStartup()
{
  s_pData = EZ_DEFAULT_NEW(ezReflectionPool::Data);

  ezRenderWorld::GetExtractionEvent().AddEventHandler(OnExtractionEvent);
  ezRenderWorld::GetRenderEvent().AddEventHandler(OnRenderEvent);
}

// static
void ezReflectionPool::OnEngineShutdown()
{
  ezRenderWorld::GetExtractionEvent().RemoveEventHandler(OnExtractionEvent);
  ezRenderWorld::GetRenderEvent().RemoveEventHandler(OnRenderEvent);

  EZ_DEFAULT_DELETE(s_pData);
}

// static
void ezReflectionPool::OnExtractionEvent(const ezRenderWorldExtractionEvent& e)
{
  if (e.m_Type == ezRenderWorldExtractionEvent::Type::BeginExtraction)
  {
    EZ_PROFILE_SCOPE("Reflection Pool BeginExtraction");
    s_pData->CreateSkyIrradianceTexture();
    s_pData->CreateReflectionViewsAndResources();
    s_pData->PreExtraction();
  }

  if (e.m_Type == ezRenderWorldExtractionEvent::Type::EndExtraction)
  {
    EZ_PROFILE_SCOPE("Reflection Pool EndExtraction");
    s_pData->PostExtraction();
  }
}

// static
void ezReflectionPool::OnRenderEvent(const ezRenderWorldRenderEvent& e)
{
  if (e.m_Type != ezRenderWorldRenderEvent::Type::BeginRender)
    return;

  if (s_pData->m_hSkyIrradianceTexture.IsInvalidated())
    return;

  EZ_LOCK(s_pData->m_Mutex);

  ezUInt64 uiWorldHasSkyLight = s_pData->m_uiWorldHasSkyLight;
  ezUInt64& uiSkyIrradianceChanged = s_pData->m_uiSkyIrradianceChanged;
  if ((~uiWorldHasSkyLight & uiSkyIrradianceChanged) == 0)
    return;

  auto& skyIrradianceStorage = s_pData->m_SkyIrradianceStorage;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  auto pCommandEncoder = pDevice->BeginCommands("Sky Irradiance Texture Update");
  ezHybridArray<ezGALTextureHandle, 4> atlasToClear;

  for (ezUInt32 i = 0; i < skyIrradianceStorage.GetCount(); ++i)
  {
    if ((uiWorldHasSkyLight & EZ_BIT(i)) == 0 && (uiSkyIrradianceChanged & EZ_BIT(i)) != 0)
    {
      ezBoundingBoxu32 destBox;
      destBox.m_vMin.Set(0, i, 0);
      destBox.m_vMax.Set(6, i + 1, 1);
      ezGALSystemMemoryDescription memDesc;
      memDesc.m_uiRowPitch = sizeof(ezAmbientCube<ezColorLinear16f>);
      memDesc.m_pData = ezMakeByteBlobPtr(&skyIrradianceStorage[i].m_Values[0], memDesc.m_uiRowPitch * 1);
      pCommandEncoder->UpdateTexture(s_pData->m_hSkyIrradianceTexture, ezGALTextureSubresource(), destBox, memDesc);

      uiSkyIrradianceChanged &= ~EZ_BIT(i);

      if (i < s_pData->m_WorldReflectionData.GetCount() && s_pData->m_WorldReflectionData[i] != nullptr)
      {
        ezReflectionPool::Data::WorldReflectionData& data = *s_pData->m_WorldReflectionData[i];
        atlasToClear.PushBack(data.m_mapping.GetTexture());
      }
    }
  }

  {
    // Clear specular sky reflection to black.
    const ezUInt32 uiNumMipMaps = GetMipLevels();
    for (ezGALTextureHandle atlas : atlasToClear)
    {
      for (ezUInt32 uiMipMapIndex = 0; uiMipMapIndex < uiNumMipMaps; ++uiMipMapIndex)
      {
        for (ezUInt32 uiFaceIndex = 0; uiFaceIndex < 6; ++uiFaceIndex)
        {
          ezGALRenderingSetup renderingSetup;
          ezGALRenderTargetViewCreationDescription desc;
          desc.m_hTexture = atlas;
          desc.m_uiMipLevel = uiMipMapIndex;
          desc.m_uiFirstSlice = uiFaceIndex;
          desc.m_uiSliceCount = 1;
          desc.m_OverrideViewType = ezGALTextureType::Texture2DArray;

          renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->CreateRenderTargetView(desc));
          renderingSetup.m_ClearColor = ezColor(0, 0, 0, 1);
          renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;

          pCommandEncoder->BeginRendering(renderingSetup, "ClearSkySpecular");
          pCommandEncoder->EndRendering();
        }
      }
    }
  }

  pDevice->EndCommands(pCommandEncoder);
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_ReflectionPool);
