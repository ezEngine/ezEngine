#pragma once

#include <OpenXRPlugin/Basics.h>
#include <OpenXRPlugin/OpenXRIncludes.h>

#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/Math/Vec3.h>

/// \brief Helper functions to convert between ezEngine and OpenXR types.
class EZ_OPENXRPLUGIN_DLL ezOpenXRConversionUtils
{
public:
  static XrPosef ConvertTransform(const ezTransform& tr);
  static XrQuaternionf ConvertOrientation(const ezQuat& q);
  static XrVector3f ConvertPosition(const ezVec3& vPos);
  static ezQuat ConvertOrientation(const XrQuaternionf& q);
  static ezVec3 ConvertPosition(const XrVector3f& pos);
  static ezMat4 ConvertPoseToMatrix(const XrPosef& pose);
};

#include <OpenXRPlugin/Utils/Implementation/OpenXRConversionUtils.inl.h>
