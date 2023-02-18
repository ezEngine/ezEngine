#include <GameEngine/GameEnginePCH.h>

#include <Core/GameApplication/GameApplicationBase.h>
#include <Core/World/World.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <GameEngine/Configuration/XRConfig.h>
#include <GameEngine/XR/DummyXR.h>
#include <GameEngine/XR/StageSpaceComponent.h>
#include <GameEngine/XR/XRWindow.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>

EZ_IMPLEMENT_SINGLETON(ezDummyXR);

ezDummyXR::ezDummyXR()
  : m_SingletonRegistrar(this)
{
}

ezDummyXR::~ezDummyXR()
{
}

bool ezDummyXR::IsHmdPresent() const
{
  return true;
}

ezResult ezDummyXR::Initialize()
{
  if (m_bInitialized)
    return EZ_FAILURE;

  m_Info.m_sDeviceName = "Dummy VR device";
  m_Info.m_vEyeRenderTargetSize = ezSizeU32(640, 720);

  m_GALdeviceEventsId = ezGALDevice::GetDefaultDevice()->m_Events.AddEventHandler(ezMakeDelegate(&ezDummyXR::GALDeviceEventHandler, this));
  m_ExecutionEventsId = ezGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(ezMakeDelegate(&ezDummyXR::GameApplicationEventHandler, this));

  m_bInitialized = true;
  return EZ_SUCCESS;
}

void ezDummyXR::Deinitialize()
{
  m_bInitialized = false;
  if (m_GALdeviceEventsId != 0)
  {
    ezGALDevice::GetDefaultDevice()->m_Events.RemoveEventHandler(m_GALdeviceEventsId);
  }
  if (m_ExecutionEventsId != 0)
  {
    ezGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(m_ExecutionEventsId);
  }
}

bool ezDummyXR::IsInitialized() const
{
  return m_bInitialized;
}

const ezHMDInfo& ezDummyXR::GetHmdInfo() const
{
  return m_Info;
}

ezXRInputDevice& ezDummyXR::GetXRInput() const
{
  return m_Input;
}

bool ezDummyXR::SupportsCompanionView()
{
  return true;
}

ezUniquePtr<ezActor> ezDummyXR::CreateActor(ezView* pView, ezGALMSAASampleCount::Enum msaaCount, ezUniquePtr<ezWindowBase> pCompanionWindow, ezUniquePtr<ezWindowOutputTargetGAL> pCompanionWindowOutput)
{
  EZ_ASSERT_DEV(IsInitialized(), "Need to call 'Initialize' first.");
  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  // Create dummy swap chain
  {
    ezGALTextureCreationDescription textureDesc;
    textureDesc.SetAsRenderTarget(m_Info.m_vEyeRenderTargetSize.width, m_Info.m_vEyeRenderTargetSize.height, ezGALResourceFormat::RGBAUByteNormalizedsRGB, msaaCount);
    textureDesc.m_uiArraySize = 2;
    textureDesc.m_bAllowShaderResourceView = true;

    m_hColorRT = pDevice->CreateTexture(textureDesc);

    textureDesc.m_Format = ezGALResourceFormat::D24S8;
    m_hDepthRT = pDevice->CreateTexture(textureDesc);
  }

  // SetHMDCamera
  {
    m_pCameraToSynchronize = pView->GetCamera();
    m_pCameraToSynchronize->SetCameraMode(ezCameraMode::Stereo, m_pCameraToSynchronize->GetFovOrDim(), m_pCameraToSynchronize->GetNearPlane(), m_pCameraToSynchronize->GetFarPlane());
  }

  ezUniquePtr<ezActor> pActor = EZ_DEFAULT_NEW(ezActor, "DummyXR", this);
  EZ_ASSERT_DEV((pCompanionWindow != nullptr) == (pCompanionWindowOutput != nullptr), "Both companionWindow and companionWindowOutput must either be null or valid.");

  ezUniquePtr<ezActorPluginWindowXR> pActorPlugin = EZ_DEFAULT_NEW(ezActorPluginWindowXR, this, std::move(pCompanionWindow), std::move(pCompanionWindowOutput));
  m_pCompanion = static_cast<ezWindowOutputTargetXR*>(pActorPlugin->GetOutputTarget());

  pActor->AddPlugin(std::move(pActorPlugin));

  m_hView = pView->GetHandle();
  m_pWorld = pView->GetWorld();
  EZ_ASSERT_DEV(m_pWorld != nullptr, "");


  ezGALRenderTargets renderTargets;
  renderTargets.m_hRTs[0] = m_hColorRT;
  renderTargets.m_hDSTarget = m_hDepthRT;
  pView->SetRenderTargets(renderTargets);

  pView->SetViewport(ezRectFloat((float)m_Info.m_vEyeRenderTargetSize.width, (float)m_Info.m_vEyeRenderTargetSize.height));

  return std::move(pActor);
}

