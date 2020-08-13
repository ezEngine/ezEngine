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
    XrSpace m_Space;
    ezXRHandBone m_Bone;
    bool m_bValid = false;
  };

  ezOpenXR* m_pOpenXR = nullptr;
#ifdef BUILDSYSTEM_ENABLE_OPENXR_PREVIEW_SUPPORT
  XrHandTrackerMSFT m_HandTracker[2] = {XR_NULL_HANDLE, XR_NULL_HANDLE};
  ezStaticArray<JointData, XR_HAND_JOINT_LITTLE_TIP_MSFT + 1> m_JointData[2];
  ezStaticArray<ezUInt32, 6> m_HandParts[ezXRHandPart::Little + 1];
#endif
};
