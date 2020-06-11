#include <OpenXRPluginPCH.h>

#include <OpenXRPlugin/OpenXRDeclarations.h>
#include <OpenXRPlugin/OpenXRHandTracking.h>
#include <OpenXRPlugin/OpenXRSingleton.h>
#include <GameEngine/XR/StageSpaceComponent.h>
#include <Core/World/World.h>
#include <Foundation/Profiling/Profiling.h>

EZ_IMPLEMENT_SINGLETON(ezOpenXRHandTracking);

bool ezOpenXRHandTracking::IsHandTrackingSupported(ezOpenXR* pOpenXR)
{
#ifdef BUILDSYSTEM_ENABLE_OPENXR_PREVIEW_SUPPORT
  XrSystemHandTrackingPropertiesMSFT handTrackingSystemProperties{ XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_MSFT };
  XrSystemProperties systemProperties{ XR_TYPE_SYSTEM_PROPERTIES, &handTrackingSystemProperties };
  XrResult res = xrGetSystemProperties(pOpenXR->m_instance, pOpenXR->m_systemId, &systemProperties);
  if (res == XrResult::XR_SUCCESS)
  {
    return handTrackingSystemProperties.supportsHandTracking;
  }
#endif
  return false;
}

ezOpenXRHandTracking::ezOpenXRHandTracking(ezOpenXR* pOpenXR)
  : m_SingletonRegistrar(this)
  , m_pOpenXR(pOpenXR)
{
#ifdef BUILDSYSTEM_ENABLE_OPENXR_PREVIEW_SUPPORT
  EZ_ASSERT_DEV(m_pOpenXR->m_extensions.m_bHandTracking, "Hand tracking not supported");
  for (ezUInt32 uiSide : {0, 1})
  {
    const XrHandMSFT uiHand = uiSide == 0 ? XR_HAND_LEFT_MSFT : XR_HAND_RIGHT_MSFT;
    XrHandTrackerCreateInfoMSFT createInfo{ XR_TYPE_HAND_TRACKER_CREATE_INFO_MSFT };
    createInfo.hand = uiHand;
    XR_LOG_ERROR(m_pOpenXR->m_extensions.pfn_xrCreateHandTrackerMSFT(pOpenXR->m_session, &createInfo, &m_HandTracker[uiSide]));

    XrHandJointSpaceCreateInfoMSFT jointCreateInfo{ XR_TYPE_HAND_JOINT_SPACE_CREATE_INFO_MSFT };
    jointCreateInfo.handTracker = m_HandTracker[uiSide];
    jointCreateInfo.poseInJointSpace = { {0, 0, 0, 1}, {0, 0, 0} };

    m_JointData[uiSide].SetCount(XR_HAND_JOINT_LITTLE_TIP_MSFT + 1);
    for (ezUInt32 i = 0; i <= XR_HAND_JOINT_LITTLE_TIP_MSFT; ++i)
    {
      m_JointData[uiSide][i].m_Bone.m_Transform.SetIdentity();
      jointCreateInfo.joint = (XrHandJointMSFT)i;
      XR_LOG_ERROR(m_pOpenXR->m_extensions.pfn_xrCreateHandJointSpaceMSFT(pOpenXR->m_session, &jointCreateInfo, &m_JointData[uiSide][i].m_Space));
    }
  }

  //Map hand parts to hand joints
  m_HandParts[ezXRHandPart::Palm].PushBack(XR_HAND_JOINT_PALM_MSFT);
  m_HandParts[ezXRHandPart::Palm].PushBack(XR_HAND_JOINT_WRIST_MSFT);

  m_HandParts[ezXRHandPart::Wrist].PushBack(XR_HAND_JOINT_WRIST_MSFT);

  m_HandParts[ezXRHandPart::Thumb].PushBack(XR_HAND_JOINT_THUMB_TIP_MSFT);
  m_HandParts[ezXRHandPart::Thumb].PushBack(XR_HAND_JOINT_THUMB_DISTAL_MSFT);
  m_HandParts[ezXRHandPart::Thumb].PushBack(XR_HAND_JOINT_THUMB_PROXIMAL_MSFT);
  m_HandParts[ezXRHandPart::Thumb].PushBack(XR_HAND_JOINT_THUMB_METACARPAL_MSFT);
  m_HandParts[ezXRHandPart::Thumb].PushBack(XR_HAND_JOINT_WRIST_MSFT);

  m_HandParts[ezXRHandPart::Index].PushBack(XR_HAND_JOINT_INDEX_TIP_MSFT);
  m_HandParts[ezXRHandPart::Index].PushBack(XR_HAND_JOINT_INDEX_DISTAL_MSFT);
  m_HandParts[ezXRHandPart::Index].PushBack(XR_HAND_JOINT_INDEX_INTERMEDIATE_MSFT);
  m_HandParts[ezXRHandPart::Index].PushBack(XR_HAND_JOINT_INDEX_PROXIMAL_MSFT);
  m_HandParts[ezXRHandPart::Index].PushBack(XR_HAND_JOINT_INDEX_METACARPAL_MSFT);
  m_HandParts[ezXRHandPart::Index].PushBack(XR_HAND_JOINT_WRIST_MSFT);

  m_HandParts[ezXRHandPart::Middle].PushBack(XR_HAND_JOINT_MIDDLE_TIP_MSFT);
  m_HandParts[ezXRHandPart::Middle].PushBack(XR_HAND_JOINT_MIDDLE_DISTAL_MSFT);
  m_HandParts[ezXRHandPart::Middle].PushBack(XR_HAND_JOINT_MIDDLE_INTERMEDIATE_MSFT);
  m_HandParts[ezXRHandPart::Middle].PushBack(XR_HAND_JOINT_MIDDLE_PROXIMAL_MSFT);
  m_HandParts[ezXRHandPart::Middle].PushBack(XR_HAND_JOINT_MIDDLE_METACARPAL_MSFT);
  m_HandParts[ezXRHandPart::Middle].PushBack(XR_HAND_JOINT_WRIST_MSFT);

  m_HandParts[ezXRHandPart::Ring].PushBack(XR_HAND_JOINT_RING_TIP_MSFT);
  m_HandParts[ezXRHandPart::Ring].PushBack(XR_HAND_JOINT_RING_DISTAL_MSFT);
  m_HandParts[ezXRHandPart::Ring].PushBack(XR_HAND_JOINT_RING_INTERMEDIATE_MSFT);
  m_HandParts[ezXRHandPart::Ring].PushBack(XR_HAND_JOINT_RING_PROXIMAL_MSFT);
  m_HandParts[ezXRHandPart::Ring].PushBack(XR_HAND_JOINT_RING_METACARPAL_MSFT);
  m_HandParts[ezXRHandPart::Ring].PushBack(XR_HAND_JOINT_WRIST_MSFT);

  m_HandParts[ezXRHandPart::Little].PushBack(XR_HAND_JOINT_LITTLE_TIP_MSFT);
  m_HandParts[ezXRHandPart::Little].PushBack(XR_HAND_JOINT_LITTLE_DISTAL_MSFT);
  m_HandParts[ezXRHandPart::Little].PushBack(XR_HAND_JOINT_LITTLE_INTERMEDIATE_MSFT);
  m_HandParts[ezXRHandPart::Little].PushBack(XR_HAND_JOINT_LITTLE_PROXIMAL_MSFT);
  m_HandParts[ezXRHandPart::Little].PushBack(XR_HAND_JOINT_LITTLE_METACARPAL_MSFT);
  m_HandParts[ezXRHandPart::Little].PushBack(XR_HAND_JOINT_WRIST_MSFT);
#endif
}

