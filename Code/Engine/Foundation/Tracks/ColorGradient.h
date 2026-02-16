#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Math/Declarations.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>
#include <Foundation/Threading/Mutex.h>

class ezStreamWriter;
class ezStreamReader;

/// \brief Color control point. Stores red, green and blue in gamma space.
struct EZ_FOUNDATION_DLL ezColorGradientColorCP
{
  EZ_DECLARE_POD_TYPE();

  ezInt64 m_iTick; ///< Position in time. 4800 ticks per second.
  ezUInt8 m_GammaRed;
  ezUInt8 m_GammaGreen;
  ezUInt8 m_GammaBlue;
  mutable float m_fInvDistToNextCp; ///< Cached 1/distance to next control point for faster interpolation

  EZ_ALWAYS_INLINE bool operator<(const ezColorGradientColorCP& rhs) const { return m_iTick < rhs.m_iTick; }
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezColorGradientColorCP);

/// \brief Alpha control point.
struct EZ_FOUNDATION_DLL ezColorGradientAlphaCP
{
  EZ_DECLARE_POD_TYPE();

  ezInt64 m_iTick;                  ///< Position in time. 4800 ticks per second.
  ezUInt8 m_Alpha;
  mutable float m_fInvDistToNextCp; ///< Cached 1/distance to next control point for faster interpolation

  EZ_ALWAYS_INLINE bool operator<(const ezColorGradientAlphaCP& rhs) const { return m_iTick < rhs.m_iTick; }
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezColorGradientAlphaCP);

/// \brief Intensity control point. Used to scale rgb for high-dynamic range values.
struct EZ_FOUNDATION_DLL ezColorGradientIntensityCP
{
  EZ_DECLARE_POD_TYPE();

  ezInt64 m_iTick;                  ///< Position in time. 4800 ticks per second.
  float m_Intensity;
  mutable float m_fInvDistToNextCp; ///< Cached 1/distance to next control point for faster interpolation

  EZ_ALWAYS_INLINE bool operator<(const ezColorGradientIntensityCP& rhs) const { return m_iTick < rhs.m_iTick; }
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezColorGradientIntensityCP);

/// \brief A color curve for animating colors.
///
/// The gradient consists of a number of control points, for rgb, alpha and intensity.
/// One can evaluate the curve at any x coordinate.
class EZ_FOUNDATION_DLL ezColorGradient
{
  EZ_ALLOW_PRIVATE_PROPERTIES(ezColorGradient);

public:
  using ColorCP = ezColorGradientColorCP;
  using AlphaCP = ezColorGradientAlphaCP;
  using IntensityCP = ezColorGradientIntensityCP;

public:
  ezColorGradient();

  ezColorGradient(const ezColorGradient& rhs);
  ezColorGradient(ezColorGradient&& rhs) noexcept;

  void operator=(const ezColorGradient& rhs);
  void operator=(ezColorGradient&& rhs) noexcept;

  /// \brief Removes all control points.
  void Clear();

  /// \brief Checks whether the curve has any control point.
  bool IsEmpty() const;

  /// \brief Appends a color control point.
  void AddColorControlPoint(double x, const ezColorGammaUB& rgb);

  /// \brief Appends an alpha control point.
  void AddAlphaControlPoint(double x, ezUInt8 uiAlpha);

  /// \brief Appends an intensity control point.
  void AddIntensityControlPoint(double x, float fIntensity);

  /// \brief Determines the min and max x-coordinate value across all control points.
  bool GetExtents(double& ref_fMinx, double& ref_fMaxx) const;

  /// \brief Returns the number of control points of each type.
  void GetNumControlPoints(ezUInt32& ref_uiRgb, ezUInt32& ref_uiAlpha, ezUInt32& ref_uiIntensity) const;

  /// \brief Const access to a control point.
  const ColorCP& GetColorControlPoint(ezUInt32 uiIdx) const { return m_ColorCPs[uiIdx]; }
  /// \brief Const access to a control point.
  const AlphaCP& GetAlphaControlPoint(ezUInt32 uiIdx) const { return m_AlphaCPs[uiIdx]; }
  /// \brief Const access to a control point.
  const IntensityCP& GetIntensityControlPoint(ezUInt32 uiIdx) const { return m_IntensityCPs[uiIdx]; }

