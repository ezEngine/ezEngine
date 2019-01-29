#include <PCH.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Time/Stopwatch.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/Interfaces/SoundInterface.h>
#include <OpenVRPlugin/OpenVRIncludes.h>
#include <OpenVRPlugin/OpenVRSingleton.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererDX11/Resources/TextureDX11.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

#include <../../../Data/Base/Shaders/Pipeline/VRCompanionViewConstants.h>
#include <Core/World/World.h>
#include <GameEngine/VirtualReality/Components/StageSpaceComponent.h>
#include <RendererCore/Shader/ShaderResource.h>

EZ_IMPLEMENT_SINGLETON(ezOpenVR);

static ezOpenVR g_OpenVRSingleton;

ezOpenVR::ezOpenVR()
    : m_SingletonRegistrar(this)
{
  m_bInitialized = false;
  m_DeviceState[0].m_mPose.SetIdentity();
}

bool ezOpenVR::IsHmdPresent() const
{
  return vr::VR_IsHmdPresent();
}

bool ezOpenVR::Initialize()
{
  if (m_bInitialized)
    return true;

  vr::EVRInitError eError = vr::VRInitError_None;
  m_pHMD = vr::VR_Init(&eError, vr::VRApplication_Scene);
  if (eError != vr::VRInitError_None)
  {
    m_pHMD = nullptr;
    ezLog::Error("Unable to init OpenVR runtime: {0}", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
    return false;
  }
  m_pRenderModels = (vr::IVRRenderModels*)vr::VR_GetGenericInterface(vr::IVRRenderModels_Version, &eError);
  if (!m_pRenderModels)
  {
    m_pHMD = nullptr;
    vr::VR_Shutdown();
    ezLog::Error("Unable to get OpenVR render model interface: {0}", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
    return false;
  }

  m_bInitialized = true;
  ezGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(
      ezMakeDelegate(&ezOpenVR::GameApplicationEventHandler, this));
  ezRenderWorld::s_BeginRenderEvent.AddEventHandler(ezMakeDelegate(&ezOpenVR::OnBeginRender, this));
  ezGALDevice::GetDefaultDevice()->m_Events.AddEventHandler(ezMakeDelegate(&ezOpenVR::GALDeviceEventHandler, this));
  ReadHMDInfo();

  SetStageSpace(ezVRStageSpace::Standing);
  for (ezVRDeviceID uiDeviceID = 0; uiDeviceID < vr::k_unMaxTrackedDeviceCount; ++uiDeviceID)
  {
    if (m_pHMD->IsTrackedDeviceConnected(uiDeviceID))
    {
      OnDeviceActivated(uiDeviceID);
    }
  }

  UpdatePoses();

  ezLog::Success("OpenVR initialized successfully.");
  return true;
}

void ezOpenVR::Deinitialize()
{
  if (m_bInitialized)
  {
    ezGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(
        ezMakeDelegate(&ezOpenVR::GameApplicationEventHandler, this));
    ezRenderWorld::s_BeginRenderEvent.RemoveEventHandler(ezMakeDelegate(&ezOpenVR::OnBeginRender, this));
    ezGALDevice::GetDefaultDevice()->m_Events.RemoveEventHandler(ezMakeDelegate(&ezOpenVR::GALDeviceEventHandler, this));

    for (ezVRDeviceID uiDeviceID = 0; uiDeviceID < vr::k_unMaxTrackedDeviceCount; ++uiDeviceID)
    {
      if (m_DeviceState[uiDeviceID].m_bDeviceIsConnected)
      {
        OnDeviceDeactivated(uiDeviceID);
      }
    }

    SetCompanionViewRenderTarget(ezGALTextureHandle());
    DestroyVRView();

    vr::VR_Shutdown();
    m_pHMD = nullptr;
    m_pRenderModels = nullptr;
    m_bInitialized = false;
  }
}

bool ezOpenVR::IsInitialized() const
{
  return m_bInitialized;
}

const ezHMDInfo& ezOpenVR::GetHmdInfo() const
{
  EZ_ASSERT_DEV(m_bInitialized, "Need to call 'Initialize' first.");
  return m_Info;
}

void ezOpenVR::GetDeviceList(ezHybridArray<ezVRDeviceID, 64>& out_Devices) const
{
  EZ_ASSERT_DEV(m_bInitialized, "Need to call 'Initialize' first.");
  for (ezVRDeviceID i = 0; i < vr::k_unMaxTrackedDeviceCount; i++)
  {
    if (m_DeviceState[i].m_bDeviceIsConnected)
    {
      out_Devices.PushBack(i);
    }
  }
}

ezVRDeviceID ezOpenVR::GetDeviceIDByType(ezVRDeviceType::Enum type) const
{
  ezVRDeviceID deviceID = -1;
  switch (type)
  {
    case ezVRDeviceType::HMD:
      deviceID = 0;
      break;
    case ezVRDeviceType::LeftController:
      deviceID = m_iLeftControllerDeviceID;
      break;
    case ezVRDeviceType::RightController:
      deviceID = m_iRightControllerDeviceID;
      break;
    default:
      deviceID = type - ezVRDeviceType::DeviceID0;
      break;
  }

  if (deviceID != -1 && !m_DeviceState[deviceID].m_bDeviceIsConnected)
  {
    deviceID = -1;
  }
  return deviceID;
}

const ezVRDeviceState& ezOpenVR::GetDeviceState(ezVRDeviceID uiDeviceID) const
{
  EZ_ASSERT_DEV(m_bInitialized, "Need to call 'Initialize' first.");
  EZ_ASSERT_DEV(uiDeviceID < vr::k_unMaxTrackedDeviceCount, "Invalid device ID.");
  EZ_ASSERT_DEV(m_DeviceState[uiDeviceID].m_bDeviceIsConnected, "Invalid device ID.");
  return m_DeviceState[uiDeviceID];
}

ezEvent<const ezVRDeviceEvent&>& ezOpenVR::DeviceEvents()
{
  return m_DeviceEvents;
}

ezViewHandle ezOpenVR::CreateVRView(
    const ezRenderPipelineResourceHandle& hRenderPipeline, ezCamera* pCamera, ezGALMSAASampleCount::Enum msaaCount)
{
  SetHMDCamera(pCamera);

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  ezView* pMainView = nullptr;
  m_hView = ezRenderWorld::CreateView("Holographic View", pMainView);
  pMainView->SetCameraUsageHint(ezCameraUsageHint::MainView);

  {
    ezGALTextureCreationDescription tcd;
    tcd.SetAsRenderTarget(
        m_Info.m_vEyeRenderTargetSize.x, m_Info.m_vEyeRenderTargetSize.y, ezGALResourceFormat::RGBAUByteNormalizedsRGB, msaaCount);
    tcd.m_uiArraySize = 2;
    m_hColorRT = pDevice->CreateTexture(tcd);

    // Store desc for one eye for later.
    m_eyeDesc = tcd;
    m_eyeDesc.m_uiArraySize = 1;
  }
  {
    ezGALTextureCreationDescription tcd;
    tcd.SetAsRenderTarget(m_Info.m_vEyeRenderTargetSize.x, m_Info.m_vEyeRenderTargetSize.y, ezGALResourceFormat::DFloat, msaaCount);
    tcd.m_uiArraySize = 2;
    m_hDepthRT = pDevice->CreateTexture(tcd);
  }

  m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(m_hColorRT))
      .SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(m_hDepthRT));

  pMainView->SetRenderTargetSetup(m_RenderTargetSetup);
  pMainView->SetRenderPipelineResource(hRenderPipeline);
  pMainView->SetCamera(&m_VRCamera);
  pMainView->SetRenderPassProperty("ColorSource", "MSAA_Mode", (ezInt32)msaaCount);
  pMainView->SetRenderPassProperty("DepthStencil", "MSAA_Mode", (ezInt32)msaaCount);

  pMainView->SetViewport(ezRectFloat((float)m_Info.m_vEyeRenderTargetSize.x, (float)m_Info.m_vEyeRenderTargetSize.y));

  ezRenderWorld::AddMainView(m_hView);
  return m_hView;
}

