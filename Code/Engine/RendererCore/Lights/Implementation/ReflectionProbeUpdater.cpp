#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/Implementation/ReflectionProbeUpdater.h>

#include <Foundation/Math/Color16f.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Lights/Implementation/ReflectionPoolData.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>

ezCVarInt cvar_RenderingReflectionPoolMaxRenderViews("Rendering.ReflectionPool.MaxRenderViews", 1, ezCVarFlags::Default, "The maximum number of render views for reflection probes each frame");
ezCVarInt cvar_RenderingReflectionPoolMaxFilterViews("Rendering.ReflectionPool.MaxFilterViews", 1, ezCVarFlags::Default, "The maximum number of filter views for reflection probes each frame");



//////////////////////////////////////////////////////////////////////////
/// ProbeUpdateInfo

ezReflectionProbeUpdater::ProbeUpdateInfo::ProbeUpdateInfo()
{
  m_globalTransform.SetIdentity();

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  {
    ezGALTextureCreationDescription desc;
    desc.m_uiWidth = s_uiReflectionCubeMapSize;
    desc.m_uiHeight = s_uiReflectionCubeMapSize;
    desc.m_uiMipLevelCount = GetMipLevels();
    desc.m_Format = ezGALResourceFormat::RGBAHalf;
    desc.m_Type = ezGALTextureType::TextureCube;
    desc.m_bCreateRenderTarget = true;
    desc.m_bAllowDynamicMipGeneration = true;
    desc.m_ResourceAccess.m_bReadBack = true;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hCubemap = ezGPUResourcePool::GetDefaultInstance()->GetRenderTarget(desc);
    pDevice->GetTexture(m_hCubemap)->SetDebugName("Reflection Cubemap");
  }

  ezStringBuilder sName;
  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(m_hCubemapProxies); ++i)
  {
    m_hCubemapProxies[i] = ezGALDevice::GetDefaultDevice()->CreateProxyTexture(m_hCubemap, i);

    sName.SetFormat("Reflection Cubemap Proxy {}", i);
    pDevice->GetTexture(m_hCubemapProxies[i])->SetDebugName(sName);
  }
}

ezReflectionProbeUpdater::ProbeUpdateInfo::~ProbeUpdateInfo()
{
  for (ezUInt32 i = 0; i < EZ_ARRAY_SIZE(m_hCubemapProxies); ++i)
  {
    if (!m_hCubemapProxies[i].IsInvalidated())
    {
      ezGALDevice::GetDefaultDevice()->DestroyProxyTexture(m_hCubemapProxies[i]);
    }
  }

  if (!m_hCubemap.IsInvalidated())
  {
    ezGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(m_hCubemap);
  }
}


//////////////////////////////////////////////////////////////////////////
/// ezReflectionProbeUpdater

ezReflectionProbeUpdater::ezReflectionProbeUpdater() = default;

ezReflectionProbeUpdater::~ezReflectionProbeUpdater()
{
  for (auto& renderView : m_RenderViews)
  {
    ezRenderWorld::DeleteView(renderView.m_hView);
  }

  for (auto& filterView : m_FilterViews)
  {
    ezRenderWorld::DeleteView(filterView.m_hView);
  }
}

ezUInt32 ezReflectionProbeUpdater::GetFreeUpdateSlots(ezDynamicArray<ezReflectionProbeRef>& out_updatesFinished)
{
  out_updatesFinished = m_FinishedLastFrame;
  m_FinishedLastFrame.Clear();
  ezUInt32 uiCount = 0;
  for (auto& slot : m_DynamicUpdates)
  {
    if (!slot->m_bInUse)
      ++uiCount;
  }
  return uiCount;
}