ezGALTextureHandle ezDummyXR::GetCurrentTexture()
{
  return m_hColorRT;
}

void ezDummyXR::OnActorDestroyed()
{
  if (m_hView.IsInvalidated())
    return;

  m_pCompanion = nullptr;
  m_pWorld = nullptr;
  m_pCameraToSynchronize = nullptr;

  ezGALDevice* pDevice = ezGALDevice::GetDefaultDevice();

  if (!m_hColorRT.IsInvalidated())
  {
    pDevice->DestroyTexture(m_hColorRT);
    m_hColorRT.Invalidate();
  }
  if (!m_hDepthRT.IsInvalidated())
  {
    pDevice->DestroyTexture(m_hDepthRT);
    m_hDepthRT.Invalidate();
  }

  ezRenderWorld::RemoveMainView(m_hView);
  m_hView.Invalidate();
}

void ezDummyXR::GALDeviceEventHandler(const ezGALDeviceEvent& e)
{
  if (e.m_Type == ezGALDeviceEvent::Type::BeforeBeginFrame)
  {
  }
  else if (e.m_Type == ezGALDeviceEvent::Type::BeforeEndFrame)
  {
    // Screenshots are taken during present callback so ideally we need to render the companion view before that to capture the current XR frame.
    // For backwards compatibility draw the companion view here (after present) which means that if We are in frame 100, we just rendered frame 99 (due to multi-threaded rendering) but due to this bug here we captured frame 98 for image comparison.
    // This will change once read back API is refactored to be async and will be executed at a different point in time.
    if (m_pCompanion)
    {
      // We capture the companion view in unit tests so we don't want to skip any frames.
      m_pCompanion->RenderCompanionView(false);
    }
  }
}

