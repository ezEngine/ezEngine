#include <OpenXRPlugin/OpenXRPluginPCH.h>

#include <Core/Input/InputManager.h>
#include <Foundation/Profiling/Profiling.h>
#include <OpenXRPlugin/OpenXRDeclarations.h>
#include <OpenXRPlugin/OpenXRInputDevice.h>
#include <OpenXRPlugin/OpenXRSingleton.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezOpenXRInputDevice, 1, ezRTTINoAllocator);
// no properties or message handlers
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


#define XR_Trigger "trigger_value"

#define XR_Select_Click "select_click"
#define XR_Menu_Click "menu_click"
#define XR_Squeeze_Click "squeeze_click"

#define XR_Primary_Analog_Stick_Axis "primary_analog_stick"
#define XR_Primary_Analog_Stick_Click "primary_analog_stick_click"
#define XR_Primary_Analog_Stick_Touch "primary_analog_stick_touch"

#define XR_Secondary_Analog_Stick_Axis "secondary_analog_stick"
#define XR_Secondary_Analog_Stick_Click "secondary_analog_stick_click"
#define XR_Secondary_Analog_Stick_Touch "secondary_analog_stick_touch"

#define XR_Grip_Pose "grip_pose"
#define XR_Aim_Pose "aim_pose"

