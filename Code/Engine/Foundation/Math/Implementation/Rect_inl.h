#pragma once

template <typename Type>
EZ_ALWAYS_INLINE ezRectTemplate<Type>::ezRectTemplate()
{
}

template <typename Type>
EZ_ALWAYS_INLINE ezRectTemplate<Type>::ezRectTemplate(Type x, Type y, Type width, Type height)
  : x(x)
  , y(y)
  , width(width)
  , height(height)
{
}

template <typename Type>
EZ_ALWAYS_INLINE ezRectTemplate<Type>::ezRectTemplate(Type width, Type height)
  : x(0)
  , y(0)
  , width(width)
  , height(height)
{
}

template <typename Type>
EZ_ALWAYS_INLINE bool ezRectTemplate<Type>::operator==(const ezRectTemplate<Type>& rhs) const
{
  return x == rhs.x && y == rhs.y && width == rhs.width && height == rhs.height;
}

template <typename Type>
EZ_ALWAYS_INLINE bool ezRectTemplate<Type>::operator!=(const ezRectTemplate<Type>& rhs) const
{
  return !(*this == rhs);
}

template <typename Type>
EZ_ALWAYS_INLINE bool ezRectTemplate<Type>::HasNonZeroArea() const
{
  return (width > 0) && (height > 0);
}

template <typename Type>
EZ_ALWAYS_INLINE bool ezRectTemplate<Type>::Contains(const ezVec2Template<Type>& vPoint) const
{
  if (vPoint.x >= x && vPoint.x <= Right())
  {
    if (vPoint.y >= y && vPoint.y <= Bottom())
      return true;
  }

  return false;
}

template <typename Type>
EZ_ALWAYS_INLINE bool ezRectTemplate<Type>::Overlaps(const ezRectTemplate<Type>& other) const
{
  if (x < other.Right() && Right() > other.x && y < other.Bottom() && Bottom() > other.y)
    return true;

  return false;
}

template <typename Type>
void ezRectTemplate<Type>::ExpandToInclude(const ezRectTemplate<Type>& other)
{
  Type thisRight = Right();
  Type thisBottom = Bottom();

  if (other.x < x)
    x = other.x;

  if (other.y < y)
    y = other.y;

  if (other.Right() > thisRight)
    width = other.Right() - x;
  else
    width = thisRight - x;

  if (other.Bottom() > thisBottom)
    height = other.Bottom() - y;
  else
    height = thisBottom - y;
}

template <typename Type>
EZ_ALWAYS_INLINE void ezRectTemplate<Type>::Clip(const ezRectTemplate<Type>& clipRect)
{
  Type newLeft = ezMath::Max<Type>(x, clipRect.x);
  Type newTop = ezMath::Max<Type>(y, clipRect.y);

  Type newRight = ezMath::Min<Type>(Right(), clipRect.Right());
  Type newBottom = ezMath::Min<Type>(Bottom(), clipRect.Bottom());

  x = newLeft;
  y = newTop;
  width = newRight - newLeft;
  height = newBottom - newTop;
}

template <typename Type>
EZ_ALWAYS_INLINE void ezRectTemplate<Type>::SetInvalid()
{
  /// \test This is new

  const Type fLargeValue = ezMath::MaxValue<Type>() / 2;
  x = fLargeValue;
  y = fLargeValue;
  width = -fLargeValue;
  height = -fLargeValue;
}

template <typename Type>
EZ_ALWAYS_INLINE bool ezRectTemplate<Type>::IsValid() const
{
  /// \test This is new

  return width >= 0 && height >= 0;
}

template <typename Type>
EZ_ALWAYS_INLINE const ezVec2Template<Type> ezRectTemplate<Type>::GetClampedPoint(const ezVec2Template<Type>& vPoint) const
{
  /// \test This is new

  return ezVec2Template<Type>(ezMath::Clamp(vPoint.x, Left(), Right()), ezMath::Clamp(vPoint.y, Top(), Bottom()));
}

template <typename Type>
void ezRectTemplate<Type>::SetIntersection(const ezRectTemplate<Type>& r0, const ezRectTemplate<Type>& r1)
{
  /// \test This is new

  Type x1 = ezMath::Max(r0.GetX1(), r1.GetX1());
  Type y1 = ezMath::Max(r0.GetY1(), r1.GetY1());
  Type x2 = ezMath::Min(r0.GetX2(), r1.GetX2());
  Type y2 = ezMath::Min(r0.GetY2(), r1.GetY2());

  x = x1;
  y = y1;
  width = x2 - x1;
  height = y2 - y1;
}

template <typename Type>
void ezRectTemplate<Type>::SetUnion(const ezRectTemplate<Type>& r0, const ezRectTemplate<Type>& r1)
{
  /// \test This is new

  Type x1 = ezMath::Min(r0.GetX1(), r1.GetX1());
  Type y1 = ezMath::Min(r0.GetY1(), r1.GetY1());
  Type x2 = ezMath::Max(r0.GetX2(), r1.GetX2());
  Type y2 = ezMath::Max(r0.GetY2(), r1.GetY2());

  x = x1;
  y = y1;
  width = x2 - x1;
  height = y2 - y1;
}

template <typename Type>
void ezRectTemplate<Type>::Translate(Type tX, Type tY)
{
  /// \test This is new

  x += tX;
  y += tY;
}

template <typename Type>
void ezRectTemplate<Type>::Scale(Type sX, Type sY)
{
  /// \test This is new

  x *= sX;
  y *= sY;
  width *= sX;
  height *= sY;
}
