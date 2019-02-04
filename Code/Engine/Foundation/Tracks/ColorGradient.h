#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Math/Declarations.h>

class ezStreamWriter;
class ezStreamReader;

/// \brief A color curve for animating colors.
///
/// The gradient consists of a number of control points, for rgb, alpha and intensity.
/// One can evaluate the curve at any x coordinate.
class EZ_FOUNDATION_DLL ezColorGradient
{
public:
  /// \brief Color control point. Stores red, green and blue in gamma space.
  struct ColorCP
  {
    EZ_DECLARE_POD_TYPE();

    double m_PosX;
    ezUInt8 m_GammaRed;
    ezUInt8 m_GammaGreen;
    ezUInt8 m_GammaBlue;
    float m_fInvDistToNextCp; /// Internal: Optimization for Evaluate to not recalculate 1/distance to the next control point

    EZ_ALWAYS_INLINE bool operator<(const ColorCP& rhs) const { return m_PosX < rhs.m_PosX; }
  };

  /// \brief Alpha control point.
  struct AlphaCP
  {
    EZ_DECLARE_POD_TYPE();

    double m_PosX;
    ezUInt8 m_Alpha;
    float m_fInvDistToNextCp; /// Internal: Optimization for Evaluate to not recalculate 1/distance to the next control point

    EZ_ALWAYS_INLINE bool operator<(const AlphaCP& rhs) const { return m_PosX < rhs.m_PosX; }
  };

  /// \brief Intensity control point. Used to scale rgb for high-dynamic range values.
  struct IntensityCP
  {
    EZ_DECLARE_POD_TYPE();

    double m_PosX;
    float m_Intensity;
    float m_fInvDistToNextCp; /// Internal: Optimization for Evaluate to not recalculate 1/distance to the next control point

    EZ_ALWAYS_INLINE bool operator<(const IntensityCP& rhs) const { return m_PosX < rhs.m_PosX; }
  };

public:
  ezColorGradient();

  /// \brief Removes all control points.
  void Clear();

  /// \brief Checks whether the curve has any control point.
  bool IsEmpty() const;

  /// \brief Appends a color control point. SortControlPoints() must be called to before evaluating the curve.
  void AddColorControlPoint(double x, const ezColorGammaUB& rgb);

  /// \brief Appends an alpha control point. SortControlPoints() must be called to before evaluating the curve.
  void AddAlphaControlPoint(double x, ezUInt8 alpha);

  /// \brief Appends an intensity control point. SortControlPoints() must be called to before evaluating the curve.
  void AddIntensityControlPoint(double x, float intensity);

  /// \brief Determines the min and max x-coordinate value across all control points.
  bool GetExtents(double& minx, double& maxx) const;

  /// \brief Returns the number of control points of each type.
  void GetNumControlPoints(ezUInt32& rgb, ezUInt32& alpha, ezUInt32& intensity) const;

  /// \brief Const access to a control point.
  const ColorCP& GetColorControlPoint(ezUInt32 idx) const { return m_ColorCPs[idx]; }
  /// \brief Const access to a control point.
  const AlphaCP& GetAlphaControlPoint(ezUInt32 idx) const { return m_AlphaCPs[idx]; }
  /// \brief Const access to a control point.
  const IntensityCP& GetIntensityControlPoint(ezUInt32 idx) const { return m_IntensityCPs[idx]; }

  /// \brief Non-const access to a control point. If you modify the x coordinate, SortControlPoints() has to be called before evaluating the
  /// curve.
  ColorCP& ModifyColorControlPoint(ezUInt32 idx) { return m_ColorCPs[idx]; }
  /// \brief Non-const access to a control point. If you modify the x coordinate, SortControlPoints() has to be called before evaluating the
  /// curve.
  AlphaCP& ModifyAlphaControlPoint(ezUInt32 idx) { return m_AlphaCPs[idx]; }
  /// \brief Non-const access to a control point. If you modify the x coordinate, SortControlPoints() has to be called before evaluating the
  /// curve.
  IntensityCP& ModifyIntensityControlPoint(ezUInt32 idx) { return m_IntensityCPs[idx]; }

  /// \brief Sorts the control point arrays by their x-coordinate. The CPs have to be sorted before calling Evaluate(), otherwise the result
  /// will be wrong.
  void SortControlPoints();

  /// \brief Evaluates the curve at the given x-coordinate and returns RGBA and intensity separately.
  ///
  /// The control points have to be sorted, so call SortControlPoints() before, if any modifications where done.
  void Evaluate(double x, ezColorGammaUB& rgba, float& intensity) const;

  /// \brief Evaluates the curve and returns RGBA and intensity in one combined ezColor value.
  void Evaluate(double x, ezColor& hdr) const;

  /// \brief Evaluates only the color curve.
  void EvaluateColor(double x, ezColorGammaUB& rgb) const;
  /// \brief Evaluates only the color curve.
  void EvaluateColor(double x, ezColor& rgb) const;
  /// \brief Evaluates only the alpha curve.
  void EvaluateAlpha(double x, ezUInt8& alpha) const;
  /// \brief Evaluates only the intensity curve.
  void EvaluateIntensity(double x, float& intensity) const;

  /// \brief How much heap memory the curve uses.
  ezUInt64 GetHeapMemoryUsage() const;

  /// \brief Stores the current state in a stream.
  void Save(ezStreamWriter& stream) const;

  /// \brief Restores the state from a stream.
  void Load(ezStreamReader& stream);

private:
  void PrecomputeLerpNormalizer();

  ezHybridArray<ColorCP, 8> m_ColorCPs;
  ezHybridArray<AlphaCP, 8> m_AlphaCPs;
  ezHybridArray<IntensityCP, 8> m_IntensityCPs;
};

