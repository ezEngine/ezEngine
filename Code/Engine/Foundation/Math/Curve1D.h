#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Math/Color.h>

class ezStreamWriter;
class ezStreamReader;

/// \brief A 1D curve for animating a single value over time.
class EZ_FOUNDATION_DLL ezCurve1D
{
public:
  /// \brief Stores position and tangents to control spline interpolation
  struct ControlPoint
  {
    EZ_DECLARE_POD_TYPE();

    /// \brief The position (x,y) of the control point
    ezVec2 m_Position;

    /// \brief The tangent for the curve segment to the left that affects the spline interpolation
    ezVec2 m_LeftTangent;
    /// \brief The tangent for the curve segment to the right that affects the spline interpolation
    ezVec2 m_RightTangent;

    EZ_ALWAYS_INLINE bool operator<(const ControlPoint& rhs) const { return m_Position.x < rhs.m_Position.x; }
  };

public:
  ezCurve1D();

  /// \brief Removes all control points.
  void Clear();

  /// \brief Checks whether the curve has any control point.
  bool IsEmpty() const;

  /// \brief Appends a control point. SortControlPoints() must be called to before evaluating the curve.
  ControlPoint& AddControlPoint(float pos);

  /// \brief Updates the min/max X value that can be retrieved through GetExtents().
  ///
  /// This is automatically done when SortControlPoints() is called. It can be called manually, if the information is required without sorting.
  void RecomputeExtents();

  /// \brief returns the min and max position value across all control points.
  ///
  /// The returned values are only up to date if either SortControlPoints() or RecomputeExtents() was called before.
  /// Otherwise they will contain stale values.
  void QueryExtents(float& minx, float& maxx) const;

  /// \brief Returns the min and max Y value across the curve.
  /// For this information to be available, the linear approximation of the curve must have been computed, otherwise stale values will be returned.
  void QueryExtremeValues(float& minVal, float& maxVal) const;

  /// \brief Returns the number of control points.
  ezUInt32 GetNumControlPoints() const;

  /// \brief Const access to a control point.
  const ControlPoint& GetControlPoint(ezUInt32 idx) const { return m_ControlPoints[idx]; }

  /// \brief Non-const access to a control point. If you modify the position, SortControlPoints() has to be called before evaluating the curve.
  ControlPoint& ModifyControlPoint(ezUInt32 idx) { return m_ControlPoints[idx]; }

  /// \brief Sorts the control point arrays by their position. The CPs have to be sorted before calling Evaluate(), otherwise the result will be wrong.
  void SortControlPoints();

  /// \brief Evaluates the curve at the given position (x coordinate) and returns the value Y value at that point.
  ///
  /// This uses the linear approximation of the curve, so CreateLinearApproximation() must have been called first.
  ///
  /// \sa CreateLinearApproximation
  float Evaluate(float position) const;

  /// \brief Takes the normalized x coordinate [0;1] and converts it into a valid position on the curve
  ///
  /// \note This only works when the curve extents are available. See QueryExtents() and RecomputeExtents().
  ///
  /// \sa RecomputeExtents
  /// \sa QueryExtents
  float ConvertNormalizedPos(float pos) const;

  /// \brief Takes a value (typically returned by Evaluate()) and normalizes it into [0;1] range
  ///
  /// \note This only works when the linear approximation of the curve has been computed first.
  float NormalizeValue(float value) const;

  /// \brief How much heap memory the curve uses.
  ezUInt64 GetHeapMemoryUsage() const;

  /// \brief Stores the current state in a stream.
  void Save(ezStreamWriter& stream) const;

  /// \brief Restores the state from a stream.
  void Load(ezStreamReader& stream);

  /// \brief Precomputes sample points for linear interpolation that approximate the curve within the allowed error threshold.
  ///
  /// \note All control points must already be in sorted order, so call SortControlPoints() first if necessary.
  void CreateLinearApproximation(float fMaxError = 0.01f);

  /// \brief Adjusts the tangents such that the curve cannot make loopings
  void ClampTangents();

  void MakeFixedLengthTangents();

  void SetCurveColor(const ezColor& color) { m_CurveColor = color; }
  const ezColor& GetCurveColor() const { return m_CurveColor; }

private:
  void RecomputeExtremes();
  void ApproximateCurve(const ezVec2& p0, const ezVec2& p1, const ezVec2& p2, const ezVec2& p3, float fMaxErrorSQR);
  void ApproximateCurvePiece(const ezVec2& p0, const ezVec2& p1, const ezVec2& p2, const ezVec2& p3, float tLeft, const ezVec2& pLeft, float tRight, const ezVec2& pRight, float fMaxErrorSQR);
  ezInt32 FindApproxControlPoint(float x) const;

  float m_fMinX, m_fMaxX;
  float m_fMinY, m_fMaxY;
  ezHybridArray<ControlPoint, 8> m_ControlPoints;
  ezHybridArray<ezVec2, 24> m_LinearApproximation;

  ezColor m_CurveColor;
};

