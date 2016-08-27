#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Containers/HybridArray.h>

class ezStreamWriter;
class ezStreamReader;

/// \brief A 1D curve for animating a single value over time.
class EZ_FOUNDATION_DLL ezCurve1D
{
public:
  /// \brief Stores position (time / x-coordinate) and value (y-coordinate), as well as points to control spline interpolation
  struct ControlPoint
  {
    EZ_DECLARE_POD_TYPE();

    float m_fPosX;
    float m_fValue;

    /// \brief The tangent for the curve segment to the left that affects the spline interpolation
    ezVec2 m_LeftTangent;
    /// \brief The tangent for the curve segment to the right that affects the spline interpolation
    ezVec2 m_RightTangent;

    EZ_FORCE_INLINE bool operator<(const ControlPoint& rhs) const { return m_fPosX < rhs.m_fPosX; }
  };

public:
  ezCurve1D();

  /// \brief Removes all control points.
  void Clear();

  /// \brief Checks whether the curve has any control point.
  bool IsEmpty() const;

  /// \brief Appends a control point. SortControlPoints() must be called to before evaluating the curve.
  ControlPoint& AddControlPoint(float pos);

  /// \brief Determines the min and max position value across all control points.
  void GetExtents(float& minx, float& maxx) const;

  /// \brief Determines the min and max Y value across all control points.
  void GetExtremeValues(float& minVal, float& maxVal) const;

  /// \brief Returns the number of control points.
  ezUInt32 GetNumControlPoints() const;

  /// \brief Const access to a control point.
  const ControlPoint& GetControlPoint(ezUInt32 idx) const { return m_ControlPoints[idx]; }

  /// \brief Non-const access to a control point. If you modify the position, SortControlPoints() has to be called before evaluating the curve.
  ControlPoint& ModifyControlPoint(ezUInt32 idx) { m_bRecomputeBBox = true;  return m_ControlPoints[idx]; }

  /// \brief Sorts the control point arrays by their position. The CPs have to be sorted before calling Evaluate(), otherwise the result will be wrong.
  void SortControlPoints();

  /// \brief Evaluates the curve at the given position and returns the value.
  ///
  /// The control points have to be sorted, so call SortControlPoints() before, if any modifications where done.
  float Evaluate(float position) const;

  /// \brief Takes the normalized position [0;1] and converts it into a valid position on the curve
  float ConvertNormalizedPos(float pos) const;

  /// \brief Takes a value (typically returned by Evaluate()) and normalizes it into [0;1] range
  float NormalizeValue(float value) const;

  /// \brief How much heap memory the curve uses.
  ezUInt64 GetHeapMemoryUsage() const;

  /// \brief Stores the current state in a stream.
  void Save(ezStreamWriter& stream) const;

  /// \brief Restores the state from a stream.
  void Load(ezStreamReader& stream);

private:
  void RecomputeBBox() const;

  mutable bool m_bRecomputeBBox;
  mutable float m_fMinX, m_fMaxX;
  mutable float m_fMinY, m_fMaxY;
  ezHybridArray<ControlPoint, 8> m_ControlPoints;
};