#pragma once

/// \brief A 16 bit IEEE float class. Often called "half"
class ezFloat16
{
public:
  // Means that vectors can be copied using memcpy instead of copy construction.
  EZ_DECLARE_POD_TYPE();

  /// \brief Create float16 from float.
  EZ_FOUNDATION_DLL ezFloat16(float f = 0.0f); // [tested]

  /// \brief Create float16 from raw data.
  ezFloat16(ezUInt16 data) : m_uiData(data) {}

  /// \brief Convert float16 to float.
  EZ_FOUNDATION_DLL operator float() const; // [tested]
  
  /// \brief Returns true, if both values are identical.
  bool operator== (const ezFloat16& c2) { return m_uiData == c2.m_uiData; } // [tested]

  /// \brief Returns true, if both values are not identical.
  bool operator!= (const ezFloat16& c2) { return m_uiData != c2.m_uiData; } // [tested]

private:
  /// Raw 16 float data.
  ezUInt16 m_uiData;
};