ezViewHandle ezOpenVR::GetVRView() const
{
  return m_hView;
}

bool ezOpenVR::DestroyVRView()
{
  if (m_hView.IsInvalidated())
    return false;

  m_pWorld = nullptr;
  SetHMDCamera(nullptr);

  vr::VRCompositor()->ClearLastSubmittedFrame();
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  ezRenderWorld::RemoveMainView(m_hView);
  ezRenderWorld::DeleteView(m_hView);
  m_hView.Invalidate();
  m_RenderTargetSetup.DestroyAllAttachedViews();

  pDevice->DestroyTexture(m_hColorRT);
  m_hColorRT.Invalidate();
  pDevice->DestroyTexture(m_hDepthRT);
  m_hDepthRT.Invalidate();
  return true;
}

bool ezOpenVR::SupportsCompanionView()
{
  return true;
}

bool ezOpenVR::SetCompanionViewRenderTarget(ezGALTextureHandle hRenderTarget)
{
  if (!m_hCompanionRenderTarget.IsInvalidated() && !hRenderTarget.IsInvalidated())
  {
    // Maintain already created resources (just switch target).
  }
  else if (!m_hCompanionRenderTarget.IsInvalidated() && hRenderTarget.IsInvalidated())
  {
    // Delete companion resources.
    ezRenderContext::DeleteConstantBufferStorage(m_hCompanionConstantBuffer);
    m_hCompanionConstantBuffer.Invalidate();
  }
  else if (m_hCompanionRenderTarget.IsInvalidated() && !hRenderTarget.IsInvalidated())
  {
    // Create companion resources.
    m_hCompanionShader = ezResourceManager::LoadResource<ezShaderResource>("Shaders/Pipeline/VRCompanionView.ezShader");
    EZ_ASSERT_DEV(m_hCompanionShader.IsValid(), "Could not load VR companion view shader!");
    m_hCompanionConstantBuffer = ezRenderContext::CreateConstantBufferStorage<ezVRCompanionViewConstants>();
    m_hCompanionRenderTarget = hRenderTarget;
  }
  return true;
}