void ezOpenXRInputDevice::GetDeviceList(ezHybridArray<ezXRDeviceID, 64>& out_devices) const
{
  EZ_ASSERT_DEV(m_pOpenXR->IsInitialized(), "Need to call 'Initialize' first.");
  out_devices.PushBack(0);
  for (ezXRDeviceID i = 0; i < 3; i++)
  {
    if (m_DeviceState[i].m_bDeviceIsConnected)
    {
      out_devices.PushBack(i);
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

const ezXRDeviceState& ezOpenXRInputDevice::GetDeviceState(ezXRDeviceID deviceID) const
{
  EZ_ASSERT_DEV(m_pOpenXR->IsInitialized(), "Need to call 'Initialize' first.");
  EZ_ASSERT_DEV(deviceID < 3 && deviceID >= 0, "Invalid device ID.");
  EZ_ASSERT_DEV(m_DeviceState[deviceID].m_bDeviceIsConnected, "Invalid device ID.");
  return m_DeviceState[deviceID];
}

ezString ezOpenXRInputDevice::GetDeviceName(ezXRDeviceID deviceID) const
{
  EZ_ASSERT_DEV(m_pOpenXR->IsInitialized(), "Need to call 'Initialize' first.");
  EZ_ASSERT_DEV(deviceID < 3 && deviceID >= 0, "Invalid device ID.");
  EZ_ASSERT_DEV(m_DeviceState[deviceID].m_bDeviceIsConnected, "Invalid device ID.");
  return m_sActiveProfile[deviceID];
}

ezBitflags<ezXRDeviceFeatures> ezOpenXRInputDevice::GetDeviceFeatures(ezXRDeviceID deviceID) const
{
  EZ_ASSERT_DEV(m_pOpenXR->IsInitialized(), "Need to call 'Initialize' first.");
  EZ_ASSERT_DEV(deviceID < 3 && deviceID >= 0, "Invalid device ID.");
  EZ_ASSERT_DEV(m_DeviceState[deviceID].m_bDeviceIsConnected, "Invalid device ID.");
  return m_SupportedFeatures[deviceID];
}

ezOpenXRInputDevice::ezOpenXRInputDevice(ezOpenXR* pOpenXR)
  : ezXRInputDevice()
  , m_pOpenXR(pOpenXR)
{
  m_pInstance = m_pOpenXR->m_pInstance;
}

XrResult ezOpenXRInputDevice::CreateActions(XrSession session, XrSpace sceneSpace)
{
  m_pSession = session;

  // HMD is always connected or we wouldn't have been able to create a session.
  m_DeviceState[0] = ezXRDeviceState();
  m_DeviceState[0].m_bDeviceIsConnected = true;
  m_sActiveProfile[0] = "HMD";
  m_SupportedFeatures[0] = ezXRDeviceFeatures::AimPose | ezXRDeviceFeatures::GripPose;

  // Controllers
  for (ezUInt32 uiControllerId : {m_iLeftControllerDeviceID, m_iRightControllerDeviceID})
  {
    m_DeviceState[uiControllerId] = ezXRDeviceState();
    m_sActiveProfile[uiControllerId].Clear();
  }

  XrActionSetCreateInfo actionSetInfo{XR_TYPE_ACTION_SET_CREATE_INFO};
  ezStringUtils::Copy(actionSetInfo.actionSetName, XR_MAX_ACTION_SET_NAME_SIZE, "gameplay");
  ezStringUtils::Copy(actionSetInfo.localizedActionSetName, XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE, "Gameplay");
  actionSetInfo.priority = 0;
  XR_SUCCEED_OR_CLEANUP_LOG(xrCreateActionSet(m_pInstance, &actionSetInfo, &m_pActionSet), DestroyActions);

  m_SubActionPrefix.SetCount(2);
  m_SubActionPrefix[0] = "xr_hand_left_";
  m_SubActionPrefix[1] = "xr_hand_right_";

  m_SubActionPath.SetCount(2);
  m_SubActionPath[0] = CreatePath("/user/hand/left");
  m_SubActionPath[1] = CreatePath("/user/hand/right");

  XrAction Trigger = XR_NULL_HANDLE;
  XrAction SelectClick = XR_NULL_HANDLE;
  XrAction MenuClick = XR_NULL_HANDLE;
  XrAction SqueezeClick = XR_NULL_HANDLE;

  XrAction PrimaryAnalogStickAxis = XR_NULL_HANDLE;
  XrAction PrimaryAnalogStickClick = XR_NULL_HANDLE;
  XrAction PrimaryAnalogStickTouch = XR_NULL_HANDLE;

  XrAction SecondaryAnalogStickAxis = XR_NULL_HANDLE;
  XrAction SecondaryAnalogStickClick = XR_NULL_HANDLE;
  XrAction SecondaryAnalogStickTouch = XR_NULL_HANDLE;

  XrAction GripPose = XR_NULL_HANDLE;
  XrAction AimPose = XR_NULL_HANDLE;

  XR_SUCCEED_OR_CLEANUP_LOG(CreateAction(ezXRDeviceFeatures::Trigger, XR_Trigger, XR_ACTION_TYPE_FLOAT_INPUT, Trigger), DestroyActions);

  XR_SUCCEED_OR_CLEANUP_LOG(CreateAction(ezXRDeviceFeatures::Select, XR_Select_Click, XR_ACTION_TYPE_BOOLEAN_INPUT, SelectClick), DestroyActions);
  XR_SUCCEED_OR_CLEANUP_LOG(CreateAction(ezXRDeviceFeatures::Menu, XR_Menu_Click, XR_ACTION_TYPE_BOOLEAN_INPUT, MenuClick), DestroyActions);
  XR_SUCCEED_OR_CLEANUP_LOG(CreateAction(ezXRDeviceFeatures::Squeeze, XR_Squeeze_Click, XR_ACTION_TYPE_BOOLEAN_INPUT, SqueezeClick), DestroyActions);

  XR_SUCCEED_OR_CLEANUP_LOG(
    CreateAction(ezXRDeviceFeatures::PrimaryAnalogStick, XR_Primary_Analog_Stick_Axis, XR_ACTION_TYPE_VECTOR2F_INPUT, PrimaryAnalogStickAxis),
    DestroyActions);
  XR_SUCCEED_OR_CLEANUP_LOG(
    CreateAction(ezXRDeviceFeatures::PrimaryAnalogStickClick, XR_Primary_Analog_Stick_Click, XR_ACTION_TYPE_BOOLEAN_INPUT, PrimaryAnalogStickClick),
    DestroyActions);
  XR_SUCCEED_OR_CLEANUP_LOG(
    CreateAction(ezXRDeviceFeatures::PrimaryAnalogStickTouch, XR_Primary_Analog_Stick_Touch, XR_ACTION_TYPE_BOOLEAN_INPUT, PrimaryAnalogStickTouch),
    DestroyActions);

  XR_SUCCEED_OR_CLEANUP_LOG(
    CreateAction(ezXRDeviceFeatures::SecondaryAnalogStick, XR_Secondary_Analog_Stick_Axis, XR_ACTION_TYPE_VECTOR2F_INPUT, SecondaryAnalogStickAxis),
    DestroyActions);
  XR_SUCCEED_OR_CLEANUP_LOG(CreateAction(ezXRDeviceFeatures::SecondaryAnalogStickClick, XR_Secondary_Analog_Stick_Click, XR_ACTION_TYPE_BOOLEAN_INPUT,
                              SecondaryAnalogStickClick),
    DestroyActions);
  XR_SUCCEED_OR_CLEANUP_LOG(CreateAction(ezXRDeviceFeatures::SecondaryAnalogStickTouch, XR_Secondary_Analog_Stick_Touch, XR_ACTION_TYPE_BOOLEAN_INPUT,
                              SecondaryAnalogStickTouch),
    DestroyActions);

  XR_SUCCEED_OR_CLEANUP_LOG(CreateAction(ezXRDeviceFeatures::GripPose, XR_Grip_Pose, XR_ACTION_TYPE_POSE_INPUT, GripPose), DestroyActions);
  XR_SUCCEED_OR_CLEANUP_LOG(CreateAction(ezXRDeviceFeatures::AimPose, XR_Aim_Pose, XR_ACTION_TYPE_POSE_INPUT, AimPose), DestroyActions);

  Bind simpleController[] = {
    {SelectClick, "/user/hand/left/input/select"},
    {MenuClick, "/user/hand/left/input/menu"},
    {GripPose, "/user/hand/left/input/grip"},
    {AimPose, "/user/hand/left/input/aim"},

    {SelectClick, "/user/hand/right/input/select"},
    {MenuClick, "/user/hand/right/input/menu"},
    {GripPose, "/user/hand/right/input/grip"},
    {AimPose, "/user/hand/right/input/aim"},
  };
  SuggestInteractionProfileBindings("/interaction_profiles/khr/simple_controller", "Simple Controller", simpleController);

  Bind motionController[] = {
    {Trigger, "/user/hand/left/input/trigger"},
    {SelectClick, "/user/hand/left/input/trigger"},
    {MenuClick, "/user/hand/left/input/menu"},
    {SqueezeClick, "/user/hand/left/input/squeeze"},
    {PrimaryAnalogStickAxis, "/user/hand/left/input/thumbstick"},
    {PrimaryAnalogStickClick, "/user/hand/left/input/thumbstick"},
    {SecondaryAnalogStickAxis, "/user/hand/left/input/trackpad"},
    {SecondaryAnalogStickClick, "/user/hand/left/input/trackpad/click"},
    {SecondaryAnalogStickTouch, "/user/hand/left/input/trackpad/touch"},
    {GripPose, "/user/hand/left/input/grip"},
    {AimPose, "/user/hand/left/input/aim"},

    {Trigger, "/user/hand/right/input/trigger"},
    {SelectClick, "/user/hand/right/input/trigger"},
    {MenuClick, "/user/hand/right/input/menu"},
    {SqueezeClick, "/user/hand/right/input/squeeze"},
    {PrimaryAnalogStickAxis, "/user/hand/right/input/thumbstick"},
    {PrimaryAnalogStickClick, "/user/hand/right/input/thumbstick"},
    {SecondaryAnalogStickAxis, "/user/hand/right/input/trackpad"},
    {SecondaryAnalogStickClick, "/user/hand/right/input/trackpad/click"},
    {SecondaryAnalogStickTouch, "/user/hand/right/input/trackpad/touch"},
    {GripPose, "/user/hand/right/input/grip"},
    {AimPose, "/user/hand/right/input/aim"},
  };
  SuggestInteractionProfileBindings("/interaction_profiles/microsoft/motion_controller", "Mixed Reality Motion Controller", motionController);

  if (m_pOpenXR->m_Extensions.m_bHandInteraction)
  {
    Bind handInteraction[] = {
      {SelectClick, "/user/hand/left/input/select"},
      {GripPose, "/user/hand/left/input/grip"},
      {AimPose, "/user/hand/left/input/aim"},

      {SelectClick, "/user/hand/right/input/select"},
      {GripPose, "/user/hand/right/input/grip"},
      {AimPose, "/user/hand/right/input/aim"},
    };
    SuggestInteractionProfileBindings("/interaction_profiles/microsoft/hand_interaction", "Hand Interaction", handInteraction);
  }


  XrActionSpaceCreateInfo spaceCreateInfo{XR_TYPE_ACTION_SPACE_CREATE_INFO};
  spaceCreateInfo.poseInActionSpace = m_pOpenXR->ConvertTransform(ezTransform::MakeIdentity());
  for (ezUInt32 uiSide : {0, 1})
  {
    spaceCreateInfo.subactionPath = m_SubActionPath[uiSide];
    spaceCreateInfo.action = GripPose;
    XR_SUCCEED_OR_CLEANUP_LOG(xrCreateActionSpace(m_pSession, &spaceCreateInfo, &m_gripSpace[uiSide]), DestroyActions);

    spaceCreateInfo.action = AimPose;
    XR_SUCCEED_OR_CLEANUP_LOG(xrCreateActionSpace(m_pSession, &spaceCreateInfo, &m_aimSpace[uiSide]), DestroyActions);
  }
  return XR_SUCCESS;
}

void ezOpenXRInputDevice::DestroyActions()
{
  for (Action& action : m_BooleanActions)
  {
    XR_LOG_ERROR(xrDestroyAction(action.m_Action));
  }
  m_BooleanActions.Clear();

  for (Action& action : m_FloatActions)
  {
    XR_LOG_ERROR(xrDestroyAction(action.m_Action));
  }
  m_FloatActions.Clear();

  for (Vec2Action& action : m_Vec2Actions)
  {
    XR_LOG_ERROR(xrDestroyAction(action.m_Action));
  }
  m_Vec2Actions.Clear();

  for (Action& action : m_PoseActions)
  {
    XR_LOG_ERROR(xrDestroyAction(action.m_Action));
  }
  m_PoseActions.Clear();

  if (m_pActionSet)
  {
    XR_LOG_ERROR(xrDestroyActionSet(m_pActionSet));
    m_pActionSet = XR_NULL_HANDLE;
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

  for (ezUInt32 i = 0; i < 3; i++)
  {
    m_sActiveProfile[i].Clear();
    m_SupportedFeatures[i].Clear();
    m_DeviceState[i].m_bDeviceIsConnected = false;
  }
}

XrPath ezOpenXRInputDevice::CreatePath(const char* szPath)
{
  XrInstance instance = m_pOpenXR->m_pInstance;

  XrPath path;
  if (xrStringToPath(instance, szPath, &path) != XR_SUCCESS)
  {
    ezLog::Error("OpenXR path conversion failure: {0}", szPath);
  }
  return path;
}

XrResult ezOpenXRInputDevice::CreateAction(ezXRDeviceFeatures::Enum feature, const char* actionName, XrActionType actionType, XrAction& out_action)
{
  XrActionCreateInfo actionCreateInfo{XR_TYPE_ACTION_CREATE_INFO};
  actionCreateInfo.actionType = actionType;
  actionCreateInfo.countSubactionPaths = m_SubActionPath.GetCount();
  actionCreateInfo.subactionPaths = m_SubActionPath.GetData();
  ezStringUtils::Copy(actionCreateInfo.actionName, XR_MAX_ACTION_NAME_SIZE, actionName);
  ezStringUtils::Copy(actionCreateInfo.localizedActionName, XR_MAX_LOCALIZED_ACTION_NAME_SIZE, actionName);

  XR_SUCCEED_OR_CLEANUP_LOG(xrCreateAction(m_pActionSet, &actionCreateInfo, &out_action), voidFunction);

  ezStringBuilder sLeft(m_SubActionPrefix[0], actionName);
  ezStringBuilder sRight(m_SubActionPrefix[1], actionName);

  switch (actionType)
  {
    case XR_ACTION_TYPE_BOOLEAN_INPUT:
      m_BooleanActions.PushBack({feature, out_action, sLeft, sRight});
      break;
    case XR_ACTION_TYPE_FLOAT_INPUT:
      m_FloatActions.PushBack({feature, out_action, sLeft, sRight});
      break;
    case XR_ACTION_TYPE_VECTOR2F_INPUT:
      m_Vec2Actions.PushBack(Vec2Action(feature, out_action, sLeft, sRight));
      break;
    case XR_ACTION_TYPE_POSE_INPUT:
      m_PoseActions.PushBack({feature, out_action, sLeft, sRight});
      break;
    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }

  return XR_SUCCESS;
}

XrResult ezOpenXRInputDevice::SuggestInteractionProfileBindings(const char* szInteractionProfile, const char* szNiceName, ezArrayPtr<Bind> bindings)
{
  XrInstance instance = m_pOpenXR->m_pInstance;

  XrPath InteractionProfile = CreatePath(szInteractionProfile);

  ezDynamicArray<XrActionSuggestedBinding> xrBindings;
  xrBindings.Reserve(bindings.GetCount());
  for (const Bind& binding : bindings)
  {
    xrBindings.PushBack({binding.action, CreatePath(binding.szPath)});
  }


  XrInteractionProfileSuggestedBinding profileBindings{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
  profileBindings.interactionProfile = InteractionProfile;
  profileBindings.suggestedBindings = xrBindings.GetData();
  profileBindings.countSuggestedBindings = xrBindings.GetCount();
  XR_SUCCEED_OR_RETURN_LOG(xrSuggestInteractionProfileBindings(instance, &profileBindings));

  m_InteractionProfileToNiceName[InteractionProfile] = szNiceName;

  return XR_SUCCESS;
}

XrResult ezOpenXRInputDevice::AttachSessionActionSets(XrSession session)
{
  m_pSession = session;
  XrSessionActionSetsAttachInfo attachInfo{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
  ezHybridArray<XrActionSet, 1> actionSets;
  actionSets.PushBack(m_pActionSet);

  attachInfo.countActionSets = actionSets.GetCount();
  attachInfo.actionSets = actionSets.GetData();
  XR_SUCCEED_OR_RETURN_LOG(xrAttachSessionActionSets(session, &attachInfo));

  return XR_SUCCESS;
}

XrResult ezOpenXRInputDevice::UpdateCurrentInteractionProfile()
{
  // This function is triggered by the XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED event.
  // Unfortunately it does not seem to provide any info in regards to what top level path is affected
  // so we check both controllers again.
  auto GetActiveControllerProfile = [this](ezUInt32 uiSide) -> XrPath
  {
    XrInteractionProfileState state{XR_TYPE_INTERACTION_PROFILE_STATE};
    XrResult res = xrGetCurrentInteractionProfile(m_pSession, m_SubActionPath[uiSide], &state);
    if (res == XR_SUCCESS)
    {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
      if (state.interactionProfile != XR_NULL_PATH && !m_InteractionProfileToNiceName.Contains(state.interactionProfile))
      {
        char buffer[256];
        ezUInt32 temp;
        xrPathToString(m_pInstance, state.interactionProfile, 256, &temp, buffer);
        EZ_REPORT_FAILURE("Unknown interaction profile was selected by the OpenXR runtime: '{}'", buffer);
      }
#endif
      return state.interactionProfile;
    }
    return XR_NULL_PATH;
  };

  for (ezUInt32 uiSide : {0, 1})
  {
    const ezUInt32 uiControllerId = uiSide == 0 ? m_iLeftControllerDeviceID : m_iRightControllerDeviceID;
    XrPath path = GetActiveControllerProfile(uiSide);
    m_sActiveProfile[uiControllerId] = m_InteractionProfileToNiceName[path];
  }

  UpdateActions();

  return XR_SUCCESS;
}

void ezOpenXRInputDevice::InitializeDevice() {}

void ezOpenXRInputDevice::RegisterInputSlots()
{
  for (const Action& action : m_BooleanActions)
  {
    for (ezUInt32 uiSide : {0, 1})
    {
      RegisterInputSlot(action.m_sKey[uiSide], action.m_sKey[uiSide], ezInputSlotFlags::IsButton);
    }
  }
  for (const Action& action : m_FloatActions)
  {
    for (ezUInt32 uiSide : {0, 1})
    {
      RegisterInputSlot(action.m_sKey[uiSide], action.m_sKey[uiSide], ezInputSlotFlags::IsAnalogTrigger);
    }
  }
  for (const Vec2Action& action : m_Vec2Actions)
  {
    for (ezUInt32 uiSide : {0, 1})
    {
      RegisterInputSlot(action.m_sKey_negx[uiSide], action.m_sKey_negx[uiSide], ezInputSlotFlags::IsAnalogStick);
      RegisterInputSlot(action.m_sKey_posx[uiSide], action.m_sKey_posx[uiSide], ezInputSlotFlags::IsAnalogStick);
      RegisterInputSlot(action.m_sKey_negy[uiSide], action.m_sKey_negy[uiSide], ezInputSlotFlags::IsAnalogStick);
      RegisterInputSlot(action.m_sKey_posy[uiSide], action.m_sKey_posy[uiSide], ezInputSlotFlags::IsAnalogStick);
    }
  }
}

XrResult ezOpenXRInputDevice::UpdateActions()
{
  if (m_pSession == XR_NULL_HANDLE)
    return XR_SUCCESS;

  EZ_PROFILE_SCOPE("UpdateActions");
  const XrFrameState& frameState = m_pOpenXR->m_FrameState;

  ezHybridArray<XrActiveActionSet, 1> activeActionSets;
  activeActionSets.PushBack({m_pActionSet, XR_NULL_PATH});

  XrActionsSyncInfo syncInfo{XR_TYPE_ACTIONS_SYNC_INFO};
  syncInfo.countActiveActionSets = activeActionSets.GetCount();
  syncInfo.activeActionSets = activeActionSets.GetData();
  XrResult res = xrSyncActions(m_pSession, &syncInfo);
  if (res == XR_SESSION_NOT_FOCUSED)
    return XR_SUCCESS;

  XR_SUCCEED_OR_RETURN_LOG(res);

  for (ezUInt32 uiSide : {0, 1})
  {
    const ezUInt32 uiControllerId = uiSide == 0 ? m_iLeftControllerDeviceID : m_iRightControllerDeviceID;

    for (const Action& action : m_PoseActions)
    {
      XrActionStatePose state{XR_TYPE_ACTION_STATE_POSE};
      XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO};
      getInfo.action = action.m_Action;
      getInfo.subactionPath = m_SubActionPath[uiSide];
      XR_SUCCEED_OR_RETURN_LOG(xrGetActionStatePose(m_pSession, &getInfo, &state));
      m_SupportedFeatures[uiControllerId].AddOrRemove(action.m_Feature, state.isActive);
    }

    for (const Action& action : m_BooleanActions)
    {
      XrActionStateBoolean state{XR_TYPE_ACTION_STATE_BOOLEAN};
      XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO};
      getInfo.action = action.m_Action;
      getInfo.subactionPath = m_SubActionPath[uiSide];
      XR_SUCCEED_OR_RETURN_LOG(xrGetActionStateBoolean(m_pSession, &getInfo, &state));
      m_InputSlotValues[action.m_sKey[uiSide]] = state.currentState ? 1.0f : 0.0f;
      m_SupportedFeatures[uiControllerId].AddOrRemove(action.m_Feature, state.isActive);
    }

    for (const Action& action : m_FloatActions)
    {
      XrActionStateFloat state{XR_TYPE_ACTION_STATE_FLOAT};
      XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO};
      getInfo.action = action.m_Action;
      getInfo.subactionPath = m_SubActionPath[uiSide];
      XR_SUCCEED_OR_RETURN_LOG(xrGetActionStateFloat(m_pSession, &getInfo, &state));
      m_InputSlotValues[action.m_sKey[uiSide]] = state.currentState;
      m_SupportedFeatures[uiControllerId].AddOrRemove(action.m_Feature, state.isActive);
    }

    for (const Vec2Action& action : m_Vec2Actions)
    {
      XrActionStateVector2f state{XR_TYPE_ACTION_STATE_VECTOR2F};
      XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO};
      getInfo.action = action.m_Action;
      getInfo.subactionPath = m_SubActionPath[uiSide];
      XR_SUCCEED_OR_RETURN_LOG(xrGetActionStateVector2f(m_pSession, &getInfo, &state));

      m_InputSlotValues[action.m_sKey_negx[uiSide]] = state.currentState.x < 0 ? -state.currentState.x : 0.0f;
      m_InputSlotValues[action.m_sKey_posx[uiSide]] = state.currentState.x > 0 ? state.currentState.x : 0.0f;
      m_InputSlotValues[action.m_sKey_negy[uiSide]] = state.currentState.y < 0 ? -state.currentState.y : 0.0f;
      m_InputSlotValues[action.m_sKey_posy[uiSide]] = state.currentState.y > 0 ? state.currentState.y : 0.0f;
      m_SupportedFeatures[uiControllerId].AddOrRemove(action.m_Feature, state.isActive);
    }

    auto UpdatePose = [](ezVec3& vPosition, ezQuat& qRotation, bool& m_bIsValid, const XrSpaceLocation& viewInScene)
    {
      if ((viewInScene.locationFlags & (XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT)) ==
          (XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT))
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

    XrInteractionProfileState state2{XR_TYPE_INTERACTION_PROFILE_STATE};
    XrResult res2 = xrGetCurrentInteractionProfile(m_pSession, m_SubActionPath[uiSide], &state2);

    ezXRDeviceState& state = m_DeviceState[uiControllerId];
    const XrTime time = frameState.predictedDisplayTime;
    XrSpaceLocation viewInScene = {XR_TYPE_SPACE_LOCATION};
    XR_SUCCEED_OR_RETURN_LOG(xrLocateSpace(m_gripSpace[uiSide], m_pOpenXR->GetBaseSpace(), time, &viewInScene));
    UpdatePose(state.m_vGripPosition, state.m_qGripRotation, state.m_bGripPoseIsValid, viewInScene);

    XR_SUCCEED_OR_RETURN_LOG(xrLocateSpace(m_aimSpace[uiSide], m_pOpenXR->GetBaseSpace(), time, &viewInScene));
    UpdatePose(state.m_vAimPosition, state.m_qAimRotation, state.m_bAimPoseIsValid, viewInScene);
  }

  UpdateControllerState();
  return XR_SUCCESS;
}

void ezOpenXRInputDevice::UpdateControllerState()
{
  for (ezUInt32 uiSide : {0, 1})
  {
    const ezUInt32 uiControllerId = uiSide == 0 ? m_iLeftControllerDeviceID : m_iRightControllerDeviceID;
    const bool bDeviceConnected = m_SupportedFeatures[uiControllerId].IsSet(ezXRDeviceFeatures::AimPose);

    if (!m_DeviceState[uiControllerId].m_bDeviceIsConnected && bDeviceConnected)
    {
      // Connected
      m_DeviceState[uiControllerId].m_bDeviceIsConnected = true;

      ezXRDeviceEventData data;
      data.m_Type = ezXRDeviceEventData::Type::DeviceAdded;
      data.uiDeviceID = uiControllerId;
      m_InputEvents.Broadcast(data);
    }
    else if (m_DeviceState[uiControllerId].m_bDeviceIsConnected && !bDeviceConnected)
    {
      // Disconnected
      m_DeviceState[uiControllerId] = ezXRDeviceState();
      m_SupportedFeatures[uiControllerId] = ezXRDeviceFeatures::None;

      ezXRDeviceEventData data;
      data.m_Type = ezXRDeviceEventData::Type::DeviceRemoved;
      data.uiDeviceID = uiControllerId;
      m_InputEvents.Broadcast(data);
    }
  }
}

ezOpenXRInputDevice::Vec2Action::Vec2Action(ezXRDeviceFeatures::Enum feature, XrAction pAction, ezStringView sLeft, ezStringView sRight)
{
  m_Feature = feature;
  m_Action = pAction;

  ezStringView sides[2] = {sLeft, sRight};
  for (ezUInt32 uiSide : {0, 1})
  {
    ezStringBuilder temp = sides[uiSide];
    temp.Append("_negx");
    m_sKey_negx[uiSide] = temp;

    temp.Shrink(0, 5);
    temp.Append("_posx");
    m_sKey_posx[uiSide] = temp;

    temp.Shrink(0, 5);
    temp.Append("_negy");
    m_sKey_negy[uiSide] = temp;

    temp.Shrink(0, 5);
    temp.Append("_posy");
    m_sKey_posy[uiSide] = temp;
  }
}
