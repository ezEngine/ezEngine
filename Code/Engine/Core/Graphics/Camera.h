#pragma once

#include <Core/Basics.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Reflection/Reflection.h>

/// \brief Specifies in which mode this camera is configured.
struct EZ_CORE_DLL ezCameraMode
{
  typedef ezInt8 StorageType;

  enum Enum
  {
    None,                   ///< Not initialized
    PerspectiveFixedFovX,   ///< Perspective camera, the fov for X is fixed, Y depends on the aspect ratio
    PerspectiveFixedFovY,   ///< Perspective camera, the fov for Y is fixed, X depends on the aspect ratio
    OrthoFixedWidth,        ///< Orthographic camera, the width is fixed, the height depends on the aspect ratio
    OrthoFixedHeight,       ///< Orthographic camera, the height is fixed, the width depends on the aspect ratio
    Default = PerspectiveFixedFovY
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezCameraMode);

/// \brief A camera class that stores the orientation and some basic camera settings.
class EZ_CORE_DLL ezCamera
{
public:
  ezCamera();
  virtual ~ezCamera() { }

  /// \brief Returns the position of the camera that should be used for rendering etc.
  ///
  /// Override this to implement different camera positions for stereo rendering,
  /// or to implement a camera shake effect, etc.
  virtual ezVec3 GetPosition() const;

  /// \brief Returns the forwards vector that should be used for rendering etc.
  virtual const ezVec3& GetDirForwards() const;

  /// \brief Returns the up vector that should be used for rendering etc.
  virtual const ezVec3& GetDirUp() const;

  /// \brief Returns the right vector that should be used for rendering etc.
  virtual const ezVec3& GetDirRight() const;

  /// \brief Returns the horizontal FOV.
  ezAngle GetFovX(float fAspectRatioWidthDivHeight) const;

  /// \brief Returns the vertical FOV.
  ezAngle GetFovY(float fAspectRatioWidthDivHeight) const;

  /// \brief Returns the horizontal dimension for an orthographic view.
  float GetDimensionX(float fAspectRatioWidthDivHeight) const;

  /// \brief Returns the vertical dimension for an orthographic view.
  float GetDimensionY(float fAspectRatioWidthDivHeight) const;

  /// \brief Returns the actual camera position.
  const ezVec3& GetCenterPosition() const;

  /// \brief Returns the actual forwards vector.
  const ezVec3& GetCenterDirForwards() const;

  /// \brief Returns the actual up vector.
  const ezVec3& GetCenterDirUp() const;

  /// \brief Returns the actual right vector.
  const ezVec3& GetCenterDirRight() const;

  /// \brief Returns the near plane distance.
  float GetNearPlane() const;

  /// \brief Returns the far plane distance.
  float GetFarPlane() const;

  /// \brief Specifies the mode and the basic settings that this camera uses.
  ///
  /// \param fFovOrDim
  ///   Fov X/Y in degree or width/height (depending on Mode)
  void SetCameraMode(ezCameraMode::Enum Mode, float fFovOrDim, float fNearPlane, float fFarPlane);

  /// \brief Returns the fFovOrDim parameter that was passed to SetCameraMode().
  float GetFovOrDim() const;

  /// \brief Returns the current camera mode.
  ezCameraMode::Enum GetCameraMode() const;
  bool IsPerspective() const;
  bool IsOrthographic() const;

  /// \brief Sets the camera position and rotation from the given look at matrix.
  void SetFromMatrix(const ezMat4& mLookAtMatrix);

  /// \brief Repositions the camera such that it looks at the given target position.
  void LookAt(const ezVec3& vCameraPos, const ezVec3& vTargetPos, const ezVec3& vUp);

  /// \brief Moves the camera in its local space. Returns the movement that was made.
  ezVec3 MoveLocally (float fForward, float fRight, float fUp);

  /// \brief Moves the camera in global space.
  void MoveGlobally(const ezVec3& vMove);

  /// \brief Rotates the camera around the X (forward), Y (right) and Z (up) axis in its own local space.
  ///
  /// Rotate around Y for looking up/down. X is roll. For turning left/right use Z with RotateGlobally().
  void RotateLocally (ezAngle X, ezAngle Y, ezAngle Z);

  /// \brief Rotates the camera around the X, Y and Z axis in global space.
  ///
  /// Rotate around Z for turning the camera left/right.
  void RotateGlobally(ezAngle X, ezAngle Y, ezAngle Z);

  /// \brief Calculates the view matrix from the current camera properties and stores it in out_viewMatrix.
  void GetViewMatrix(ezMat4& out_viewMatrix) const;

  /// \brief Calculates the projection matrix from the current camera properties and stores it in out_projectionMatrix.
  void GetProjectionMatrix(float fAspectRatioWidthDivHeight, ezMat4& out_projectionMatrix, ezProjectionDepthRange::Enum depthRange = ezProjectionDepthRange::Default) const;

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
  /// Override this function to implement restrictions on the camera position or rotation.
  virtual void CameraOrientationChanged(bool bPosition, bool bRotation) { ++m_uiOrientationModificationCounter; }

  /// \brief This function is called when the camera mode changes (e.g. SetCameraMode was called).
  /// Override this to do sanity checks or restrict certain values.
  virtual void CameraSettingsChanged();

  /// \brief This function is called by RotateLocally() and RotateGlobally() BEFORE the values are applied,
  /// and allows to adjust them (e.g. for limiting how far the camera can rotate).
  virtual void ClampRotationAngles(bool bLocalSpace, ezAngle& X, ezAngle& Y, ezAngle& Z);

  float m_fNearPlane;
  float m_fFarPlane;

  ezCameraMode::Enum m_Mode;

  float m_fFovOrDim;

  ezVec3 m_vPosition;
  ezVec3 m_vDirForwards;
  ezVec3 m_vDirUp;
  ezVec3 m_vDirRight;

  float m_fExposure;

  ezUInt32 m_uiSettingsModificationCounter;
  ezUInt32 m_uiOrientationModificationCounter;
};


#include <Core/Graphics/Implementation/Camera_inl.h>


