#pragma once

/// \brief A 16 bit IEEE float class. Often called "half"
///
/// This class only contains functions to convert between float and float16. It does not support any mathematical operations.
/// It is only intended for conversion, always to all mathematical operations on regular floats (or let the GPU do them on halfs).
class EZ_FOUNDATION_DLL ezFloat16
{
public:
  // Means that vectors can be copied using memcpy instead of copy construction.
  EZ_DECLARE_POD_TYPE();

  /// \brief Create float16 from float.
  ezFloat16(float f = 0.0f); // [tested]

  /// \brief Create float16 from float.
  void operator=(float f); // [tested]

  /// \brief Create float16 from raw data.
  void SetRawData(ezUInt16 data) { m_uiData = data; } // [tested]

  /// \brief Returns the raw 16 Bit data.
  ezUInt16 GetRawData() const { return m_uiData; } // [tested]

  /// \brief Convert float16 to float.
  operator float() const; // [tested]
  
  /// \brief Returns true, if both values are identical.
  bool operator== (const ezFloat16& c2) { return m_uiData == c2.m_uiData; } // [tested]

  /// \brief Returns true, if both values are not identical.
  bool operator!= (const ezFloat16& c2) { return m_uiData != c2.m_uiData; } // [tested]

private:
  /// Raw 16 float data.
  ezUInt16 m_uiData;
};