ezResult ezReflectionProbeUpdater::StartDynamicUpdate(const ezReflectionProbeRef& probe, const ezReflectionProbeDesc& desc, const ezTransform& globalTransform, const TargetSlot& target)
{
  EZ_ASSERT_DEBUG(target.m_hIrradianceOutputTexture.IsInvalidated() == (target.m_iIrradianceOutputIndex == -1), "Invalid irradiance output settings.");
  EZ_ASSERT_DEBUG(!target.m_hSpecularOutputTexture.IsInvalidated() && target.m_iSpecularOutputIndex != -1, "Specular output invalid.");
  for (auto& slot : m_DynamicUpdates)
  {
    if (!slot->m_bInUse)
    {
      slot->m_flags = target.m_iIrradianceOutputIndex != -1 ? ezReflectionProbeUpdaterFlags::SkyLight : ezReflectionProbeUpdaterFlags::Default;
      slot->m_probe = probe;
      slot->m_desc = desc;
      slot->m_globalTransform = globalTransform;
      // Ignore scale when rendering the probe. Zero scaled components will otherwise cause asserts.
      slot->m_globalTransform.m_vScale = ezVec3(1.0f);
      slot->m_sourceTexture.Invalidate();
      slot->m_TargetSlot = target;
      slot->m_bInUse = true;
      return EZ_SUCCESS;
    }
  }
  return EZ_FAILURE;
}

ezResult ezReflectionProbeUpdater::StartFilterUpdate(const ezReflectionProbeRef& probe, const ezReflectionProbeDesc& desc, ezTextureCubeResourceHandle hSourceTexture, const TargetSlot& target)
{
  EZ_ASSERT_DEBUG(target.m_hIrradianceOutputTexture.IsInvalidated() == (target.m_iIrradianceOutputIndex == -1), "Invalid irradiance output settings.");
  EZ_ASSERT_DEBUG(!target.m_hSpecularOutputTexture.IsInvalidated() && target.m_iSpecularOutputIndex != -1, "Specular output invalid.");
  for (auto& slot : m_DynamicUpdates)
  {
    if (!slot->m_bInUse)
    {
      slot->m_flags = ezReflectionProbeUpdaterFlags::HasCustomCubeMap;
      if (target.m_iIrradianceOutputIndex != -1)
      {
        slot->m_flags.Add(ezReflectionProbeUpdaterFlags::SkyLight);
      }
      slot->m_probe = probe;
      slot->m_desc = desc;
      slot->m_globalTransform.SetIdentity();
      slot->m_sourceTexture = hSourceTexture;
      slot->m_TargetSlot = target;
      slot->m_bInUse = true;
      return EZ_SUCCESS;
    }
  }
  return EZ_FAILURE;
}

void ezReflectionProbeUpdater::CancelUpdate(const ezReflectionProbeRef& probe)
{
  m_FinishedLastFrame.RemoveAndSwap(probe);
  for (ezUInt32 uiInfo = m_DynamicUpdates.GetCount(); uiInfo-- > 0;)
  {
    if (m_DynamicUpdates[uiInfo]->m_probe == probe)
    {
      ResetProbeUpdateInfo(uiInfo);
      // A probe can be in the queue multiple times so don't exit the loop.
    }
  }
}

void ezReflectionProbeUpdater::GenerateUpdateSteps()
{
  if (!m_bUpdateStepsFlushed)
    return;

  m_bUpdateStepsFlushed = false;
  ezUInt32 uiRenderViewIndex = 0;
  ezUInt32 uiFilterViewIndex = 0;

  ezUInt32 uiSortedUpdateInfoIndex = 0;
  while (uiSortedUpdateInfoIndex < m_DynamicUpdates.GetCount())
  {
    auto pUpdateInfo = m_DynamicUpdates[uiSortedUpdateInfoIndex].Borrow();
    if (!pUpdateInfo->m_bInUse)
    {
      ++uiSortedUpdateInfoIndex;
      continue;
    }

    auto& updateSteps = pUpdateInfo->m_UpdateSteps;
    UpdateStep::Enum nextStep = UpdateStep::NextStep(updateSteps.IsEmpty() ? pUpdateInfo->m_LastUpdateStep : updateSteps.PeekBack().m_UpdateStep);

    if (pUpdateInfo->m_flags.IsSet(ezReflectionProbeUpdaterFlags::HasCustomCubeMap))
      nextStep = UpdateStep::Filter;

    bool bNextProbe = false;

    if (UpdateStep::IsRenderStep(nextStep))
    {
      if (uiRenderViewIndex < m_RenderViews.GetCount())
      {
        updateSteps.PushBack({(ezUInt8)uiRenderViewIndex, nextStep});
        ++uiRenderViewIndex;
      }
      else
      {
        bNextProbe = true;
      }
    }
    else if (nextStep == UpdateStep::Filter)
    {
      // don't do render and filter in one frame
      if (uiFilterViewIndex < m_FilterViews.GetCount() && updateSteps.IsEmpty())
      {
        updateSteps.PushBack({(ezUInt8)uiFilterViewIndex, nextStep});
        ++uiFilterViewIndex;
      }
      bNextProbe = true;
    }

    // break if no more views are available
    if (uiRenderViewIndex == m_RenderViews.GetCount() && uiFilterViewIndex == m_FilterViews.GetCount())
    {
      break;
    }

    if (bNextProbe)
    {
      ++uiSortedUpdateInfoIndex;
    }
  }
}

