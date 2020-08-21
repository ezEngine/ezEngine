#pragma once

template <typename Type>
EZ_ALWAYS_INLINE ezRectTemplate<Type>::ezRectTemplate()
{
}

template <typename Type>
EZ_ALWAYS_INLINE ezRectTemplate<Type>::ezRectTemplate(Type X, Type Y, Type Width, Type Height)
  : x(X)
  , y(Y)
  , width(Width)
  , height(Height)
{
}

template <typename Type>
EZ_ALWAYS_INLINE ezRectTemplate<Type>::ezRectTemplate(Type Width, Type Height)
  : x(0)
  , y(0)
  , width(Width)
  , height(Height)
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
EZ_ALWAYS_INLINE bool ezRectTemplate<Type>::Contains(const ezVec2Template<Type>& point) const
{
  if (point.x >= x && point.x <= Right())
  {
    if (point.y >= y && point.y <= Bottom())
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
EZ_ALWAYS_INLINE void ezRectTemplate<Type>::Encapsulate(const ezRectTemplate<Type>& other)
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
EZ_ALWAYS_INLINE bool ezRectTemplate<Type>::Clip(const ezRectTemplate<Type>& clipRect)
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