ezOpenXRHandTracking::~ezOpenXRHandTracking()
{
#ifdef BUILDSYSTEM_ENABLE_OPENXR_PREVIEW_SUPPORT
  for (ezUInt32 uiSide : {0, 1})
  {
    for (ezUInt32 i = 0; i <= XR_HAND_JOINT_LITTLE_TIP_MSFT; ++i)
    {
      XR_LOG_ERROR(xrDestroySpace(m_JointData[uiSide][i].m_Space));
    }
    XR_LOG_ERROR(m_pOpenXR->m_extensions.pfn_xrDestroyHandTrackerMSFT(m_HandTracker[uiSide]));
  }
#endif
}

ezXRHandTrackingInterface::HandPartTrackingState ezOpenXRHandTracking::TryGetBoneTransforms(
  ezEnum<ezXRHand> hand, ezEnum<ezXRHandPart> handPart, ezEnum<ezXRTransformSpace> space,
  ezDynamicArray<ezXRHandBone>& out_bones)
{
#ifdef BUILDSYSTEM_ENABLE_OPENXR_PREVIEW_SUPPORT
  EZ_ASSERT_DEV(handPart <= ezXRHandPart::Little, "Invalid hand part.");
  out_bones.Clear();

  for (ezUInt32 uiJointIndex : m_HandParts[handPart])
  {
    const JointData& jointData = m_JointData[hand][uiJointIndex];
    if (!jointData.m_bValid)
      return ezXRHandTrackingInterface::HandPartTrackingState::Untracked;

    out_bones.PushBack(jointData.m_Bone);
  }

  if (space == ezXRTransformSpace::Global)
  {
    const ezWorld* pWorld = m_pOpenXR->m_pWorld;
    if (const ezStageSpaceComponentManager* pStageMan = pWorld->GetComponentManager<ezStageSpaceComponentManager>())
    {
      if (const ezStageSpaceComponent* pStage = pStageMan->GetSingletonComponent())
      {
        ezTransform globalStageTransform = pStage->GetOwner()->GetGlobalTransform();
        for (ezXRHandBone& bone : out_bones)
        {
          ezTransform local = bone.m_Transform;
          bone.m_Transform.SetGlobalTransform(globalStageTransform, local);
        }
      }
    }
  }
  return ezXRHandTrackingInterface::HandPartTrackingState::Tracked;
#else
  return ezXRHandTrackingInterface::HandPartTrackingState::NotSupported;
#endif
}

