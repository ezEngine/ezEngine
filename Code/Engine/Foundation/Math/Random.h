#pragma once

#include <Foundation/Math/Declarations.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Containers/DynamicArray.h>

/// \brief A random number generator. Currently uses the WELL512 algorithm.
class EZ_FOUNDATION_DLL ezRandom
{
public:
  ezRandom();

  /// \brief Initializes the RNG with the given seed value. The value should not be zero.
  void Initialize(ezUInt64 uiSeed); // [tested]

  /// \brief Initializes the RNG using current time stamp.
  /// Not very sophisticated, but good enough for things that do not need to be secure.
  void InitializeFromCurrentTime();

  /// \brief Serializes the current state
  void Save(ezStreamWriter& stream) const; // [tested]

  /// \brief Deserializes the current state
  void Load(ezStreamReader& stream); // [tested]

  /// \brief Returns a uint32 value, ie. ranging from 0 to (2 ^ 32) - 1
  ezUInt32 UInt(); // [tested]

  /// \brief Returns a uint32 value in range [0 ; uiRange - 1]
  ///
  /// \note A range of 0 is invalid and will assert! It also has no mathematical meaning. A range of 1 already means "between 0 and 1 EXCLUDING 1".
  /// So always use a range of at least 1.
  ezUInt32 UIntInRange(ezUInt32 uiRange); // [tested]

  /// \brief Returns an int32 value in range [iMinValue ; iMinValue + uiRange - 1]
  ///
  /// \note A range of 0 is invalid and will assert! It also has no mathematical meaning. A range of 1 already means "between 0 and 1 EXCLUDING 1".
  /// So always use a range of at least 1.
  ezInt32 IntInRange(ezInt32 iMinValue, ezUInt32 uiRange); // [tested]

  /// \brief Returns an int32 value in range [iMinValue ; iMaxValue]
  ezInt32 IntMinMax(ezInt32 iMinValue, ezInt32 iMaxValue); // [tested]

  /// \brief Returns a value in range [0.0 ; 1.0), ie. including zero, but excluding one
  EZ_FORCE_INLINE double DoubleZeroToOneExclusive() { return (double)UInt() / (double)(0xFFFFFFFFUL); } // [tested]

  /// \brief Returns a value in range [0.0 ; 1.0], ie. including zero and one
  EZ_FORCE_INLINE double DoubleZeroToOneInclusive() { return (double)UInt() / (double)(0xFFFFFFFFUL + 1.0); } // [tested]

  /// \brief Returns a double value in range [fMinValue ; fMinValue + fRange)
  double DoubleInRange(double fMinValue, double fRange); // [tested]

  /// \brief Returns a double value in range [fMinValue ; fMaxValue]
  double DoubleMinMax(double fMinValue, double fMaxValue); // [tested]

  /// \brief Returns a double value around fValue with a given variance (0 - 1 range)
  double DoubleVariance(double fValue, double fVariance);

private:
  ezUInt32 m_uiIndex;
  ezUInt32 m_uiState[16];
};


/// \brief A random number generator that produces values with a normal / Gaussian distribution
class EZ_FOUNDATION_DLL ezRandomGauss
{
public:

  /// \brief Initializes the RNG and sets the maximum value that the functions UnsignedValue() and SignedValue() may return
  ///
  /// The Variance configures the distribution of the samples. 1.0 gives a standard bell-curve. Values below 1 lead to a distribution
  /// with more emphasis around zero, whereas values above 1 result in a flatter curve with more equally distributed results.
  ///
  /// For more details, look here: https://en.wikipedia.org/wiki/Normal_distribution
  void Initialize(ezUInt64 uiRandomSeed, ezUInt32 uiMaxValue, float fVariance = 1.0f); // [tested]

  /// \brief Returns a value in range [0; uiMaxValue - 1] with a Gaussian distribution. Ie. 0 is much more probable than uiMaxValue.
  ezUInt32 UnsignedValue(); // [tested]

  /// \brief Returns a value in range [-uiMaxValue + 1; uiMaxValue - 1] with a Gaussian distribution. Ie. 0 is much more probable than +/-uiMaxValue.
  ezInt32 SignedValue(); // [tested]

  /// \brief Serializes the current state
  void Save(ezStreamWriter& stream) const; // [tested]

  /// \brief Deserializes the current state
  void Load(ezStreamReader& stream); // [tested]

private:
  void SetupTable(ezUInt32 uiMaxValue, float fSigma);

  float m_fSigma;
  double m_fAreaSum;
  ezDynamicArray<float> m_GaussAreaSum;
  ezRandom m_Generator;
};

#include <Foundation/Math/Implementation/AllClassesRandom_inl.h>


