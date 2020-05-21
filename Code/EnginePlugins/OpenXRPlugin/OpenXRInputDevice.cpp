#include <OpenXRPluginPCH.h>

#include <OpenXRPlugin/OpenXRInputDevice.h>
#include <OpenXRPlugin/OpenXRSingleton.h>
#include <OpenXRPlugin/OpenXRDeclarations.h>
#include <Core/Input/InputManager.h>
#include <Foundation/Profiling/Profiling.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezOpenXRInputDevice, 1, ezRTTINoAllocator);
// no properties or message handlers
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezString ezOpenXRInputDevice::GetInteractionProfile() const
{
  throw std::logic_error("The method or operation is not implemented.");
}

void ezOpenXRInputDevice::GetDeviceList(ezHybridArray<ezXRDeviceID, 64>& out_Devices) const
{
  EZ_ASSERT_DEV(m_pOpenXR->IsInitialized(), "Need to call 'Initialize' first.");
  out_Devices.PushBack(0);
  for (ezXRDeviceID i = 0; i < 3; i++)
  {
    if (m_DeviceState[i].m_bDeviceIsConnected)
    {
      out_Devices.PushBack(i);
    }
  }
}

ezXRDeviceID ezOpenXRInputDevice::GetDeviceIDByType(ezXRDeviceType::Enum type) const
{
  ezXRDeviceID deviceID = -1;
  switch (type)
  {
    case ezXRDeviceType::HMD:
      deviceID = 0;
      break;
    case ezXRDeviceType::LeftController:
      deviceID = m_iLeftControllerDeviceID;
      break;
    case ezXRDeviceType::RightController:
      deviceID = m_iRightControllerDeviceID;
      break;
    default:
      deviceID = -1;
      break;
  }

  if (deviceID != -1 && !m_DeviceState[deviceID].m_bDeviceIsConnected)
  {
    deviceID = -1;
  }
  return deviceID;
}

const ezXRDeviceState& ezOpenXRInputDevice::GetDeviceState(ezXRDeviceID iDeviceID) const
{
  EZ_ASSERT_DEV(m_pOpenXR->IsInitialized(), "Need to call 'Initialize' first.");
  EZ_ASSERT_DEV(iDeviceID < 3 && iDeviceID >= 0, "Invalid device ID.");
  EZ_ASSERT_DEV(m_DeviceState[iDeviceID].m_bDeviceIsConnected, "Invalid device ID.");
  return m_DeviceState[iDeviceID];
}

ezOpenXRInputDevice::ezOpenXRInputDevice(ezOpenXR* pOpenXR)
  : ezXRInputDevice()
  , m_pOpenXR(pOpenXR)
{
  m_instance = m_pOpenXR->m_instance;
}

