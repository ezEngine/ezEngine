#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Vec2.h>

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
  ezRectTemplate(Type x, Type y, Type width, Type height);

  /// \brief Initializes x and y with zero, width and height with the given values.
  ezRectTemplate(Type width, Type height);

  /// The smaller value along x.
  Type Left() const { return x; }

  /// The larger value along x.
  Type Right() const { return x + width; }

  /// The smaller value along y.
  Type Top() const { return y; }

  /// The larger value along y.
  Type Bottom() const { return y + height; }

  /// The smaller value along x. Same as Left().
  Type GetX1() const { return x; }

  /// The larger value along x. Same as Right().
  Type GetX2() const { return x + width; }

  /// The smaller value along y. Same as Top().
  Type GetY1() const { return y; }

  /// The larger value along y. Same as Bottom().
  Type GetY2() const { return y + height; }

  // *** Common Functions ***
public:
  bool operator==(const ezRectTemplate<Type>& rhs) const;

  bool operator!=(const ezRectTemplate<Type>& rhs) const;

  /// \brief Sets the rect to invalid values.
  ///
  /// IsValid() will return false afterwards.
  /// It is possible to make an invalid rect valid using ExpandToInclude().
  void SetInvalid();

  /// \brief Checks whether the position and size contain valid values.
  bool IsValid() const;

  /// \brief Returns true if the area of the rectangle is non zero
  bool HasNonZeroArea() const;

  /// \brief Returns true if the rectangle contains the provided point
  bool Contains(const ezVec2Template<Type>& vPoint) const;

  /// \brief Returns true if the rectangle overlaps the provided rectangle.
  /// Also returns true if the rectangles are contained within each other completely(no intersecting edges).
  bool Overlaps(const ezRectTemplate<Type>& other) const;

  /// \brief Extends this rectangle so that the provided rectangle is completely contained within it.
  void ExpandToInclude(const ezRectTemplate<Type>& other);

  /// \brief Clips this rect so that it is fully inside the provided rectangle.
  void Clip(const ezRectTemplate<Type>& clipRect);

  /// \brief The given point is clamped to the area of the rect, i.e. it will be either inside the rect or on its edge and it will have the closest
  /// possible distance to the original point.
  const ezVec2Template<Type> GetClampedPoint(const ezVec2Template<Type>& vPoint) const;

  void SetIntersection(const ezRectTemplate<Type>& r0, const ezRectTemplate<Type>& r1);

  void SetUnion(const ezRectTemplate<Type>& r0, const ezRectTemplate<Type>& r1);

  /// \brief Moves the rectangle
  void Translate(Type tX, Type tY);

  /// \brief Scales width and height, and moves the position as well.
  void Scale(Type sX, Type sY);
};

#include <Foundation/Math/Implementation/Rect_inl.h>

using ezRectU32 = ezRectTemplate<ezUInt32>;
using ezRectU16 = ezRectTemplate<ezUInt16>;
using ezRectI32 = ezRectTemplate<ezInt32>;
using ezRectI16 = ezRectTemplate<ezInt16>;
using ezRectFloat = ezRectTemplate<float>;
using ezRectDouble = ezRectTemplate<double>;
