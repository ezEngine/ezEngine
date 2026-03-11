#pragma once

#include <Foundation/Basics.h>

/// \brief Generic two-dimensional size representation with width and height components
///
/// Provides a simple container for representing rectangular dimensions in 2D space.
/// The template parameter allows using different numeric types (integers, floats) depending
/// on precision requirements. Common typedefs include ezSizeU32, ezSizeFloat, and ezSizeDouble.
/// Primarily used for representing viewport dimensions, texture sizes, and UI element bounds.
template <typename Type>
class ezSizeTemplate
{
public:
  // Means this object can be copied using memcpy instead of copy construction.
  EZ_DECLARE_POD_TYPE();

  // *** Data ***
public:
  Type width;
  Type height;

  // *** Constructors ***
public:
  /// \brief Default constructor does not initialize the data.
  ezSizeTemplate();

  /// \brief Constructor to set all values.
  ezSizeTemplate(Type width, Type height);

  // *** Common Functions ***
public:
  /// \brief Returns true if the area described by the size is non zero
  bool HasNonZeroArea() const;
};

template <typename Type>
bool operator==(const ezSizeTemplate<Type>& v1, const ezSizeTemplate<Type>& v2);

template <typename Type>
bool operator!=(const ezSizeTemplate<Type>& v1, const ezSizeTemplate<Type>& v2);

#include <Foundation/Math/Implementation/Size_inl.h>

using ezSizeU32 = ezSizeTemplate<ezUInt32>;
using ezSizeFloat = ezSizeTemplate<float>;
using ezSizeDouble = ezSizeTemplate<double>;

EZ_FOUNDATION_DLL ezStringView BuildString(char* szTmp, ezUInt32 uiLength, const ezSizeU32& arg);