ezGALTextureHandle ezOpenVR::GetCompanionViewRenderTarget() const
{
  return m_hCompanionRenderTarget;
}

void ezOpenVR::GameApplicationEventHandler(const ezGameApplicationExecutionEvent& e)
{
  EZ_ASSERT_DEV(m_bInitialized, "Need to call 'Initialize' first.");

  if (e.m_Type == ezGameApplicationExecutionEvent::Type::BeforeUpdatePlugins)
  {
    vr::VREvent_t event;
    while (m_pHMD->PollNextEvent(&event, sizeof(event)))
    {
      switch (event.eventType)
      {
        case vr::VREvent_TrackedDeviceActivated:
        {
          OnDeviceActivated((ezVRDeviceID)event.trackedDeviceIndex);
        }
        break;
        case vr::VREvent_TrackedDeviceDeactivated:
        {
          OnDeviceDeactivated((ezVRDeviceID)event.trackedDeviceIndex);
        }
        break;
      }
    }
  }
  else if (e.m_Type == ezGameApplicationExecutionEvent::Type::BeforePresent)
  {
    if (m_hView.IsInvalidated())
      return;

    ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
    ezGALContext* pGALContext = pDevice->GetPrimaryContext();

    ezGALTextureHandle hLeft;
    ezGALTextureHandle hRight;

    if (m_eyeDesc.m_SampleCount == ezGALMSAASampleCount::None)
    {
      // OpenVR does not support submitting texture arrays, as there is no slice param in the VRTextureBounds_t :-/
      // However, it reads it just fine for the first slice (left eye) so we only need to copy the right eye into a
      // second texture to submit it :-)
      hLeft = m_hColorRT;
      hRight = ezGPUResourcePool::GetDefaultInstance()->GetRenderTarget(m_eyeDesc);
      ezGALTextureSubresource sourceSubRes;
      sourceSubRes.m_uiArraySlice = 1;
      sourceSubRes.m_uiMipLevel = 0;

      ezGALTextureSubresource destSubRes;
      destSubRes.m_uiArraySlice = 0;
      destSubRes.m_uiMipLevel = 0;

      pGALContext->CopyTextureRegion(hRight, destSubRes, ezVec3U32(0, 0, 0), m_hColorRT, sourceSubRes,
          ezBoundingBoxu32(ezVec3U32(0, 0, 0), ezVec3U32(m_Info.m_vEyeRenderTargetSize.x, m_Info.m_vEyeRenderTargetSize.y, 1)));
    }
    else
    {
      // Submitting the multi-sampled m_hColorRT will cause dx errors on submit :-/
      // So have to resolve both eyes.
      ezGALTextureCreationDescription tempDesc = m_eyeDesc;
      tempDesc.m_SampleCount = ezGALMSAASampleCount::None;
      hLeft = ezGPUResourcePool::GetDefaultInstance()->GetRenderTarget(tempDesc);
      hRight = ezGPUResourcePool::GetDefaultInstance()->GetRenderTarget(tempDesc);

      ezGALTextureSubresource sourceSubRes;
      sourceSubRes.m_uiArraySlice = 0;
      sourceSubRes.m_uiMipLevel = 0;

      ezGALTextureSubresource destSubRes;
      destSubRes.m_uiArraySlice = 0;
      destSubRes.m_uiMipLevel = 0;
      pGALContext->ResolveTexture(hLeft, destSubRes, m_hColorRT, sourceSubRes);
      sourceSubRes.m_uiArraySlice = 1;
      pGALContext->ResolveTexture(hRight, destSubRes, m_hColorRT, sourceSubRes);
    }

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    // TODO: We currently assume that we always use dx11 on windows. Need to figure out how to check for that.
    vr::Texture_t Texture;
    Texture.eType = vr::TextureType_DirectX;
    Texture.eColorSpace = vr::ColorSpace_Auto;

    {
      const ezGALTexture* pTex = pDevice->GetTexture(hLeft);
      const ezGALTextureDX11* pTex11 = static_cast<const ezGALTextureDX11*>(pTex);
      Texture.handle = pTex11->GetDXTexture();
      vr::EVRCompositorError err = vr::VRCompositor()->Submit(vr::Eye_Left, &Texture, nullptr);
    }

    {
      const ezGALTexture* pTex = pDevice->GetTexture(hRight);
      const ezGALTextureDX11* pTex11 = static_cast<const ezGALTextureDX11*>(pTex);
      Texture.handle = pTex11->GetDXTexture();
      vr::EVRCompositorError err = vr::VRCompositor()->Submit(vr::Eye_Right, &Texture, nullptr);
    }
#endif

    if (m_eyeDesc.m_SampleCount == ezGALMSAASampleCount::None)
    {
      ezGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(hRight);
    }
    else
    {
      ezGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(hLeft);
      ezGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(hRight);
    }
  }
  else if (e.m_Type == ezGameApplicationExecutionEvent::Type::BeginAppTick)
  {
  }
  else if (e.m_Type == ezGameApplicationExecutionEvent::Type::AfterPresent)
  {
    // This tells the compositor we submitted the frames are done rendering to them this frame.
    vr::VRCompositor()->PostPresentHandoff();

    ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();
    ezGALContext* pGALContext = pDevice->GetPrimaryContext();
    ezRenderContext* m_pRenderContext = ezRenderContext::GetDefaultInstance();

    if (const ezGALTexture* tex = pDevice->GetTexture(m_hCompanionRenderTarget))
    {
      // We are rendering the companion window at the very start of the frame, using the content
      // of the last frame. That way we do not add additional delay before submitting the frames.
      EZ_PROFILE_AND_MARKER(pGALContext, "VR CompanionView");

      m_pRenderContext->BindMeshBuffer(ezGALBufferHandle(), ezGALBufferHandle(), nullptr, ezGALPrimitiveTopology::Triangles, 1);
      m_pRenderContext->BindConstantBuffer("ezVRCompanionViewConstants", m_hCompanionConstantBuffer);
      m_pRenderContext->BindShader(m_hCompanionShader);

      auto hRenderTargetView = ezGALDevice::GetDefaultDevice()->GetDefaultRenderTargetView(m_hCompanionRenderTarget);
      ezVec2 targetSize = ezVec2((float)tex->GetDescription().m_uiWidth, (float)tex->GetDescription().m_uiHeight);

      ezGALRenderTagetSetup renderTargetSetup;
      renderTargetSetup.SetRenderTarget(0, hRenderTargetView);
      pGALContext->SetRenderTargetSetup(renderTargetSetup);
      pGALContext->SetViewport(ezRectFloat(targetSize.x, targetSize.y));

      auto* constants = ezRenderContext::GetConstantBufferData<ezVRCompanionViewConstants>(m_hCompanionConstantBuffer);
      constants->TargetSize = targetSize;

      ezGALResourceViewHandle hInputView = pDevice->GetDefaultResourceView(m_hColorRT);
      m_pRenderContext->BindTexture2D("VRTexture", hInputView);
      m_pRenderContext->DrawMeshBuffer();
    }
  }
}