void ezReflectionProbeUpdater::ScheduleUpdateSteps()
{
  if (m_bUpdateStepsFlushed)
  {
    return;
  }
  m_bUpdateStepsFlushed = true;

  // #TODO: would like to do that in the ctor but then the renderer tests assert that don't have the base asset directory set up.
  CreateReflectionViewsAndResources();

  // Iterate in reverse as ResetProbeUpdateInfo will move the current index to the back of the array.
  for (ezUInt32 uiInfo = m_DynamicUpdates.GetCount(); uiInfo-- > 0;)
  {
    ProbeUpdateInfo& info = *m_DynamicUpdates[uiInfo];
    if (info.m_bInUse)
    {
      bool bDone = false;
      if (!info.m_UpdateSteps.IsEmpty())
      {
        // Render steps are done in reverse order so the last committed view is rendered first. Thus we need to iterate the array in reverse order.
        for (ezUInt32 uiStep = info.m_UpdateSteps.GetCount(); uiStep-- > 0;)
        {
          if (info.m_UpdateSteps[uiStep].m_UpdateStep == UpdateStep::Filter)
          {
            bDone = true;
          }

          AddViewToRender(info.m_UpdateSteps[uiStep], info);
        }

        bool bIsLoadingResources = false;
        if (info.m_desc.m_Mode == ezReflectionProbeMode::Static && info.m_sourceTexture.IsValid())
        {
          // Wait until the input texture is fully loaded.
          ezResourceLock<ezTextureCubeResource> pTexture(info.m_sourceTexture, ezResourceAcquireMode::AllowLoadingFallback);
          if (pTexture->GetLoadingState() != ezResourceState::Loaded || pTexture->GetNumQualityLevelsLoadable() > 0 || pTexture->GetResourceHandle() != info.m_sourceTexture)
          {
            bIsLoadingResources = true;
          }
        }
        else
        {
          bIsLoadingResources = ezResourceManager::IsAnyLoadingInProgress();
        }

        if (!bIsLoadingResources)
        {
          info.m_LastUpdateStep = info.m_UpdateSteps.PeekBack().m_UpdateStep;
        }
        info.m_UpdateSteps.Clear();
        if (bDone && !bIsLoadingResources)
        {
          m_FinishedLastFrame.PushBack(info.m_probe);
          ResetProbeUpdateInfo(uiInfo);
        }
      }
    }
  }
}