XrResult ezOpenXRInputDevice::CreateActions(XrSession session, XrSpace sceneSpace)
{
  m_session = session;
  // OpenXR has no concept of connected so assume everything always is.
  m_DeviceState[0].m_bDeviceIsConnected = true;
  m_DeviceState[1].m_bDeviceIsConnected = true;
  m_DeviceState[2].m_bDeviceIsConnected = true;

  XrActionSetCreateInfo actionSetInfo{ XR_TYPE_ACTION_SET_CREATE_INFO };
  strcpy(actionSetInfo.actionSetName, "gameplay");
  strcpy(actionSetInfo.localizedActionSetName, "Gameplay");
  actionSetInfo.priority = 0;
  XR_SUCCEED_OR_CLEANUP_LOG(xrCreateActionSet(m_instance, &actionSetInfo, &m_ActionSet), DestroyActions);

  m_subActionPrefix.SetCount(2);
  m_subActionPrefix[0] = "xr_hand_left_";
  m_subActionPrefix[1] = "xr_hand_right_";

  m_subActionPath.SetCount(2);
  m_subActionPath[0] = CreatePath("/user/hand/left");
  m_subActionPath[1] = CreatePath("/user/hand/right");

  XR_SUCCEED_OR_CLEANUP_LOG(CreateAction(XR_Select_Click, XR_ACTION_TYPE_BOOLEAN_INPUT, m_SelectClick), DestroyActions);
  XR_SUCCEED_OR_CLEANUP_LOG(CreateAction(XR_Menu_Click, XR_ACTION_TYPE_BOOLEAN_INPUT, m_MenuClick), DestroyActions);
  XR_SUCCEED_OR_CLEANUP_LOG(CreateAction(XR_Grip_Pose, XR_ACTION_TYPE_POSE_INPUT, m_GripPose), DestroyActions);
  XR_SUCCEED_OR_CLEANUP_LOG(CreateAction(XR_Aim_Pose, XR_ACTION_TYPE_POSE_INPUT, m_AimPose), DestroyActions);

  Bind simpleController[] = {
        {m_SelectClick, "/user/hand/left/input/select"},
        {m_MenuClick, "/user/hand/left/input/menu"},
        {m_GripPose, "/user/hand/left/input/grip"},
        {m_AimPose, "/user/hand/left/input/aim"},

        {m_SelectClick, "/user/hand/right/input/select"},
        {m_MenuClick, "/user/hand/right/input/menu"},
        {m_GripPose, "/user/hand/right/input/grip"},
        {m_AimPose, "/user/hand/right/input/aim"},
  };
  SuggestInteractionProfileBindings("/interaction_profiles/khr/simple_controller", simpleController);

  Bind motionController[] = {
        {m_SelectClick, "/user/hand/left/input/trigger"},
        {m_MenuClick, "/user/hand/left/input/menu"},
        {m_GripPose, "/user/hand/left/input/grip"},
        {m_AimPose, "/user/hand/left/input/aim"},

        {m_SelectClick, "/user/hand/right/input/trigger"},
        {m_MenuClick, "/user/hand/right/input/menu"},
        {m_GripPose, "/user/hand/right/input/grip"},
        {m_AimPose, "/user/hand/right/input/aim"},
  };
  SuggestInteractionProfileBindings("/interaction_profiles/microsoft/motion_controller", motionController);

  if (m_pOpenXR->m_extensions.m_bHandInteraction)
  {
    Bind handInteraction[] = {
        {m_SelectClick, "/user/hand/left/input/select"},
        {m_GripPose, "/user/hand/left/input/grip"},
        {m_AimPose, "/user/hand/left/input/aim"},

        {m_SelectClick, "/user/hand/right/input/select"},
        {m_GripPose, "/user/hand/right/input/grip"},
        {m_AimPose, "/user/hand/right/input/aim"},
    };
    SuggestInteractionProfileBindings("/interaction_profiles/microsoft/hand_interaction", handInteraction);
  }


  XrActionSpaceCreateInfo spaceCreateInfo{ XR_TYPE_ACTION_SPACE_CREATE_INFO };
  spaceCreateInfo.poseInActionSpace = m_pOpenXR->ConvertTransform(ezTransform::IdentityTransform());
  for (ezUInt32 uiSide : {0, 1})
  {
    spaceCreateInfo.subactionPath = m_subActionPath[uiSide];
    spaceCreateInfo.action = m_GripPose;
    XR_SUCCEED_OR_CLEANUP_LOG(xrCreateActionSpace(m_session, &spaceCreateInfo, &m_gripSpace[uiSide]), DestroyActions);

    spaceCreateInfo.action = m_AimPose;
    XR_SUCCEED_OR_CLEANUP_LOG(xrCreateActionSpace(m_session, &spaceCreateInfo, &m_aimSpace[uiSide]), DestroyActions);
  }
  return XR_SUCCESS;
}

