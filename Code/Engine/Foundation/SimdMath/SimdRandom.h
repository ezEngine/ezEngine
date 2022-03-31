#pragma once

#include <Foundation/SimdMath/SimdVec4u.h>

/// \brief Noise based random number generator that generates 4 pseudo random values at once.
///
/// Does not keep any internal state but relies on the user to provide different positions for each call.
/// The seed parameter can be used to further alter the noise function.
struct ezSimdRandom
{
  /// \brief Returns 4 random uint32 values at position, ie. ranging from 0 to (2 ^ 32) - 1
  static ezSimdVec4u UInt(const ezSimdVec4i& position, const ezSimdVec4u& seed = ezSimdVec4u::ZeroVector());

  /// \brief Returns 4 random float values in range [0.0 ; 1.0], ie. including zero and one
  static ezSimdVec4f FloatZeroToOne(const ezSimdVec4i& position, const ezSimdVec4u& seed = ezSimdVec4u::ZeroVector());

  /// \brief Returns 4 random float values in range [fMinValue ; fMaxValue]
  static ezSimdVec4f FloatMinMax(const ezSimdVec4i& position, const ezSimdVec4f& minValue, const ezSimdVec4f& maxValue, const ezSimdVec4u& seed = ezSimdVec4u::ZeroVector());
};

#include <Foundation/SimdMath/Implementation/SimdRandom_inl.h>