void ezOpenVR::GALDeviceEventHandler(const ezGALDeviceEvent& e)
{
  if (e.m_Type == ezGALDeviceEvent::Type::BeforeBeginFrame)
  {
    // Will call 'WaitGetPoses' which will block the thread. Alternatively we can
    // call 'PostPresentHandoff' but then we need to do more work ourselves.
    // According to the docu at 'PostPresentHandoff' both calls should happen
    // after present, the docu for 'WaitGetPoses' contradicts this :-/
    // This needs to happen on the render thread as OpenVR will use DX calls.
    UpdatePoses();

    // This will update the extracted view from last frame with the new data we got
    // this frame just before starting to render.
    ezView* pView = nullptr;
    if (ezRenderWorld::TryGetView(m_hView, pView))
    {
      pView->UpdateViewData(ezRenderWorld::GetDataIndexForRendering());
    }
  }
}

void ezOpenVR::OnBeginRender(ezUInt64)
{
  // TODO: Ideally we would like to call UpdatePoses() here and block and in BeforeBeginFrame
  // we would predict the pose in two frames.
}

void ezOpenVR::ReadHMDInfo()
{
  EZ_ASSERT_DEV(m_bInitialized, "Need to call 'Initialize' first.");

  m_Info.m_sDeviceName = GetTrackedDeviceString(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String);
  m_Info.m_sDeviceDriver = GetTrackedDeviceString(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String);
  m_pHMD->GetRecommendedRenderTargetSize(&m_Info.m_vEyeRenderTargetSize.x, &m_Info.m_vEyeRenderTargetSize.y);
  m_Info.m_mat4eyePosLeft = GetHMDEyePose(vr::Eye_Left);
  m_Info.m_mat4eyePosRight = GetHMDEyePose(vr::Eye_Right);
}


