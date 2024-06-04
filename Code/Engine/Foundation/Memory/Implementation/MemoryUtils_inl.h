
#define EZ_CHECK_CLASS(T)                   \
  static_assert(!std::is_trivial<T>::value, \
    "Trivial POD type is treated as class. Use EZ_DECLARE_POD_TYPE(YourClass) or EZ_DEFINE_AS_POD_TYPE(ExternalClass) to mark it as POD.")

template <ezConstructionMode mode, typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::Construct(T* pDestination, size_t uiCount)
{
  if constexpr (mode == SkipTrivialTypes && std::is_trivial<T>::value)
  {
    // do nothing
  }
  else
  {
    for (size_t i = 0; i < uiCount; i++)
    {
      ::new (pDestination + i) T();
    }
  }
}

template <ezConstructionMode mode, typename T>
EZ_ALWAYS_INLINE ezMemoryUtils::ConstructorFunction ezMemoryUtils::MakeConstructorFunction()
{
  if constexpr (mode == SkipTrivialTypes && std::is_trivial<T>::value)
  {
    return nullptr;
  }
  else
  {
    struct Helper
    {
      static void Construct(void* pDestination) { ezMemoryUtils::Construct<mode>(static_cast<T*>(pDestination), 1); }
    };

    return &Helper::Construct;
  }
}