void ezDummyXR::GameApplicationEventHandler(const ezGameApplicationExecutionEvent& e)
{
  if (e.m_Type == ezGameApplicationExecutionEvent::Type::BeforePresent)
  {
  }
  else if (e.m_Type == ezGameApplicationExecutionEvent::Type::BeforeUpdatePlugins)
  {
    ezView* pView0 = nullptr;
    if (ezRenderWorld::TryGetView(m_hView, pView0))
    {
      if (ezWorld* pWorld0 = pView0->GetWorld())
      {
        EZ_LOCK(pWorld0->GetWriteMarker());
        ezCameraComponent* pCameraComponent = pWorld0->GetComponentManager<ezCameraComponentManager>()->GetCameraByUsageHint(ezCameraUsageHint::MainView);
        if (!pCameraComponent)
          return;

        pCameraComponent->SetCameraMode(ezCameraMode::Stereo);

        // Projection
        {
          const float fAspectRatio = (float)m_Info.m_vEyeRenderTargetSize.width / (float)m_Info.m_vEyeRenderTargetSize.height;

          ezMat4 mProj = ezGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(ezAngle::Degree(pCameraComponent->GetFieldOfView()), fAspectRatio,
            pCameraComponent->GetNearPlane(), ezMath::Max(pCameraComponent->GetNearPlane() + 0.00001f, pCameraComponent->GetFarPlane()));

          m_pCameraToSynchronize->SetStereoProjection(mProj, mProj, fAspectRatio);
        }

        // Update camera view
        {
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
                  ezEnum<ezXRStageSpace> stageSpace = pStage->GetStageSpace();
                  if (m_StageSpace != stageSpace)
                    m_StageSpace = pStage->GetStageSpace();
                  add = pStage->GetOwner()->GetGlobalTransform();
                }
              }
            }
          }

          {
            // Update device state
            ezQuat rot;
            rot.SetIdentity();
            ezVec3 pos = ezVec3::ZeroVector();
            if (m_StageSpace == ezXRStageSpace::Standing)
            {
              pos.z = m_fHeadHeight;
            }

            m_Input.m_DeviceState[0].m_vGripPosition = pos;
            m_Input.m_DeviceState[0].m_qGripRotation = rot;
            m_Input.m_DeviceState[0].m_vAimPosition = pos;
            m_Input.m_DeviceState[0].m_qAimRotation = rot;
            m_Input.m_DeviceState[0].m_Type = ezXRDeviceType::HMD;
            m_Input.m_DeviceState[0].m_bGripPoseIsValid = true;
            m_Input.m_DeviceState[0].m_bAimPoseIsValid = true;
            m_Input.m_DeviceState[0].m_bDeviceIsConnected = true;
          }

          // Set view matrix
          {
            const float fHeight = m_StageSpace == ezXRStageSpace::Standing ? m_fHeadHeight : 0.0f;
            const ezMat4 mStageTransform = add.GetInverse().GetAsMat4();
            ezMat4 poseLeft;
            poseLeft.SetTranslationMatrix(ezVec3(0, -m_fEyeOffset, fHeight));
            ezMat4 poseRight;
            poseRight.SetTranslationMatrix(ezVec3(0, m_fEyeOffset, fHeight));

            // EZ Forward is +X, need to add this to align the forward projection
            const ezMat4 viewMatrix = ezGraphicsUtils::CreateLookAtViewMatrix(ezVec3::ZeroVector(), ezVec3(1, 0, 0), ezVec3(0, 0, 1));
            const ezMat4 mViewTransformLeft = viewMatrix * mStageTransform * poseLeft.GetInverse();
            const ezMat4 mViewTransformRight = viewMatrix * mStageTransform * poseRight.GetInverse();

            m_pCameraToSynchronize->SetViewMatrix(mViewTransformLeft, ezCameraEye::Left);
            m_pCameraToSynchronize->SetViewMatrix(mViewTransformRight, ezCameraEye::Right);
          }
        }
      }
    }
  }
}


//////////////////////////////////////////////////////////////////////////

void ezDummyXRInput::GetDeviceList(ezHybridArray<ezXRDeviceID, 64>& out_devices) const
{
  out_devices.PushBack(0);
}

ezXRDeviceID ezDummyXRInput::GetDeviceIDByType(ezXRDeviceType::Enum type) const
{
  ezXRDeviceID deviceID = -1;
  switch (type)
  {
    case ezXRDeviceType::HMD:
      deviceID = 0;
      break;
    default:
      deviceID = -1;
  }

  return deviceID;
}

const ezXRDeviceState& ezDummyXRInput::GetDeviceState(ezXRDeviceID deviceID) const
{
  EZ_ASSERT_DEV(deviceID < 1 && deviceID >= 0, "Invalid device ID.");
  return m_DeviceState[deviceID];
}

ezString ezDummyXRInput::GetDeviceName(ezXRDeviceID deviceID) const
{
  EZ_ASSERT_DEV(deviceID < 1 && deviceID >= 0, "Invalid device ID.");
  return "Dummy HMD";
}

ezBitflags<ezXRDeviceFeatures> ezDummyXRInput::GetDeviceFeatures(ezXRDeviceID deviceID) const
{
  EZ_ASSERT_DEV(deviceID < 1 && deviceID >= 0, "Invalid device ID.");
  return ezXRDeviceFeatures::AimPose | ezXRDeviceFeatures::GripPose;
}

void ezDummyXRInput::InitializeDevice()
{
}

void ezDummyXRInput::UpdateInputSlotValues()
{
}

void ezDummyXRInput::RegisterInputSlots()
{
}


EZ_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_DummyXR);