void ezOpenVR::OnDeviceActivated(ezVRDeviceID uiDeviceID)
{
  m_DeviceState[uiDeviceID].m_bDeviceIsConnected = true;
  switch (m_pHMD->GetTrackedDeviceClass(uiDeviceID))
  {
    case vr::TrackedDeviceClass_HMD:
      m_DeviceState[uiDeviceID].m_Type = ezVRDeviceState::Type::HMD;
      break;
    case vr::TrackedDeviceClass_Controller:
      m_DeviceState[uiDeviceID].m_Type = ezVRDeviceState::Type::Controller;

      break;
    case vr::TrackedDeviceClass_GenericTracker:
      m_DeviceState[uiDeviceID].m_Type = ezVRDeviceState::Type::Tracker;
      break;
    case vr::TrackedDeviceClass_TrackingReference:
      m_DeviceState[uiDeviceID].m_Type = ezVRDeviceState::Type::Reference;
      break;
    default:
      m_DeviceState[uiDeviceID].m_Type = ezVRDeviceState::Type::Unknown;
      break;
  }

  UpdateHands();

  {
    ezVRDeviceEvent e;
    e.m_Type = ezVRDeviceEvent::Type::DeviceAdded;
    e.uiDeviceID = uiDeviceID;
    m_DeviceEvents.Broadcast(e);
  }
}


void ezOpenVR::OnDeviceDeactivated(ezVRDeviceID uiDeviceID)
{
  m_DeviceState[uiDeviceID].m_bDeviceIsConnected = false;
  m_DeviceState[uiDeviceID].m_bPoseIsValid = false;
  UpdateHands();
  {
    ezVRDeviceEvent e;
    e.m_Type = ezVRDeviceEvent::Type::DeviceRemoved;
    e.uiDeviceID = uiDeviceID;
    m_DeviceEvents.Broadcast(e);
  }
}

