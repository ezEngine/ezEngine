#pragma once

#include <Core/CoreDLL.h>
#include <Core/World/CoordinateSystem.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>

/// \brief Specifies in which mode this camera is configured.
struct EZ_CORE_DLL ezCameraMode
{
  using StorageType = ezInt8;

  enum Enum
  {
    None,                 ///< Not initialized
    PerspectiveFixedFovX, ///< Perspective camera, the fov for X is fixed, Y depends on the aspect ratio
    PerspectiveFixedFovY, ///< Perspective camera, the fov for Y is fixed, X depends on the aspect ratio
    OrthoFixedWidth,      ///< Orthographic camera, the width is fixed, the height depends on the aspect ratio
    OrthoFixedHeight,     ///< Orthographic camera, the height is fixed, the width depends on the aspect ratio
    Stereo,               ///< A stereo camera with view/projection matrices provided by an HMD.
    Default = PerspectiveFixedFovY
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezCameraMode);

/// \brief Determines left or right eye of a stereo camera.
///
/// As a general rule, this parameter does not matter for mono-scopic cameras and will always return the same value.
enum class ezCameraEye
{
  Left,
  Right,
  // Two eyes should be enough for everyone.
};

/// \brief A camera class that stores the orientation and some basic camera settings.
class EZ_CORE_DLL ezCamera
{
public:
  ezCamera();

  /// \brief Allows to specify a different coordinate system in which the camera input and output coordinates are given.
  ///
  /// The default in z is forward = PositiveX, right = PositiveY, Up = PositiveZ.
  void SetCoordinateSystem(ezBasisAxis::Enum forwardAxis, ezBasisAxis::Enum rightAxis, ezBasisAxis::Enum axis);

  /// \brief Allows to specify a full ezCoordinateSystemProvider to determine forward/right/up vectors for camera movement
  void SetCoordinateSystem(const ezSharedPtr<ezCoordinateSystemProvider>& pProvider);

  /// \brief Returns the position of the camera that should be used for rendering etc.
  ezVec3 GetPosition(ezCameraEye eye = ezCameraEye::Left) const;

  /// \brief Returns the forwards vector that should be used for rendering etc.
  ezVec3 GetDirForwards(ezCameraEye eye = ezCameraEye::Left) const;

  /// \brief Returns the up vector that should be used for rendering etc.
  ezVec3 GetDirUp(ezCameraEye eye = ezCameraEye::Left) const;

  /// \brief Returns the right vector that should be used for rendering etc.
  ezVec3 GetDirRight(ezCameraEye eye = ezCameraEye::Left) const;

  /// \brief Returns the horizontal FOV.
  ///
  /// Works only with ezCameraMode::PerspectiveFixedFovX and ezCameraMode::PerspectiveFixedFovY
  ezAngle GetFovX(float fAspectRatioWidthDivHeight) const;

  /// \brief Returns the vertical FOV.
  ///
  /// Works only with ezCameraMode::PerspectiveFixedFovX and ezCameraMode::PerspectiveFixedFovY
  ezAngle GetFovY(float fAspectRatioWidthDivHeight) const;

  /// \brief Returns the horizontal dimension for an orthographic view.
  ///
  /// Works only with ezCameraMode::OrthoFixedWidth and ezCameraMode::OrthoFixedWidth
  float GetDimensionX(float fAspectRatioWidthDivHeight) const;

  /// \brief Returns the vertical dimension for an orthographic view.
  ///
  /// Works only with ezCameraMode::OrthoFixedWidth and ezCameraMode::OrthoFixedWidth
  float GetDimensionY(float fAspectRatioWidthDivHeight) const;

  /// \brief Returns the average camera position.
  ///
  /// For all cameras execpt Stereo cameras this is identical to GetPosition()
  ezVec3 GetCenterPosition() const;

  /// \brief Returns the average forwards vector.
  ///
  /// For all cameras execpt Stereo cameras this is identical to GetDirForwards()
  ezVec3 GetCenterDirForwards() const;

  /// \brief Returns the average up vector.
  ///
  /// For all cameras execpt Stereo cameras this is identical to GetDirUp()
  ezVec3 GetCenterDirUp() const;

  /// \brief Returns the average right vector.
  ///
  /// For all cameras execpt Stereo cameras this is identical to GetDirRight()
  ezVec3 GetCenterDirRight() const;

  /// \brief Returns the near plane distance that was passed to SetCameraProjectionAndMode().
  float GetNearPlane() const;

  /// \brief Returns the far plane distance that was passed to SetCameraProjectionAndMode().
  float GetFarPlane() const;

  /// \brief Specifies the mode and the projection settings that this camera uses.
  ///
  /// \param fFovOrDim
  ///   Fov X/Y in degree or width/height (depending on Mode).
  void SetCameraMode(ezCameraMode::Enum mode, float fFovOrDim, float fNearPlane, float fFarPlane);

  /// Sets the camera mode to stereo and specifies projection matrices directly.
  ///
  /// \param fAspectRatio
  ///   These stereo projection matrices will only be returned by getProjectionMatrix for the given aspectRatio.
  void SetStereoProjection(const ezMat4& mProjectionLeftEye, const ezMat4& mProjectionRightEye, float fAspectRatioWidthDivHeight);

  /// \brief Returns the fFovOrDim parameter that was passed to SetCameraProjectionAndMode().
  float GetFovOrDim() const;

