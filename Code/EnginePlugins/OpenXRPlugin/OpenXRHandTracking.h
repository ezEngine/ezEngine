#pragma once

#include <Foundation/Configuration/Singleton.h>
#include <GameEngine/XR/XRHandTrackingInterface.h>
#include <OpenXRPlugin/Basics.h>
#include <OpenXRPlugin/OpenXRIncludes.h>

class ezOpenXR;

class EZ_OPENXRPLUGIN_DLL ezOpenXRHandTracking : public ezXRHandTrackingInterface
{
  EZ_DECLARE_SINGLETON_OF_INTERFACE(ezOpenXRHandTracking, ezXRHandTrackingInterface);

public:
  static bool IsHandTrackingSupported(ezOpenXR* pOpenXR);

public:
  ezOpenXRHandTracking(ezOpenXR* pOpenXR);
  ~ezOpenXRHandTracking();

  HandPartTrackingState TryGetBoneTransforms(
    ezEnum<ezXRHand> hand, ezEnum<ezXRHandPart> handPart, ezEnum<ezXRTransformSpace> space, ezDynamicArray<ezXRHandBone>& out_bones) override;

  void UpdateJointTransforms();

private:
  friend class ezOpenXR;

  struct JointData
  {
    EZ_DECLARE_POD_TYPE();
    ezXRHandBone m_Bone;
    bool m_bValid;
  };

  ezOpenXR* m_pOpenXR = nullptr;
  XrHandTrackerEXT m_HandTracker[2] = { XR_NULL_HANDLE, XR_NULL_HANDLE };
  XrHandJointLocationEXT m_JointLocations[2][XR_HAND_JOINT_COUNT_EXT];
  XrHandJointVelocityEXT m_JointVelocities[2][XR_HAND_JOINT_COUNT_EXT];
  XrHandJointLocationsEXT m_Locations[2]{ XR_TYPE_HAND_JOINT_LOCATIONS_EXT };
  XrHandJointVelocitiesEXT m_Velocities[2]{ XR_TYPE_HAND_JOINT_VELOCITIES_EXT };

  ezStaticArray<JointData, XR_HAND_JOINT_LITTLE_TIP_EXT + 1> m_JointData[2];
  ezStaticArray<ezUInt32, 6> m_HandParts[ezXRHandPart::Little + 1];
};
