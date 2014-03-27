#pragma once

template<typename Type>
EZ_FORCE_INLINE ezSizeTemplate<Type>::ezSizeTemplate()
{
}

template<typename Type>
EZ_FORCE_INLINE ezSizeTemplate<Type>::ezSizeTemplate(Type Width, Type Height)
  : width(Width), height(Height)
{
}

template<typename Type>
EZ_FORCE_INLINE bool ezSizeTemplate<Type>::HasNonZeroArea() const
{
  return (width > 0) && (height > 0);
}