  /// \brief Non-const access to a control point.
  ///
  /// Invalidates the cached sort order, which will be rebuilt on next evaluation.
  ColorCP& ModifyColorControlPoint(ezUInt32 uiIdx)
  {
    m_ColorOrder.Clear();
    return m_ColorCPs[uiIdx];
  }

  /// \brief Non-const access to a control point.
  ///
  /// Invalidates the cached sort order, which will be rebuilt on next evaluation.
  AlphaCP& ModifyAlphaControlPoint(ezUInt32 uiIdx)
  {
    m_AlphaOrder.Clear();
    return m_AlphaCPs[uiIdx];
  }

  /// \brief Non-const access to a control point.
  ///
  /// Invalidates the cached sort order, which will be rebuilt on next evaluation.
  IntensityCP& ModifyIntensityControlPoint(ezUInt32 uiIdx)
  {
    m_IntensityOrder.Clear();
    return m_IntensityCPs[uiIdx];
  }

  /// \brief Evaluates the curve at the given x-coordinate and returns RGBA and intensity separately.
  void Evaluate(double x, ezColorGammaUB& ref_rgba, float& ref_fIntensity) const;

  /// \brief Evaluates the curve and returns RGBA and intensity in one combined ezColor value.
  void Evaluate(double x, ezColor& ref_hdr) const;

  /// \brief Evaluates only the color curve.
  void EvaluateColor(double x, ezColorGammaUB& ref_rgb) const;
  /// \brief Evaluates only the color curve.
  void EvaluateColor(double x, ezColor& ref_rgb) const;
  /// \brief Evaluates only the alpha curve.
  void EvaluateAlpha(double x, ezUInt8& ref_uiAlpha) const;
  /// \brief Evaluates only the intensity curve.
  void EvaluateIntensity(double x, float& ref_fIntensity) const;

  /// \brief How much heap memory the curve uses.
  ezUInt64 GetHeapMemoryUsage() const;

  /// \brief Stores the current state in a stream.
  void Save(ezStreamWriter& inout_stream) const;

  /// \brief Restores the state from a stream.
  void Load(ezStreamReader& inout_stream);

  /// \brief Converts a tick value to time (in seconds). 4800 ticks per second.
  static double TickToTime(ezInt64 iTick) { return iTick / 4800.0; }

  /// \brief Converts a time value (in seconds) to ticks. 4800 ticks per second.
  static ezInt64 TimeToTick(double fTimeInSeconds) { return static_cast<ezInt64>(fTimeInSeconds * 4800.0); }

  /// \brief Converts a time value to ticks and snaps to the nearest frame boundary for the given FPS.
  static ezInt64 SnapTimeToTick(double fTimeInSeconds, ezUInt32 uiFramesPerSecond = 120);

  /// \brief Snaps a tick value to the nearest frame boundary for the given FPS.
  static ezInt64 SnapTickTo(ezInt64 iTick, ezUInt32 uiFramesPerSecond);

  /// \brief Snaps a time value to the nearest frame boundary for the given FPS.
  static double SnapTimeTo(double fTimeInSeconds, ezUInt32 uiFramesPerSecond = 120);

private:
  /// \brief Caches the inverse distance between consecutive control points for faster interpolation.
  void PrecomputeLerpNormalizer() const;

  /// \brief Builds the sort order arrays without modifying the original control point arrays.
  ///
  /// Control points are stored in insertion order to preserve array indices during editing.
  /// The sort order arrays are built lazily during evaluation to access points in temporal order.
  void UpdatePointOrder() const;

  ezSmallArray<ColorCP, 8> m_ColorCPs;
  ezSmallArray<AlphaCP, 8> m_AlphaCPs;
  ezSmallArray<IntensityCP, 8> m_IntensityCPs;

  /// Mapping from sorted position to storage index. Cleared when control points are modified.
  mutable ezSmallArray<ezUInt8, 8> m_ColorOrder;
  mutable ezSmallArray<ezUInt8, 8> m_AlphaOrder;
  mutable ezSmallArray<ezUInt8, 8> m_IntensityOrder;

  /// Protects lazy initialization of sort order arrays and precomputed values during evaluation.
  mutable ezMutex m_InitializationMutex;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_FOUNDATION_DLL, ezColorGradient);
