#pragma once

#include <Foundation/Basics.h>

/// \brief A simple size class templated on the type for width and height.
///
template<typename Type>
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
  ezSizeTemplate(Type Width, Type Height);


// *** Common Functions ***
public:

  /// \brief Returns true if the area described by the size is non zero
  bool HasNonZeroArea() const;

};

#include <Foundation/Math/Implementation/Size_inl.h>

typedef ezSizeTemplate<ezUInt32> ezSizeU32;
typedef ezSizeTemplate<float> ezSizeFloat;
typedef ezSizeTemplate<double> ezSizeDouble;