void ezOpenVR::UpdatePoses()
{
  EZ_ASSERT_DEV(m_bInitialized, "Need to call 'Initialize' first.");

  ezStopwatch sw;

  UpdateHands();
  vr::TrackedDevicePose_t TrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
  vr::EVRCompositorError err = vr::VRCompositor()->WaitGetPoses(TrackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0);
  for (ezVRDeviceID uiDeviceID = 0; uiDeviceID < vr::k_unMaxTrackedDeviceCount; ++uiDeviceID)
  {
    m_DeviceState[uiDeviceID].m_bPoseIsValid = TrackedDevicePose[uiDeviceID].bPoseIsValid;
    if (TrackedDevicePose[uiDeviceID].bPoseIsValid)
    {
      m_DeviceState[uiDeviceID].m_vVelocity = ConvertSteamVRVector(TrackedDevicePose[uiDeviceID].vVelocity);
      m_DeviceState[uiDeviceID].m_vAngularVelocity = ConvertSteamVRVector(TrackedDevicePose[uiDeviceID].vAngularVelocity);
      m_DeviceState[uiDeviceID].m_mPose = ConvertSteamVRMatrix(TrackedDevicePose[uiDeviceID].mDeviceToAbsoluteTracking);
      m_DeviceState[uiDeviceID].m_vPosition = m_DeviceState[uiDeviceID].m_mPose.GetTranslationVector();
      m_DeviceState[uiDeviceID].m_qRotation.SetFromMat3(m_DeviceState[uiDeviceID].m_mPose.GetRotationalPart());
    }
  }

  if (m_pCameraToSynchronize)
  {
    UpdateCamera();

    ezMat4 viewMatrix;
    viewMatrix.SetLookAtMatrix(ezVec3::ZeroVector(), ezVec3(0, -1, 0), ezVec3(0, 0, 1));

    ezTransform add;
    add.SetIdentity();
    ezView* pView = nullptr;
    if (ezRenderWorld::TryGetView(m_hView, pView))
    {
      if (const ezWorld* pWorld = pView->GetWorld())
      {
        EZ_LOCK(pWorld->GetReadMarker());
        if (const ezStageSpaceComponentManager* pStageMan = pWorld->GetComponentManager<ezStageSpaceComponentManager>())
        {
          if (const ezStageSpaceComponent* pStage = pStageMan->GetSingletonComponent())
          {
            auto stageSpace = pStage->GetStageSpace();
            if (m_StageSpace != stageSpace)
              SetStageSpace(pStage->GetStageSpace());
            add = pStage->GetOwner()->GetGlobalTransform();
          }
        }
      }
    }

    const ezMat4 mAdd = add.GetAsMat4();
    ezMat4 mShiftedPos = m_DeviceState[0].m_mPose * mAdd;
    mShiftedPos.Invert();

    const ezMat4 mViewTransformLeft = viewMatrix * m_Info.m_mat4eyePosLeft * mShiftedPos;
    const ezMat4 mViewTransformRight = viewMatrix * m_Info.m_mat4eyePosRight * mShiftedPos;

    m_VRCamera.SetViewMatrix(mViewTransformLeft, ezCameraEye::Left);
    m_VRCamera.SetViewMatrix(mViewTransformRight, ezCameraEye::Right);

    // put the camera orientation into the sound listener and enable the listener override mode
    if (ezSoundInterface* pSoundInterface = ezSingletonRegistry::GetSingletonInstance<ezSoundInterface>("ezSoundInterface"))
    {
      pSoundInterface->SetListener(
          -1, m_VRCamera.GetCenterPosition(), m_VRCamera.GetCenterDirForwards(), m_VRCamera.GetCenterDirUp(), ezVec3::ZeroVector());
    }
  }
}

void ezOpenVR::UpdateHands()
{
  m_iLeftControllerDeviceID = m_pHMD->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_LeftHand);
  m_iRightControllerDeviceID = m_pHMD->GetTrackedDeviceIndexForControllerRole(vr::TrackedControllerRole_RightHand);
}

void ezOpenVR::SetStageSpace(ezVRStageSpace::Enum space)
{
  m_StageSpace = space;
  switch (space)
  {
    case ezVRStageSpace::Seated:
      vr::VRCompositor()->SetTrackingSpace(vr::TrackingUniverseOrigin::TrackingUniverseSeated);
      break;
    case ezVRStageSpace::Standing:
      vr::VRCompositor()->SetTrackingSpace(vr::TrackingUniverseOrigin::TrackingUniverseStanding);
      break;
  }
}