void ezOpenXRHandTracking::UpdateJointTransforms()
{
#ifdef BUILDSYSTEM_ENABLE_OPENXR_PREVIEW_SUPPORT
  EZ_PROFILE_SCOPE("UpdateJointTransforms");
  const XrTime time = m_pOpenXR->m_frameState.predictedDisplayTime;
  for (ezUInt32 uiSide : {0, 1})
  {
    for (ezUInt32 i = 0; i <= XR_HAND_JOINT_LITTLE_TIP_MSFT; ++i)
    {
      XrHandJointRadiusMSFT jointRadius{ XR_TYPE_HAND_JOINT_RADIUS_MSFT };
      XrSpaceLocation spaceLocation{ XR_TYPE_SPACE_LOCATION, &jointRadius };
      XrResult res = xrLocateSpace(m_JointData[uiSide][i].m_Space, m_pOpenXR->GetBaseSpace(), time, &spaceLocation);
      if (res == XrResult::XR_SUCCESS &&
        (spaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
        (spaceLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0)
      {
        m_JointData[uiSide][i].m_bValid = true;
        m_JointData[uiSide][i].m_Bone.m_fRadius = jointRadius.radius;
        m_JointData[uiSide][i].m_Bone.m_Transform.m_vPosition = ezOpenXR::ConvertPosition(spaceLocation.pose.position);
        m_JointData[uiSide][i].m_Bone.m_Transform.m_qRotation = ezOpenXR::ConvertOrientation(spaceLocation.pose.orientation);
      }
      else
      {
        m_JointData[uiSide][i].m_bValid = false;
      }
    }
  }
#endif
}

EZ_STATICLINK_FILE(OpenXRPlugin, OpenXRPlugin_OpenXRHandTracking);