template <typename Destination, typename Source>
EZ_ALWAYS_INLINE void ezMemoryUtils::CopyConstruct(Destination* pDestination, const Source& copy, size_t uiCount)
{
  if constexpr (ezIsPodType<Destination>::value)
  {
    static_assert(std::is_same<Destination, Source>::value ||
                    (std::is_base_of<Destination, Source>::value == false && std::is_base_of<Source, Destination>::value == false),
      "Can't copy POD types that are derived from each other. Are you certain any of these types should be POD?");

    const Destination& copyConverted = copy;
    for (size_t i = 0; i < uiCount; i++)
    {
      memcpy(pDestination + i, &copyConverted, sizeof(Destination));
    }
  }
  else
  {
    EZ_CHECK_CLASS(Destination);

    for (size_t i = 0; i < uiCount; i++)
    {
      ::new (pDestination + i) Destination(copy); // Note that until now copy has not been converted to Destination. This allows for calling
                                                  // specialized constructors if available.
    }
  }
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::CopyConstructArray(T* pDestination, const T* pSource, size_t uiCount)
{
  EZ_ASSERT_DEV(pDestination + uiCount <= pSource || pSource + uiCount <= pDestination, "Memory regions must not overlap when using CopyConstruct.");

  if constexpr (ezIsPodType<T>::value)
  {
    memcpy(pDestination, pSource, uiCount * sizeof(T));
  }
  else
  {
    EZ_CHECK_CLASS(T);

    for (size_t i = 0; i < uiCount; i++)
    {
      ::new (pDestination + i) T(pSource[i]);
    }
  }
}

template <typename T>
EZ_ALWAYS_INLINE ezMemoryUtils::CopyConstructorFunction ezMemoryUtils::MakeCopyConstructorFunction()
{
  struct Helper
  {
    static void CopyConstruct(void* pDestination, const void* pSource)
    {
      ezMemoryUtils::CopyConstruct(static_cast<T*>(pDestination), *static_cast<const T*>(pSource), 1);
    }
  };

  return &Helper::CopyConstruct;
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::MoveConstruct(T* pDestination, T&& source)
{
  // Make sure source is actually an rvalue reference (T&& is a universal reference).
  static_assert(std::is_rvalue_reference<decltype(source)>::value, "'source' parameter is not an rvalue reference.");
  ::new (pDestination) T(std::forward<T>(source));
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::MoveConstruct(T* pDestination, T* pSource, size_t uiCount)
{
  EZ_ASSERT_DEV(pDestination + uiCount <= pSource || pSource + uiCount <= pDestination, "Memory regions must not overlap when using MoveConstruct.");

  // Enforce move construction.
  static_assert(std::is_move_constructible<T>::value, "Type is not move constructible!");

  for (size_t i = 0; i < uiCount; ++i)
  {
    ::new (pDestination + i) T(std::move(pSource[i]));
  }
}

template <typename Destination, typename Source>
EZ_ALWAYS_INLINE void ezMemoryUtils::CopyOrMoveConstruct(Destination* pDestination, Source&& source)
{
  if constexpr (std::is_rvalue_reference<decltype(source)>::value)
  {
    static_assert(std::is_rvalue_reference<decltype(source)>::value, "This version of CopyOrMoveConstruct should only be called with a rvalue reference!");

    ::new (pDestination) Destination(std::move(source));
  }
  else
  {
    CopyConstruct<Destination, Source>(pDestination, source, 1);
  }
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::RelocateConstruct(T* pDestination, T* pSource, size_t uiCount)
{
  EZ_ASSERT_DEV(pDestination + uiCount <= pSource || pSource + uiCount <= pDestination, "Memory regions must not overlap when using RelocateConstruct.");

  if constexpr (ezGetTypeClass<T>::value != 0) // POD or mem-relocatable
  {
    memcpy(pDestination, pSource, uiCount * sizeof(T));
  }
  else // class
  {
    EZ_CHECK_CLASS(T);

    for (size_t i = 0; i < uiCount; i++)
    {
      // Note that this calls the move constructor only if available and will copy otherwise.
      ::new (pDestination + i) T(std::move(pSource[i]));
    }

    Destruct(pSource, uiCount);
  }
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::Destruct(T* pDestination, size_t uiCount)
{
  if constexpr (ezIsPodType<T>::value == 1)
  {
    static_assert(std::is_trivially_destructible<T>::value != 0, "Class is declared as POD but has a non-trivial destructor. Remove the destructor or don't declare it as POD.");
  }
  else if constexpr (std::is_trivially_destructible<T>::value == 0)
  {
    for (size_t i = 0; i < uiCount; ++i)
    {
      pDestination[i].~T();
    }
  }
}

template <typename T>
EZ_ALWAYS_INLINE ezMemoryUtils::DestructorFunction ezMemoryUtils::MakeDestructorFunction()
{
  if constexpr (ezIsPodType<T>::value)
  {
    return nullptr;
  }
  else
  {
    EZ_CHECK_CLASS(T);

    struct Helper
    {
      static void Destruct(void* pDestination) { ezMemoryUtils::Destruct(static_cast<T*>(pDestination), 1); }
    };

    return &Helper::Destruct;
  }
}

EZ_ALWAYS_INLINE void ezMemoryUtils::RawByteCopy(void* pDestination, const void* pSource, size_t uiNumBytesToCopy)
{
  memcpy(pDestination, pSource, uiNumBytesToCopy);
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::Copy(T* pDestination, const T* pSource, size_t uiCount)
{
  EZ_ASSERT_DEV(pDestination < pSource || pSource + uiCount <= pDestination, "Memory regions must not overlap when using Copy. Use CopyOverlapped instead.");

  if constexpr (ezIsPodType<T>::value)
  {
    memcpy(pDestination, pSource, uiCount * sizeof(T));
  }
  else
  {
    EZ_CHECK_CLASS(T);

    for (size_t i = 0; i < uiCount; i++)
    {
      pDestination[i] = pSource[i];
    }
  }
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::CopyOverlapped(T* pDestination, const T* pSource, size_t uiCount)
{
  if constexpr (ezIsPodType<T>::value)
  {
    memmove(pDestination, pSource, uiCount * sizeof(T));
  }
  else
  {
    EZ_CHECK_CLASS(T);

    if (pDestination == pSource)
      return;

    if (pDestination < pSource)
    {
      for (size_t i = 0; i < uiCount; i++)
      {
        pDestination[i] = pSource[i];
      }
    }
    else
    {
      for (size_t i = uiCount; i > 0; --i)
      {
        pDestination[i - 1] = pSource[i - 1];
      }
    }
  }
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::Relocate(T* pDestination, T* pSource, size_t uiCount)
{
  EZ_ASSERT_DEV(pDestination + uiCount <= pSource || pSource + uiCount <= pDestination, "Memory regions must not overlap when using Relocate.");

  if constexpr (ezGetTypeClass<T>::value != 0) // POD or mem-relocatable
  {
    memcpy(pDestination, pSource, uiCount * sizeof(T));
  }
  else // class
  {
    EZ_CHECK_CLASS(T);

    for (size_t i = 0; i < uiCount; i++)
    {
      // Note that this calls the move constructor only if available and will copy otherwise.
      pDestination[i] = std::move(pSource[i]);
    }

    Destruct(pSource, uiCount);
  }
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::RelocateOverlapped(T* pDestination, T* pSource, size_t uiCount)
{
  if constexpr (ezGetTypeClass<T>::value == 2) // mem-relocatable
  {
    if (pDestination < pSource)
    {
      size_t uiDestructCount = pSource - pDestination;
      Destruct(pDestination, uiDestructCount);
    }
    else
    {
      size_t uiDestructCount = pDestination - pSource;
      Destruct(pSource + uiCount, uiDestructCount);
    }
    memmove(pDestination, pSource, uiCount * sizeof(T));
  }
  else if constexpr (ezGetTypeClass<T>::value == 1) // POD
  {
    memmove(pDestination, pSource, uiCount * sizeof(T));
  }
  else
  {
    EZ_CHECK_CLASS(T);

    if (pDestination == pSource)
      return;

    if (pDestination < pSource)
    {
      for (size_t i = 0; i < uiCount; i++)
      {
        pDestination[i] = std::move(pSource[i]);
      }

      size_t uiDestructCount = pSource - pDestination;
      Destruct(pSource + uiCount - uiDestructCount, uiDestructCount);
    }
    else
    {
      for (size_t i = uiCount; i > 0; --i)
      {
        pDestination[i - 1] = std::move(pSource[i - 1]);
      }

      size_t uiDestructCount = pDestination - pSource;
      Destruct(pSource, uiDestructCount);
    }
  }
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::Prepend(T* pDestination, const T& source, size_t uiCount)
{
  if constexpr (ezGetTypeClass<T>::value != 0) // POD or mem-relocatable
  {
    memmove(pDestination + 1, pDestination, uiCount * sizeof(T));
    CopyConstruct(pDestination, source, 1);
  }
  else // class
  {
    EZ_CHECK_CLASS(T);

    if (uiCount > 0)
    {
      MoveConstruct(pDestination + uiCount, std::move(pDestination[uiCount - 1]));

      for (size_t i = uiCount - 1; i > 0; --i)
      {
        pDestination[i] = std::move(pDestination[i - 1]);
      }

      *pDestination = source;
    }
    else
    {
      CopyConstruct(pDestination, source, 1);
    }
  }
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::Prepend(T* pDestination, T&& source, size_t uiCount)
{
  if constexpr (ezGetTypeClass<T>::value != 0) // POD or mem-relocatable
  {
    memmove(pDestination + 1, pDestination, uiCount * sizeof(T));
    MoveConstruct(pDestination, std::move(source));
  }
  else // class
  {
    EZ_CHECK_CLASS(T);

    if (uiCount > 0)
    {
      MoveConstruct(pDestination + uiCount, std::move(pDestination[uiCount - 1]));

      for (size_t i = uiCount - 1; i > 0; --i)
      {
        pDestination[i] = std::move(pDestination[i - 1]);
      }

      *pDestination = std::move(source);
    }
    else
    {
      MoveConstruct(pDestination, std::move(source));
    }
  }
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::Prepend(T* pDestination, const T* pSource, size_t uiSourceCount, size_t uiCount)
{
  if constexpr (ezGetTypeClass<T>::value != 0) // POD or mem-relocatable
  {
    memmove(pDestination + uiSourceCount, pDestination, uiCount * sizeof(T));
    CopyConstructArray(pDestination, pSource, uiSourceCount);
  }
  else // class
  {
    EZ_CHECK_CLASS(T);

    if (uiCount > 0)
    {
      MoveConstruct(pDestination + uiSourceCount, pDestination, uiCount);
      CopyConstructArray(pDestination, pSource, uiSourceCount);
    }
    else
    {
      CopyConstructArray(pDestination, pSource, uiSourceCount);
    }
  }
}

template <typename T>
EZ_ALWAYS_INLINE bool ezMemoryUtils::IsEqual(const T* a, const T* b, size_t uiCount /*= 1*/)
{
  if constexpr (ezIsPodType<T>::value)
  {
    return memcmp(a, b, uiCount * sizeof(T)) == 0;
  }
  else
  {
    EZ_CHECK_CLASS(T);

    for (size_t i = 0; i < uiCount; i++)
    {
      if (!(a[i] == b[i]))
        return false;
    }
    return true;
  }
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::ZeroFill(T* pDestination, size_t uiCount)
{
  memset(pDestination, 0, uiCount * sizeof(T));
}

template <typename T, size_t N>
EZ_ALWAYS_INLINE void ezMemoryUtils::ZeroFillArray(T (&destination)[N])
{
  return ZeroFill(destination, N);
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::PatternFill(T* pDestination, ezUInt8 uiBytePattern, size_t uiCount)
{
  memset(pDestination, uiBytePattern, uiCount * sizeof(T));
}

template <typename T, size_t N>
EZ_ALWAYS_INLINE void ezMemoryUtils::PatternFillArray(T (&destination)[N], ezUInt8 uiBytePattern)
{
  return PatternFill(destination, uiBytePattern, N);
}

template <typename T>
EZ_ALWAYS_INLINE ezInt32 ezMemoryUtils::Compare(const T* a, const T* b, size_t uiCount /*= 1*/)
{
  return memcmp(a, b, uiCount * sizeof(T));
}

EZ_ALWAYS_INLINE ezInt32 ezMemoryUtils::RawByteCompare(const void* a, const void* b, size_t uiNumBytesToCompare)
{
  return memcmp(a, b, uiNumBytesToCompare);
}

template <typename T>
EZ_ALWAYS_INLINE T* ezMemoryUtils::AddByteOffset(T* pPtr, std::ptrdiff_t offset)
{
  return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(pPtr) + offset);
}

template <typename T>
EZ_ALWAYS_INLINE T* ezMemoryUtils::AlignBackwards(T* pPtr, size_t uiAlignment)
{
  return reinterpret_cast<T*>(reinterpret_cast<size_t>(pPtr) & ~(uiAlignment - 1));
}

template <typename T>
EZ_ALWAYS_INLINE T* ezMemoryUtils::AlignForwards(T* pPtr, size_t uiAlignment)
{
  return reinterpret_cast<T*>((reinterpret_cast<size_t>(pPtr) + uiAlignment - 1) & ~(uiAlignment - 1));
}

template <typename T>
EZ_ALWAYS_INLINE T ezMemoryUtils::AlignSize(T uiSize, T uiAlignment)
{
  return ((uiSize + (uiAlignment - 1)) & ~(uiAlignment - 1));
}

template <typename T>
EZ_ALWAYS_INLINE bool ezMemoryUtils::IsAligned(const T* pPtr, size_t uiAlignment)
{
  return (reinterpret_cast<size_t>(pPtr) & (uiAlignment - 1)) == 0;
}

template <typename T>
EZ_ALWAYS_INLINE bool ezMemoryUtils::IsSizeAligned(T uiSize, T uiAlignment)
{
  return (uiSize & (uiAlignment - 1)) == 0;
}

#undef EZ_CHECK_CLASS