void ezOpenVR::SetHMDCamera(ezCamera* pCamera)
{
  EZ_ASSERT_DEV(m_bInitialized, "Need to call 'Initialize' first.");

  if (m_pCameraToSynchronize == pCamera)
    return;

  m_pCameraToSynchronize = pCamera;
  if (m_pCameraToSynchronize)
  {
    m_uiSettingsModificationCounter = m_pCameraToSynchronize->GetSettingsModificationCounter() + 1;
    m_VRCamera = *m_pCameraToSynchronize;
    m_VRCamera.SetCameraMode(ezCameraMode::Stereo, 90.0f, m_pCameraToSynchronize->GetNearPlane(), m_pCameraToSynchronize->GetFarPlane());
    UpdateCamera();
  }

  if (ezSoundInterface* pSoundInterface = ezSingletonRegistry::GetSingletonInstance<ezSoundInterface>("ezSoundInterface"))
  {
    pSoundInterface->SetListenerOverrideMode(m_pCameraToSynchronize != nullptr);
  }
}


void ezOpenVR::UpdateCamera()
{
  if (m_uiSettingsModificationCounter != m_pCameraToSynchronize->GetSettingsModificationCounter())
  {
    const float fAspectRatio = (float)m_Info.m_vEyeRenderTargetSize.x / (float)m_Info.m_vEyeRenderTargetSize.y;
    ezMat4 projLeft =
        GetHMDProjectionEye(vr::Hmd_Eye::Eye_Left, m_pCameraToSynchronize->GetNearPlane(), m_pCameraToSynchronize->GetFarPlane());
    ezMat4 projRight =
        GetHMDProjectionEye(vr::Hmd_Eye::Eye_Right, m_pCameraToSynchronize->GetNearPlane(), m_pCameraToSynchronize->GetFarPlane());
    m_VRCamera.SetStereoProjection(projLeft, projRight, fAspectRatio);
  }
}

ezMat4 ezOpenVR::GetHMDProjectionEye(vr::Hmd_Eye nEye, float fNear, float fFar) const
{
  EZ_ASSERT_DEV(m_pHMD, "Need to call 'Initialize' first.");

  float Left, Right, Top, Bottom;
  m_pHMD->GetProjectionRaw(nEye, &Left, &Right, &Top, &Bottom);
  ezMat4 proj;
  proj.SetPerspectiveProjectionMatrix(Left * fNear, Right * fNear, Top * fNear, Bottom * fNear, fNear, fFar);
  return proj;
}

ezMat4 ezOpenVR::GetHMDEyePose(vr::Hmd_Eye nEye) const
{
  EZ_ASSERT_DEV(m_pHMD, "Need to call 'Initialize' first.");

  vr::HmdMatrix34_t matEyeRight = m_pHMD->GetEyeToHeadTransform(nEye);
  ezMat4 matrixObj = ConvertSteamVRMatrix(matEyeRight);
  matrixObj.Invert();
  return matrixObj;
}

ezString ezOpenVR::GetTrackedDeviceString(
    vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError* peError) const
{
  EZ_ASSERT_DEV(m_pHMD, "Need to call 'Initialize' first.");

  const ezUInt32 uiCharCount = m_pHMD->GetStringTrackedDeviceProperty(unDevice, prop, nullptr, 0, peError);
  ezHybridArray<char, 128> temp;
  temp.SetCountUninitialized(uiCharCount);
  if (uiCharCount > 0)
  {
    m_pHMD->GetStringTrackedDeviceProperty(unDevice, prop, temp.GetData(), uiCharCount, peError);
  }
  else
  {
    temp.SetCount(1);
    temp[0] = 0;
  }
  return ezString(temp.GetData());
}

ezMat4 ezOpenVR::ConvertSteamVRMatrix(const vr::HmdMatrix34_t& matPose)
{
  // clang-format off
  // Convert right handed to left handed with Y and Z swapped.
  // Same as A^t * matPose * A with A being identity with y and z swapped.
  ezMat4 mMat(
    matPose.m[0][0], matPose.m[0][2], matPose.m[0][1], matPose.m[0][3],
    matPose.m[2][0], matPose.m[2][2], matPose.m[2][1], matPose.m[2][3],
    matPose.m[1][0], matPose.m[1][2], matPose.m[1][1], matPose.m[1][3],
    0, 0, 0, 1.0f);

  return mMat;
  // clang-format on
}

ezVec3 ezOpenVR::ConvertSteamVRVector(const vr::HmdVector3_t& vector)
{
  return ezVec3(vector.v[0], vector.v[2], vector.v[1]);
}

EZ_STATICLINK_FILE(OpenVRPlugin, OpenVRPlugin_OpenVRSingleton);
