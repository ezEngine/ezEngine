#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

/// Script extension class providing deterministic random number generation for scripts.
///
/// Generates reproducible random sequences using a seed and position-based approach.
/// The same seed and position sequence will always produce identical results,
/// which is essential for deterministic gameplay and debugging.
class EZ_CORE_DLL ezScriptExtensionClass_StableRandom
{
public:
  /// Generates a random integer within the specified range.
  ///
  /// \param inout_iPosition Current position in the random sequence, automatically incremented
  /// \param iMinValue Minimum value (inclusive)
  /// \param iMaxValue Maximum value (inclusive)
  /// \param uiSeed Random seed to use for generation
  /// \return Random integer between iMinValue and iMaxValue
  static int IntMinMax(int& inout_iPosition, int iMinValue, int iMaxValue, ezUInt32 uiSeed);

  /// Generates a random float between 0.0 and 1.0.
  ///
  /// \param inout_iPosition Current position in the random sequence, automatically incremented
  /// \param uiSeed Random seed to use for generation
  /// \return Random float between 0.0 and 1.0
  static float FloatZeroToOne(int& inout_iPosition, ezUInt32 uiSeed);

  /// Generates a random float within the specified range.
  ///
  /// \param inout_iPosition Current position in the random sequence, automatically incremented
  /// \param fMinValue Minimum value (inclusive)
  /// \param fMaxValue Maximum value (inclusive)
  /// \param uiSeed Random seed to use for generation
  /// \return Random float between fMinValue and fMaxValue
  static float FloatMinMax(int& inout_iPosition, float fMinValue, float fMaxValue, ezUInt32 uiSeed);

  /// Generates a random 3D vector with components within the specified ranges.
  ///
  /// \param inout_iPosition Current position in the random sequence, automatically incremented
  /// \param vMinValue Minimum values for each component (inclusive)
  /// \param vMaxValue Maximum values for each component (inclusive)
  /// \param uiSeed Random seed to use for generation
  /// \return Random vector with each component between corresponding min and max values
  static ezVec3 Vec3MinMax(int& inout_iPosition, const ezVec3& vMinValue, const ezVec3& vMaxValue, ezUInt32 uiSeed);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_CORE_DLL, ezScriptExtensionClass_StableRandom);
