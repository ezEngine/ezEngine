#include <PCH.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Time/Stopwatch.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/Interfaces/SoundInterface.h>
#include <OpenVRPlugin/OpenVRIncludes.h>
#include <OpenVRPlugin/OpenVRSingleton.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererDX11/Resources/TextureDX11.h>
#include <RendererFoundation/Context/Context.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

EZ_IMPLEMENT_SINGLETON(ezOpenVR);

static ezOpenVR g_OpenVRSingleton;

ezOpenVR::ezOpenVR()
    : m_SingletonRegistrar(this)
{
  m_bInitialized = false;
  m_DeviceState[0].m_mPose.SetIdentity();
  m_AdditionalCameraTransform.SetIdentity();
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
  ezGameApplication::GetGameApplicationInstance()->m_Events.AddEventHandler(
      ezMakeDelegate(&ezOpenVR::GameApplicationEventHandler, ezOpenVR::GetSingleton()));

  ReadHMDInfo();

  for (ezUInt8 uiDeviceID = 0; uiDeviceID < vr::k_unMaxTrackedDeviceCount; ++uiDeviceID)
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
    ezGameApplication::GetGameApplicationInstance()->m_Events.RemoveEventHandler(
        ezMakeDelegate(&ezOpenVR::GameApplicationEventHandler, ezOpenVR::GetSingleton()));

    for (ezUInt8 uiDeviceID = 0; uiDeviceID < vr::k_unMaxTrackedDeviceCount; ++uiDeviceID)
    {
      if (m_DeviceState[uiDeviceID].m_bDeviceIsConnected)
      {
        OnDeviceDeactivated(uiDeviceID);
      }
    }

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

void ezOpenVR::GetDeviceList(ezHybridArray<ezUInt8, 64>& out_Devices) const
{
  EZ_ASSERT_DEV(m_bInitialized, "Need to call 'Initialize' first.");
  for (ezUInt8 i = 0; i < vr::k_unMaxTrackedDeviceCount; i++)
  {
    if (m_DeviceState[i].m_bDeviceIsConnected)
    {
      out_Devices.PushBack(i);
    }
  }
}

const ezVRDeviceState& ezOpenVR::GetDeviceState(ezUInt8 uiDeviceID) const
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

ezViewHandle ezOpenVR::CreateVRView(const ezRenderPipelineResourceHandle& hRenderPipeline, ezCamera* pCamera,
                                             ezGALMSAASampleCount::Enum msaaCount)
{
  SetHMDCamera(pCamera);

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  ezView* pMainView = nullptr;
  m_hView = ezRenderWorld::CreateView("Holographic View", pMainView);
  pMainView->SetCameraUsageHint(ezCameraUsageHint::MainView);

  {
    ezGALTextureCreationDescription tcd;
    tcd.SetAsRenderTarget(m_Info.m_vEyeRenderTargetSize.x, m_Info.m_vEyeRenderTargetSize.y, ezGALResourceFormat::RGBAUByteNormalizedsRGB,
                          msaaCount);
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
  pMainView->SetCamera(pCamera);
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

void ezOpenVR::GameApplicationEventHandler(const ezGameApplicationEvent& e)
{
  EZ_ASSERT_DEV(m_bInitialized, "Need to call 'Initialize' first.");

  if (e.m_Type == ezGameApplicationEvent::Type::BeforeUpdatePlugins)
  {
    vr::VREvent_t event;
    while (m_pHMD->PollNextEvent(&event, sizeof(event)))
    {
      switch (event.eventType)
      {
        case vr::VREvent_TrackedDeviceActivated:
        {
          OnDeviceActivated((ezUInt8)event.trackedDeviceIndex);
        }
        break;
        case vr::VREvent_TrackedDeviceDeactivated:
        {
          OnDeviceDeactivated((ezUInt8)event.trackedDeviceIndex);
        }
        break;
      }
    }
  }
  else if (e.m_Type == ezGameApplicationEvent::Type::BeforePresent)
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

      pGALContext->CopyTextureRegion(
          hRight, destSubRes, ezVec3U32(0, 0, 0), m_hColorRT, sourceSubRes,
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
  else if (e.m_Type == ezGameApplicationEvent::Type::AfterPresent)
  {
    // Will call 'WaitGetPoses' which will block the thread. Alternatively we can
    // call 'PostPresentHandoff' but then we need to do more work ourselves.
    // According to the docu at 'PostPresentHandoff' both calls should happen
    // after present, the docu for 'WaitGetPoses' contradicts this :-/
    // This needs to happen on the render thread as OpenVR will use DX calls.
    ezOpenVR::GetSingleton()->UpdatePoses();
  }
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


void ezOpenVR::OnDeviceActivated(ezUInt8 uiDeviceID)
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

  {
    ezVRDeviceEvent e;
    e.m_Type = ezVRDeviceEvent::Type::DeviceAdded;
    e.uiDeviceID = uiDeviceID;
    m_DeviceEvents.Broadcast(e);
  }
}


void ezOpenVR::OnDeviceDeactivated(ezUInt8 uiDeviceID)
{
  m_DeviceState[uiDeviceID].m_bDeviceIsConnected = false;
  m_DeviceState[uiDeviceID].m_bPoseIsValid = false;

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

  vr::TrackedDevicePose_t TrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
  vr::EVRCompositorError err = vr::VRCompositor()->WaitGetPoses(TrackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0);
  for (ezUInt8 uiDeviceID = 0; uiDeviceID < vr::k_unMaxTrackedDeviceCount; ++uiDeviceID)
  {
    m_DeviceState[uiDeviceID].m_bPoseIsValid = TrackedDevicePose[uiDeviceID].bPoseIsValid;
    if (TrackedDevicePose[uiDeviceID].bPoseIsValid)
    {
      m_DeviceState[uiDeviceID].m_vVelocity = ConvertSteamVRVector(TrackedDevicePose[uiDeviceID].vVelocity);
      m_DeviceState[uiDeviceID].m_vAngularVelocity = ConvertSteamVRVector(TrackedDevicePose[uiDeviceID].vAngularVelocity);
      m_DeviceState[uiDeviceID].m_mPose = ConvertSteamVRMatrix(TrackedDevicePose[uiDeviceID].mDeviceToAbsoluteTracking);
      m_DeviceState[uiDeviceID].m_mPose.Invert();
    }
  }

  if (m_pCameraToSynchronize)
  {
    ezMat4 viewMatrix;
    viewMatrix.SetLookAtMatrix(ezVec3::ZeroVector(), ezVec3(0, -1, 0), ezVec3(0, 0, 1));

    const ezMat4 mAdd = m_AdditionalCameraTransform.GetAsMat4();
    const ezMat4 mShiftedPos = m_DeviceState[0].m_mPose * mAdd;
    const ezMat4 mViewTransformLeft = viewMatrix * m_Info.m_mat4eyePosLeft * mShiftedPos;
    const ezMat4 mViewTransformRight = viewMatrix * m_Info.m_mat4eyePosRight * mShiftedPos;

    m_pCameraToSynchronize->SetViewMatrix(mViewTransformLeft, ezCameraEye::Left);
    m_pCameraToSynchronize->SetViewMatrix(mViewTransformRight, ezCameraEye::Right);

    // put the camera orientation into the sound listener and enable the listener override mode
    if (ezSoundInterface* pSoundInterface = ezSingletonRegistry::GetSingletonInstance<ezSoundInterface>("ezSoundInterface"))
    {
      pSoundInterface->SetListener(-1, m_pCameraToSynchronize->GetCenterPosition(), m_pCameraToSynchronize->GetCenterDirForwards(),
                                   m_pCameraToSynchronize->GetCenterDirUp(), ezVec3::ZeroVector());
    }
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
    pCamera->SetCameraMode(ezCameraMode::Stereo, 90.0f, pCamera->GetNearPlane(), pCamera->GetFarPlane());
    const float fAspectRatio = (float)m_Info.m_vEyeRenderTargetSize.x / (float)m_Info.m_vEyeRenderTargetSize.y;
    ezMat4 projLeft =
        GetHMDProjectionEye(vr::Hmd_Eye::Eye_Left, m_pCameraToSynchronize->GetNearPlane(), m_pCameraToSynchronize->GetFarPlane());
    ezMat4 projRight =
        GetHMDProjectionEye(vr::Hmd_Eye::Eye_Right, m_pCameraToSynchronize->GetNearPlane(), m_pCameraToSynchronize->GetFarPlane());
    m_pCameraToSynchronize->SetStereoProjection(projLeft, projRight, fAspectRatio);
  }

  if (ezSoundInterface* pSoundInterface = ezSingletonRegistry::GetSingletonInstance<ezSoundInterface>("ezSoundInterface"))
  {
    pSoundInterface->SetListenerOverrideMode(m_pCameraToSynchronize != nullptr);
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

ezString ezOpenVR::GetTrackedDeviceString(vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop,
                                          vr::TrackedPropertyError* peError) const
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