void ezReflectionProbeUpdater::CreateViews(
  ezDynamicArray<ReflectionView>& views, ezUInt32 uiMaxRenderViews, const char* szNameSuffix, const char* szRenderPipelineResource)
{
  uiMaxRenderViews = ezMath::Max<ezUInt32>(uiMaxRenderViews, 1);

  if (uiMaxRenderViews > views.GetCount())
  {
    ezStringBuilder sName;

    ezUInt32 uiCurrentCount = views.GetCount();
    for (ezUInt32 i = uiCurrentCount; i < uiMaxRenderViews; ++i)
    {
      auto& renderView = views.ExpandAndGetRef();

      sName.SetFormat("Reflection Probe {} {}", szNameSuffix, i);

      ezView* pView = nullptr;
      renderView.m_hView = ezRenderWorld::CreateView(sName, pView);

      pView->SetCameraUsageHint(ezCameraUsageHint::Reflection);
      pView->SetViewport(ezRectFloat(0.0f, 0.0f, static_cast<float>(s_uiReflectionCubeMapSize), static_cast<float>(s_uiReflectionCubeMapSize)));

      pView->SetRenderPipelineResource(ezResourceManager::LoadResource<ezRenderPipelineResource>(szRenderPipelineResource));

      renderView.m_Camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, 0.1f, 100.0f);
      pView->SetCamera(&renderView.m_Camera);
    }
  }
  else if (uiMaxRenderViews < views.GetCount())
  {
    views.SetCount(uiMaxRenderViews);
  }
}

void ezReflectionProbeUpdater::CreateReflectionViewsAndResources()
{
  // ReflectionRenderPipeline.ezRenderPipelineAsset
  CreateViews(m_RenderViews, cvar_RenderingReflectionPoolMaxRenderViews, "Render", "{ 734898e8-b1a2-0da2-c4ae-701912983c2f }");

  // ReflectionFilterPipeline.ezRenderPipelineAsset
  CreateViews(m_FilterViews, cvar_RenderingReflectionPoolMaxFilterViews, "Filter", "{ 3437db17-ddf1-4b67-b80f-9999d6b0c352 }");

  if (m_DynamicUpdates.IsEmpty())
  {
    for (ezUInt32 i = 0; i < 2; i++)
    {
      m_DynamicUpdates.PushBack(EZ_DEFAULT_NEW(ProbeUpdateInfo));
    }
  }
}

void ezReflectionProbeUpdater::ResetProbeUpdateInfo(ezUInt32 uiInfo)
{
  // Reset and move to the end of the queue.
  ezUniquePtr<ProbeUpdateInfo> info = std::move(m_DynamicUpdates[uiInfo]);
  info->m_bInUse = false;
  info->m_flags = {};
  info->m_probe.m_Id.Invalidate();
  info->m_probe.m_uiWorldIndex = 0;
  info->m_globalTransform.SetIdentity();
  info->m_sourceTexture.Invalidate();
  info->m_LastUpdateStep = UpdateStep::Default;
  info->m_UpdateSteps.Clear();

  m_DynamicUpdates.RemoveAtAndCopy(uiInfo);
  m_DynamicUpdates.PushBack(std::move(info));
}

