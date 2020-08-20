#pragma once

#include <Foundation/Basics.h>

/// \brief A simple rectangle class templated on the type for x, y and width, height.
///
template <typename Type>
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

  /// The smaller value along x
  Type Left() const { return x; }

  /// The larger value along x
  Type Right() const { return x + width; }

  /// The smaller value along y
  Type Top() const { return y; }

  /// The larger value along y
  Type Bottom() const { return y + height; }


  // *** Common Functions ***
public:
  bool operator==(const ezRectTemplate<Type>& rhs) const;

  bool operator!=(const ezRectTemplate<Type>& rhs) const;

  /// \brief Returns true if the area of the rectangle is non zero
  bool HasNonZeroArea() const;

  /// \brief Returns true if the rectangle contains the provided point
  bool Contains(const ezVec2Template<Type>& point) const;

  /// \brief Returns true if the rectangle overlaps the provided rectangle.
  /// Also returns true if the rectangles are contained within each other completely(no intersecting edges).
  bool Overlaps(const ezRectTemplate<Type>& other) const;

  /// \brief Extends this rectangle so that the provided rectangle is completely contained within it.
  void Encapsulate(const ezRectTemplate<Type>& other);

  /// \brief Clips current rectangle so that it does not overlap the provided rectangle.
  bool Clip(const ezRectTemplate<Type>& clipRect);
};

#include <Foundation/Math/Implementation/Rect_inl.h>

typedef ezRectTemplate<ezUInt32> ezRectU32;
typedef ezRectTemplate<ezUInt16> ezRectU16;
typedef ezRectTemplate<float> ezRectFloat;
typedef ezRectTemplate<double> ezRectDouble;
