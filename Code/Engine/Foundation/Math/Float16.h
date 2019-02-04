#pragma once

/// \brief A 16 bit IEEE float class. Often called "half"
///
/// This class only contains functions to convert between float and float16. It does not support any mathematical operations.
/// It is only intended for conversion, always do all mathematical operations on regular floats (or let the GPU do them on halfs).
class EZ_FOUNDATION_DLL ezFloat16
{
public:
  // Means that vectors can be copied using memcpy instead of copy construction.
  EZ_DECLARE_POD_TYPE();

  /// \brief Default constructor does not initialize the value.
  ezFloat16() = default;

  /// \brief Create float16 from float.
  ezFloat16(float f); // [tested]

  /// \brief Create float16 from float.
  void operator=(float f); // [tested]

  /// \brief Create float16 from raw data.
  void SetRawData(ezUInt16 data) { m_uiData = data; } // [tested]

  /// \brief Returns the raw 16 Bit data.
  ezUInt16 GetRawData() const { return m_uiData; } // [tested]

  /// \brief Convert float16 to float.
  operator float() const; // [tested]

  /// \brief Returns true, if both values are identical.
  bool operator==(const ezFloat16& c2) { return m_uiData == c2.m_uiData; } // [tested]

  /// \brief Returns true, if both values are not identical.
  bool operator!=(const ezFloat16& c2) { return m_uiData != c2.m_uiData; } // [tested]

private:
  /// Raw 16 float data.
  ezUInt16 m_uiData;
};

/// \brief A simple helper class to use half-precision floats (ezFloat16) as vectors
class EZ_FOUNDATION_DLL ezFloat16Vec2
{
public:
  // Means that vectors can be copied using memcpy instead of copy construction.
  EZ_DECLARE_POD_TYPE();

  ezFloat16Vec2() = default;
  ezFloat16Vec2(const ezVec2& vec);

  void operator=(const ezVec2& vec);
  operator ezVec2() const;

  ezFloat16 x, y;
};

/// \brief A simple helper class to use half-precision floats (ezFloat16) as vectors
class EZ_FOUNDATION_DLL ezFloat16Vec3
{
public:
  // Means that vectors can be copied using memcpy instead of copy construction.
  EZ_DECLARE_POD_TYPE();

  ezFloat16Vec3() = default;
  ezFloat16Vec3(const ezVec3& vec);

  void operator=(const ezVec3& vec);
  operator ezVec3() const;

  ezFloat16 x, y, z;
};

/// \brief A simple helper class to use half-precision floats (ezFloat16) as vectors
class EZ_FOUNDATION_DLL ezFloat16Vec4
{
public:
  // Means that vectors can be copied using memcpy instead of copy construction.
  EZ_DECLARE_POD_TYPE();

  ezFloat16Vec4() = default;
  ezFloat16Vec4(const ezVec4& vec);

  void operator=(const ezVec4& vec);
  operator ezVec4() const;

  ezFloat16 x, y, z, w;
};