void ezOpenXRInputDevice::DestroyActions()
{
  for (Action& action : m_booleanActions)
  {
    XR_LOG_ERROR(xrDestroyAction(action.action));
  }
  m_booleanActions.Clear();
  for (Action& action : m_poseActions)
  {
    XR_LOG_ERROR(xrDestroyAction(action.action));
  }
  m_poseActions.Clear();

  m_SelectClick = XR_NULL_HANDLE;
  m_MenuClick = XR_NULL_HANDLE;
  m_GripPose = XR_NULL_HANDLE;
  m_AimPose = XR_NULL_HANDLE;

  if (m_ActionSet)
  {
    XR_LOG_ERROR(xrDestroyActionSet(m_ActionSet));
  }

  for (ezUInt32 uiSide : {0, 1})
  {
    if (m_gripSpace[uiSide])
    {
      XR_LOG_ERROR(xrDestroySpace(m_gripSpace[uiSide]));
      m_gripSpace[uiSide] = XR_NULL_HANDLE;
    }
    if (m_aimSpace[uiSide])
    {
      XR_LOG_ERROR(xrDestroySpace(m_aimSpace[uiSide]));
      m_aimSpace[uiSide] = XR_NULL_HANDLE;
    }
  }
}

XrPath ezOpenXRInputDevice::CreatePath(const char* szPath)
{
  XrInstance instance = m_pOpenXR->m_instance;

  XrPath path;
  if (xrStringToPath(instance, szPath, &path) != XR_SUCCESS)
  {
    ezLog::Error("OpenXR path conversion failure: {0}", szPath);
  }
  return path;
}

XrResult ezOpenXRInputDevice::CreateAction(const char* actionName, XrActionType actionType, XrAction& out_action)
{
  XrActionCreateInfo actionCreateInfo{ XR_TYPE_ACTION_CREATE_INFO };
  actionCreateInfo.actionType = actionType;
  actionCreateInfo.countSubactionPaths = m_subActionPath.GetCount();
  actionCreateInfo.subactionPaths = m_subActionPath.GetData();
  ezStringUtils::Copy(actionCreateInfo.actionName, XR_MAX_ACTION_NAME_SIZE, actionName);
  ezStringUtils::Copy(actionCreateInfo.localizedActionName, XR_MAX_LOCALIZED_ACTION_NAME_SIZE, actionName);

  XR_SUCCEED_OR_CLEANUP_LOG(xrCreateAction(m_ActionSet, &actionCreateInfo, &out_action), voidFunction);

  ezStringBuilder sLeft(m_subActionPrefix[0], actionName);
  ezStringBuilder sRight(m_subActionPrefix[1], actionName);

  switch (actionType)
  {
    case XR_ACTION_TYPE_BOOLEAN_INPUT:
      m_booleanActions.PushBack({ out_action, sLeft, sRight });
      break;
    case XR_ACTION_TYPE_POSE_INPUT:
      m_poseActions.PushBack({ out_action, sLeft, sRight });
      break;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }

  return XR_SUCCESS;
}

XrResult ezOpenXRInputDevice::SuggestInteractionProfileBindings(const char* szInteractionProfile, ezArrayPtr<Bind> bindings)
{
  XrInstance instance = m_pOpenXR->m_instance;

  XrPath InteractionProfile = CreatePath(szInteractionProfile);

  ezDynamicArray<XrActionSuggestedBinding> xrBindings;
  xrBindings.Reserve(bindings.GetCount());
  for (const Bind& binding : bindings)
  {
    xrBindings.PushBack({ binding.action, CreatePath(binding.szPath) });
  }


  XrInteractionProfileSuggestedBinding profileBindings{ XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING };
  profileBindings.interactionProfile = InteractionProfile;
  profileBindings.suggestedBindings = xrBindings.GetData();
  profileBindings.countSuggestedBindings = xrBindings.GetCount();
  XR_SUCCEED_OR_RETURN_LOG(xrSuggestInteractionProfileBindings(instance, &profileBindings));
  return XR_SUCCESS;
}

XrResult ezOpenXRInputDevice::AttachSessionActionSets(XrSession session)
{
  m_session = session;
  XrSessionActionSetsAttachInfo attachInfo{ XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO };
  ezHybridArray<XrActionSet, 1> actionSets;
  actionSets.PushBack(m_ActionSet);

  attachInfo.countActionSets = actionSets.GetCount();
  attachInfo.actionSets = actionSets.GetData();
  return xrAttachSessionActionSets(session, &attachInfo);
}