  /// \brief Returns the current camera mode.
  ezCameraMode::Enum GetCameraMode() const;

  bool IsPerspective() const;

  bool IsOrthographic() const;

  /// \brief Whether this is a stereoscopic camera.
  bool IsStereoscopic() const;

  /// \brief Sets the view matrix directly.
  ///
  /// Works with all camera types. Position- and direction- getter/setter will work as usual.
  void SetViewMatrix(const ezMat4& mLookAtMatrix, ezCameraEye eye = ezCameraEye::Left);

  /// \brief Repositions the camera such that it looks at the given target position.
  ///
  /// Not supported for stereo cameras.
  void LookAt(const ezVec3& vCameraPos, const ezVec3& vTargetPos, const ezVec3& vUp);

  /// \brief Moves the camera in its local space along the forward/right/up directions of the coordinate system.
  ///
  /// Not supported for stereo cameras.
  void MoveLocally(float fForward, float fRight, float fUp);

  /// \brief Moves the camera in global space along the forward/right/up directions of the coordinate system.
  ///
  /// Not supported for stereo cameras.
  void MoveGlobally(float fForward, float fRight, float fUp);

  /// \brief Rotates the camera around the forward, right and up axis in its own local space.
  ///
  /// Rotate around \a rightAxis for looking up/down. \forwardAxis is roll. For turning left/right use RotateGlobally().
  /// Not supported for stereo cameras.
  void RotateLocally(ezAngle forwardAxis, ezAngle rightAxis, ezAngle axis);

  /// \brief Rotates the camera around the forward, right and up axis of the coordinate system in global space.
  ///
  /// Rotate around Z for turning the camera left/right.
  /// Not supported for stereo cameras.
  void RotateGlobally(ezAngle forwardAxis, ezAngle rightAxis, ezAngle axis);

  /// \brief Returns the view matrix for the given eye.
  ///
  /// \note The view matrix is given in OpenGL convention.
  const ezMat4& GetViewMatrix(ezCameraEye eye = ezCameraEye::Left) const;

  /// \brief Calculates the projection matrix from the current camera properties and stores it in out_projectionMatrix.
  ///
  /// If the camera is stereo and the given aspect ratio is close to the aspect ratio passed in SetStereoProjection,
  /// the matrix set in SetStereoProjection will be used.
  void GetProjectionMatrix(float fAspectRatioWidthDivHeight, ezMat4& out_mProjectionMatrix, ezCameraEye eye = ezCameraEye::Left,
    ezClipSpaceDepthRange::Enum depthRange = ezClipSpaceDepthRange::Default) const;

  float GetExposure() const;

  void SetExposure(float fExposure);

  /// \brief Returns a counter that is increased every time the camera settings are modified.
  ///
  /// The camera settings are used to compute the projection matrix. This counter can be used to determine whether the projection matrix
  /// has changed and thus whether cached values need to be updated.
  ezUInt32 GetSettingsModificationCounter() const { return m_uiSettingsModificationCounter; }

  /// \brief Returns a counter that is increased every time the camera orientation is modified.
  ///
  /// The camera orientation is used to compute the view matrix. This counter can be used to determine whether the view matrix
  /// has changed and thus whether cached values need to be updated.
  ezUInt32 GetOrientationModificationCounter() const { return m_uiOrientationModificationCounter; }

private:
  /// \brief This function is called whenever the camera position or rotation changed.
  void CameraOrientationChanged() { ++m_uiOrientationModificationCounter; }

  /// \brief This function is called when the camera mode or projection changes (e.g. SetCameraProjectionAndMode was called).
  void CameraSettingsChanged();

  /// \brief This function is called by RotateLocally() and RotateGlobally() BEFORE the values are applied,
  /// and allows to adjust them (e.g. for limiting how far the camera can rotate).
  void ClampRotationAngles(bool bLocalSpace, ezAngle& forwardAxis, ezAngle& rightAxis, ezAngle& upAxis);

  ezVec3 InternalGetPosition(ezCameraEye eye = ezCameraEye::Left) const;
  ezVec3 InternalGetDirForwards(ezCameraEye eye = ezCameraEye::Left) const;
  ezVec3 InternalGetDirUp(ezCameraEye eye = ezCameraEye::Left) const;
  ezVec3 InternalGetDirRight(ezCameraEye eye = ezCameraEye::Left) const;

  float m_fNearPlane = 0.1f;
  float m_fFarPlane = 1000.0f;

  ezCameraMode::Enum m_Mode = ezCameraMode::None;

  float m_fFovOrDim = 90.0f;

  float m_fExposure = 1.0f;

  ezVec3 m_vCameraPosition[2];
  ezMat4 m_mViewMatrix[2];

  /// If the camera mode is stereo and the aspect ratio given in getProjectio is close to this value, one of the stereo projection matrices
  /// is returned.
  float m_fAspectOfPrecomputedStereoProjection = -1.0;
  ezMat4 m_mStereoProjectionMatrix[2];

  ezUInt32 m_uiSettingsModificationCounter = 0;
  ezUInt32 m_uiOrientationModificationCounter = 0;

  ezSharedPtr<ezCoordinateSystemProvider> m_pCoordinateSystem;

  ezVec3 MapExternalToInternal(const ezVec3& v) const;
  ezVec3 MapInternalToExternal(const ezVec3& v) const;
};


#include <Core/Graphics/Implementation/Camera_inl.h>
