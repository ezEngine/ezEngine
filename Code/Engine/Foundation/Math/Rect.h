#pragma once

#include <Foundation/Basics.h>

/// \brief A simple rectangle class templated on the type for x, y and width, height.
///
template<typename Type>
class ezRectTemplate
{
public:
  // Means this object can be copied using memcpy instead of copy construction.
  EZ_DECLARE_POD_TYPE();

// *** Data ***
public:
  Type x;
  Type y;

  Type width;
  Type height;

// *** Constructors ***
public:

  /// \brief Default constructor does not initialize the data.
  ezRectTemplate();

  /// \brief Constructor to set all values.
  ezRectTemplate(Type X, Type Y, Type Width, Type Height);

  /// \brief Initializes x and y with zero, width and height with the given values.
  ezRectTemplate(Type Width, Type Height);


// *** Common Functions ***
public:

  /// \brief Returns true if the area of the rectangle is non zero
  bool HasNonZeroArea() const;

};

#include <Foundation/Math/Implementation/Rect_inl.h>

typedef ezRectTemplate<ezUInt32> ezRectU32;
typedef ezRectTemplate<ezUInt16> ezRectU16;
typedef ezRectTemplate<float> ezRectFloat;
typedef ezRectTemplate<double> ezRectDouble;




