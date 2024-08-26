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

  /// \brief Initializes x and y from pos, width and height from vSize.
  ezRectTemplate<Type>(const ezVec2Template<Type>& vTopLeftPosition, const ezVec2Template<Type>& vSize);

  /// \brief Creates an 'invalid' rect.
  ///
  /// IsValid() will return false.
  /// It is possible to make an invalid rect valid using ExpandToInclude().
  [[nodiscard]] static ezRectTemplate<Type> MakeInvalid();

  /// \brief Creates a rect that is located at the origin and has zero size. This is a 'valid' rect.
  [[nodiscard]] static ezRectTemplate<Type> MakeZero();

  /// \brief Creates a rect that is the intersection of the two provided rects.
  ///
  /// If the two rects don't overlap, the result will be a valid rect, but have zero area.
  /// See IsValid() and HasNonZeroArea().
  [[nodiscard]] static ezRectTemplate<Type> MakeIntersection(const ezRectTemplate<Type>& r0, const ezRectTemplate<Type>& r1);

  /// \brief Creates a rect that is the union of the two provided rects.
  ///
  /// This is the same as constructing a bounding box around the two rects.
  [[nodiscard]] static ezRectTemplate<Type> MakeUnion(const ezRectTemplate<Type>& r0, const ezRectTemplate<Type>& r1);

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

  /// \brief Returns the minimum corner position. Same as GetTopLeft().
  ezVec2Template<Type> GetMinCorner() const { return ezVec2Template<Type>(x, y); }

  /// \brief Returns the maximum corner position. Same as GetBottomRight().
  ezVec2Template<Type> GetMaxCorner() const { return ezVec2Template<Type>(x + width, y + height); }

  /// \brief Returns the top left corner. Same as GetMinCorner().
  ezVec2Template<Type> GetTopLeft() const { return ezVec2Template<Type>(x, y); }

  /// \brief Returns the top right corner.
  ezVec2Template<Type> GetTopRight() const { return ezVec2Template<Type>(x + width, y); }

  /// \brief Returns the bottom left corner.
  ezVec2Template<Type> GetBottomLeft() const { return ezVec2Template<Type>(x, y + height); }

  /// \brief Returns the bottom right corner. Same as GetMaxCorner().
  ezVec2Template<Type> GetBottomRight() const { return ezVec2Template<Type>(x + width, y + height); }

  /// \brief Returns the center point of the rectangle.
  ezVec2Template<Type> GetCenter() const { return ezVec2Template<Type>(x + width / 2, y + height / 2); }

  /// \brief Returns the width and height as a vec2.
  ezVec2Template<Type> GetExtents() const { return ezVec2Template<Type>(width, height); }

  /// \brief Returns the half width and half height as a vec2.
  ezVec2Template<Type> GetHalfExtents() const { return ezVec2Template<Type>(width / 2, height / 2); }

  /// \brief Increases the size of the rect in all directions.
  void Grow(Type xy);

  // *** Common Functions ***
public:
  [[nodiscard]] bool operator==(const ezRectTemplate<Type>& rhs) const;
  [[nodiscard]] bool operator!=(const ezRectTemplate<Type>& rhs) const;

  /// \brief Checks whether the position and size contain valid values.
  [[nodiscard]] bool IsValid() const;

  /// \brief Returns true if the area of the rectangle is non zero
  [[nodiscard]] bool HasNonZeroArea() const;

  /// \brief Returns true if the rectangle contains the provided point
  [[nodiscard]] bool Contains(const ezVec2Template<Type>& vPoint) const;

  [[nodiscard]] bool Contains(const ezRectTemplate<Type>& r) const;

  /// \brief Returns true if the rectangle overlaps the provided rectangle.
  /// Also returns true if the rectangles are contained within each other completely(no intersecting edges).
  [[nodiscard]] bool Overlaps(const ezRectTemplate<Type>& other) const;

  /// \brief Extends this rectangle so that the provided rectangle is completely contained within it.
  void ExpandToInclude(const ezRectTemplate<Type>& other);

  /// \brief Extends this rectangle so that the provided point is contained within it.
  void ExpandToInclude(const ezVec2Template<Type>& other);

  /// \brief Clips this rect so that it is fully inside the provided rectangle.
  void Clip(const ezRectTemplate<Type>& clipRect);

  /// \brief The given point is clamped to the area of the rect, i.e. it will be either inside the rect or on its edge and it will have the closest
  /// possible distance to the original point.
  [[nodiscard]] const ezVec2Template<Type> GetClampedPoint(const ezVec2Template<Type>& vPoint) const;

  /// \brief Clamps the given rect to the area of this rect and returns it.
  ///
  /// If the input rect is entirely outside this rect, the result will be reduced to a point or a line closest to the input rect.
  [[nodiscard]] const ezRectTemplate<Type> GetClampedRect(const ezRectTemplate<Type>& r) const
  {
    const ezVec2Template<Type> vNewMin = GetClampedPoint(r.GetMinCorner());
    const ezVec2Template<Type> vNewMax = GetClampedPoint(r.GetMaxCorner());
    return ezRectTemplate<Type>(vNewMin, vNewMax - vNewMin);
  }

  /// \brief Sets the center of the rectangle.
  void SetCenter(Type tX, Type tY);

  /// \brief Moves the rectangle.
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