void ezReflectionProbeUpdater::AddViewToRender(const ProbeUpdateInfo::Step& step, ProbeUpdateInfo& updateInfo)
{
  ezVec3 vForward[6] = {
    ezVec3(1.0f, 0.0f, 0.0f),
    ezVec3(-1.0f, 0.0f, 0.0f),
    ezVec3(0.0f, 0.0f, 1.0f),
    ezVec3(0.0f, 0.0f, -1.0f),
    ezVec3(0.0f, -1.0f, 0.0f),
    ezVec3(0.0f, 1.0f, 0.0f),
  };

  ezVec3 vUp[6] = {
    ezVec3(0.0f, 0.0f, 1.0f),
    ezVec3(0.0f, 0.0f, 1.0f),
    ezVec3(0.0f, 1.0f, 0.0f),
    ezVec3(0.0f, -1.0f, 0.0f),
    ezVec3(0.0f, 0.0f, 1.0f),
    ezVec3(0.0f, 0.0f, 1.0f),
  };

  // Setup view and camera
  {
    ReflectionView* pReflectionView = nullptr;
    ezUInt32 uiFaceIndex = 0;

    if (step.m_UpdateStep == UpdateStep::Filter)
    {
      pReflectionView = &m_FilterViews[step.m_uiViewIndex];
    }
    else
    {
      pReflectionView = &m_RenderViews[step.m_uiViewIndex];
      uiFaceIndex = step.m_UpdateStep;
    }

    ezView* pView = nullptr;
    ezRenderWorld::TryGetView(pReflectionView->m_hView, pView);

    pView->m_IncludeTags = updateInfo.m_desc.m_IncludeTags;
    pView->m_ExcludeTags = updateInfo.m_desc.m_ExcludeTags;
    ezWorld* pWorld = ezWorld::GetWorld(updateInfo.m_probe.m_uiWorldIndex);
    pView->SetWorld(pWorld);

    ezGALRenderTargets renderTargets;
    if (step.m_UpdateStep == UpdateStep::Filter)
    {
      renderTargets.m_hRTs[0] = updateInfo.m_TargetSlot.m_hSpecularOutputTexture;

      if (updateInfo.m_flags.IsSet(ezReflectionProbeUpdaterFlags::SkyLight))
      {
        renderTargets.m_hRTs[2] = updateInfo.m_TargetSlot.m_hIrradianceOutputTexture;
      }
      pView->SetRenderPassProperty("ReflectionFilterPass", "Intensity", updateInfo.m_desc.m_fIntensity);
      pView->SetRenderPassProperty("ReflectionFilterPass", "Saturation", updateInfo.m_desc.m_fSaturation);
      pView->SetRenderPassProperty("ReflectionFilterPass", "SpecularOutputIndex", updateInfo.m_TargetSlot.m_iSpecularOutputIndex);
      pView->SetRenderPassProperty("ReflectionFilterPass", "IrradianceOutputIndex", updateInfo.m_TargetSlot.m_iIrradianceOutputIndex);


      ezGALTextureHandle hSourceTexture = updateInfo.m_hCubemap;
      if (updateInfo.m_desc.m_Mode == ezReflectionProbeMode::Static)
      {
        if (updateInfo.m_flags.IsSet(ezReflectionProbeUpdaterFlags::HasCustomCubeMap))
        {
          ezResourceLock<ezTextureCubeResource> pTexture(updateInfo.m_sourceTexture, ezResourceAcquireMode::BlockTillLoaded);
          // #TODO Currently even in static mode we render the 6 sides and only change the filter stage to point to the static texture if available. Rendering the 6 sides is intended only in the editor as a preview for non-baked probes. We will need to find a way to quickly determine if we need to do this fallback at a much earlier stage.
          if (pTexture->GetLoadingState() == ezResourceState::Loaded && pTexture->GetResourceHandle() == updateInfo.m_sourceTexture)
          {
            hSourceTexture = pTexture->GetGALTexture();
          }
        }
      }
      pView->SetRenderPassProperty("ReflectionFilterPass", "InputCubemap", hSourceTexture.GetInternalID().m_Data);
    }
    else
    {
      renderTargets.m_hRTs[0] = updateInfo.m_hCubemapProxies[uiFaceIndex];
    }
    pView->SetRenderTargets(renderTargets);

    ezVec3 vPosition = updateInfo.m_globalTransform * updateInfo.m_desc.m_vCaptureOffset;
    ezVec3 vForward2 = updateInfo.m_globalTransform.TransformDirection(vForward[uiFaceIndex]);
    ezVec3 vUp2 = updateInfo.m_globalTransform.TransformDirection(vUp[uiFaceIndex]);
    if (updateInfo.m_flags.IsSet(ezReflectionProbeUpdaterFlags::SkyLight))
    {
      vForward2 = vForward[uiFaceIndex];
      vUp2 = vUp[uiFaceIndex];
    }

    const float fFar = updateInfo.m_desc.m_fFarPlane;
    float fNear = updateInfo.m_desc.m_fNearPlane;
    if (fNear >= fFar)
    {
      fNear = fFar - 0.001f;
    }
    else if (fNear == 0.0f)
    {
      fNear = fFar / 1000.0f;
    }

    pReflectionView->m_Camera.LookAt(vPosition, vPosition + vForward2, vUp2);
    pReflectionView->m_Camera.SetCameraMode(ezCameraMode::PerspectiveFixedFovX, 90.0f, fNear, fFar);
    ezRenderWorld::AddViewToRender(pReflectionView->m_hView);
  }
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_ReflectionProbeUpdater);
