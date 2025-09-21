#pragma once

#include <Core/CoreDLL.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/SimdMath/SimdTransform.h>

class ezStreamReader;
class ezStreamWriter;

/// \brief The different modes that tangents may use in a spline control point.
struct ezSplineTangentMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Auto,   ///< The curvature through the control point is automatically computed to be smooth.
    Custom, ///< Custom tangents specified by the user.
    Linear, ///< There is no curvature through this control point/tangent. Creates sharp corners.

    Default = Auto
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezSplineTangentMode);

//////////////////////////////////////////////////////////////////////////

/// \brief Describes a spline consisting of cubic Bezier curves segments. Each control point defines the position, rotation, and scale at that point.
/// The parameter fT to evaluate the spline is a combination of the control point index and the zero to one parameter in the fractional part to interpolate within that segment.
struct EZ_CORE_DLL ezSpline
{
  struct ControlPoint
  {
    ezSimdVec4f m_vPos = ezSimdVec4f::MakeZero();
    ezSimdVec4f m_vPosTangentIn = ezSimdVec4f::MakeZero();  // Contains the tangent mode in w
    ezSimdVec4f m_vPosTangentOut = ezSimdVec4f::MakeZero(); // Contains the tangent mode in w

    ezSimdVec4f m_vUpDirAndRoll = ezSimdVec4f::MakeZero();  // Roll angle in w
    ezSimdVec4f m_vUpDirTangentIn = ezSimdVec4f::MakeZero();
    ezSimdVec4f m_vUpDirTangentOut = ezSimdVec4f::MakeZero();

    ezSimdVec4f m_vScale = ezSimdVec4f(1);
    ezSimdVec4f m_vScaleTangentIn = ezSimdVec4f::MakeZero();
    ezSimdVec4f m_vScaleTangentOut = ezSimdVec4f::MakeZero();

    ezResult Serialize(ezStreamWriter& ref_writer) const;
    ezResult Deserialize(ezStreamReader& ref_reader);

    void SetPosition(const ezSimdVec4f& vPos);

    ezSplineTangentMode::Enum GetTangentModeIn() const;
    void SetTangentModeIn(ezSplineTangentMode::Enum mode);
    void SetTangentIn(const ezSimdVec4f& vTangent, ezSplineTangentMode::Enum mode = ezSplineTangentMode::Custom);

    ezSplineTangentMode::Enum GetTangentModeOut() const;
    void SetTangentModeOut(ezSplineTangentMode::Enum mode);
    void SetTangentOut(const ezSimdVec4f& vTangent, ezSplineTangentMode::Enum mode = ezSplineTangentMode::Custom);

    ezAngle GetRoll() const;
    void SetRoll(ezAngle roll);

    void SetScale(const ezSimdVec4f& vScale);

    void SetAutoTangents(const ezSimdVec4f& vDirIn, const ezSimdVec4f& vDirOut);
  };

  ezDynamicArray<ControlPoint, ezAlignedAllocatorWrapper> m_ControlPoints;
  bool m_bClosed = false;
  ezUInt32 m_uiChangeCounter = 0; //< Not incremented automatically, but can be incremented by the user to signal that the spline has changed.

  ezResult Serialize(ezStreamWriter& ref_writer) const;
  ezResult Deserialize(ezStreamReader& ref_reader);

  /// \brief Calculates tangents for all control points with a tangent mode other than 'Custom'.
  void CalculateUpDirAndAutoTangents(const ezSimdVec4f& vGlobalUpDir = ezSimdVec4f(0, 0, 1), const ezSimdVec4f& vGlobalForwardDir = ezSimdVec4f(1, 0, 0));

  /// \brief Returns the position of the spline at the given parameter fT.
  ezSimdVec4f EvaluatePosition(float fT) const;
  ezSimdVec4f EvaluatePosition(ezUInt32 uiCp0, const ezSimdFloat& fT) const;

  /// \brief Returns the derivative, aka the tangent of the spline at the given parameter fT. This also equals to the unnormalized forward direction.
  ezSimdVec4f EvaluateDerivative(float fT) const;
  ezSimdVec4f EvaluateDerivative(ezUInt32 uiCp0, const ezSimdFloat& fT) const;

  /// \brief Returns the up direction of the spline at the given parameter fT.
  ezSimdVec4f EvaluateUpDirection(float fT) const;

  /// \brief Returns the scale of the spline at the given parameter fT.
  ezSimdVec4f EvaluateScale(float fT) const;

  /// \brief Returns the full transform (consisting of position, scale, and orientation) of the spline at the given parameter fT.
  ezSimdTransform EvaluateTransform(float fT) const;

private:
  float ClampAndSplitT(float fT, ezUInt32& out_uiIndex) const;
  ezUInt32 GetCp1Index(ezUInt32 uiCp0) const;

  ezSimdVec4f EvaluatePosition(const ControlPoint& cp0, const ControlPoint& cp1, const ezSimdFloat& fT) const;
  ezSimdVec4f EvaluateDerivative(const ControlPoint& cp0, const ControlPoint& cp1, const ezSimdFloat& fT) const;

  void EvaluateRotation(const ControlPoint& cp0, const ControlPoint& cp1, const ezSimdFloat& fT, ezSimdVec4f& out_forwardDir, ezSimdVec4f& out_rightDir, ezSimdVec4f& out_upDir) const;

  ezSimdVec4f EvaluateScale(const ControlPoint& cp0, const ControlPoint& cp1, const ezSimdFloat& fT) const;
};

#include <Core/Graphics/Implementation/Spline_inl.h>
