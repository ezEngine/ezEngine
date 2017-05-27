#pragma once

template<typename Type>
EZ_ALWAYS_INLINE ezRectTemplate<Type>::ezRectTemplate()
{
}

template<typename Type>
EZ_ALWAYS_INLINE ezRectTemplate<Type>::ezRectTemplate(Type X, Type Y, Type Width, Type Height)
  : x(X), y(Y), width(Width), height(Height)
{
}

template<typename Type>
EZ_ALWAYS_INLINE ezRectTemplate<Type>::ezRectTemplate(Type Width, Type Height)
  : x(0), y(0), width(Width), height(Height)
{
}

template<typename Type>
EZ_ALWAYS_INLINE bool ezRectTemplate<Type>::operator==(const ezRectTemplate<Type>& rhs) const
{
  return x == rhs.x && y == rhs.y && width == rhs.width && height == rhs.height;
}

template<typename Type>
EZ_ALWAYS_INLINE bool ezRectTemplate<Type>::operator!=(const ezRectTemplate<Type>& rhs) const
{
  return !(*this == rhs);
}

template<typename Type>
EZ_ALWAYS_INLINE bool ezRectTemplate<Type>::HasNonZeroArea() const
{
  return (width > 0) && (height > 0);
}