void ezOpenXRInputDevice::InitializeDevice()
{
}

void ezOpenXRInputDevice::RegisterInputSlots()
{
  for (const Action& action : m_booleanActions)
  {
    RegisterInputSlot(action.sLeftKey, action.sLeftKey, ezInputSlotFlags::IsButton);
    RegisterInputSlot(action.sRightKey, action.sRightKey, ezInputSlotFlags::IsButton);
  }
}

XrResult ezOpenXRInputDevice::UpdateActions()
{
  if (m_session == XR_NULL_HANDLE)
    return XR_SUCCESS;

  EZ_PROFILE_SCOPE("UpdateInputSlotValues");
  const XrFrameState& frameState = m_pOpenXR->m_frameState;

  ezHybridArray<XrActiveActionSet, 1> activeActionSets;
  activeActionSets.PushBack({m_ActionSet, XR_NULL_PATH });

  XrActionsSyncInfo syncInfo{ XR_TYPE_ACTIONS_SYNC_INFO };
  syncInfo.countActiveActionSets = activeActionSets.GetCount();
  syncInfo.activeActionSets = activeActionSets.GetData();
  XrResult res = xrSyncActions(m_session, &syncInfo);
  if (res == XR_SESSION_NOT_FOCUSED)
    return XR_SUCCESS;

  XR_SUCCEED_OR_RETURN_LOG(res);

  for (const Action& action : m_booleanActions)
  {
    XrActionStateBoolean state{ XR_TYPE_ACTION_STATE_BOOLEAN };
    XrActionStateGetInfo getInfo{ XR_TYPE_ACTION_STATE_GET_INFO };
    getInfo.action = action.action;
    getInfo.subactionPath = m_subActionPath[0];
    XR_SUCCEED_OR_RETURN_LOG(xrGetActionStateBoolean(m_session, &getInfo, &state));
    m_InputSlotValues[action.sLeftKey] = state.currentState;

    getInfo.subactionPath = m_subActionPath[1];
    XR_SUCCEED_OR_RETURN_LOG(xrGetActionStateBoolean(m_session, &getInfo, &state));
    m_InputSlotValues[action.sRightKey] = state.currentState;
  }

  auto UpdatePose = [](ezVec3& vPosition, ezQuat& qRotation, bool& m_bIsValid, const XrSpaceLocation& viewInScene)
  {
    if ((viewInScene.locationFlags & (XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)) == (XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT))
    {
      vPosition = ezOpenXR::ConvertPosition(viewInScene.pose.position);
      qRotation = ezOpenXR::ConvertOrientation(viewInScene.pose.orientation);
      m_bIsValid = true;
    }
    else
    {
      m_bIsValid = false;
    }
  };

  for (ezUInt32 uiSide : {0, 1})
  {
    ezXRDeviceState& state = m_DeviceState[uiSide == 0 ? m_iLeftControllerDeviceID : m_iRightControllerDeviceID];
    const XrTime time = frameState.predictedDisplayTime;
    XrSpaceLocation viewInScene = { XR_TYPE_SPACE_LOCATION };
    XR_SUCCEED_OR_RETURN_LOG(xrLocateSpace(m_gripSpace[uiSide], m_pOpenXR->m_sceneSpace, time, &viewInScene));
    UpdatePose(state.m_vGripPosition, state.m_qGripRotation, state.m_bGripPoseIsValid, viewInScene);

    XR_SUCCEED_OR_RETURN_LOG(xrLocateSpace(m_aimSpace[uiSide], m_pOpenXR->m_sceneSpace, time, &viewInScene));
    UpdatePose(state.m_vAimPosition, state.m_qAimRotation, state.m_bAimPoseIsValid, viewInScene);
  }
  return XR_SUCCESS;
}

