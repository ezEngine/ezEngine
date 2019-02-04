
#define EZ_CHECK_CLASS(T)                                                                                                                  \
  EZ_CHECK_AT_COMPILETIME_MSG(!std::is_trivial<T>::value, "Pod type is treated as class, did you forget EZ_DECLARE_POD_TYPE?")

// public methods: redirect to implementation
template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::Construct(T* pDestination, size_t uiCount)
{
  // Default constructor is always called, so that debug helper initializations (e.g. ezVec3 initializes to NaN) take place.
  // Note that destructor is ONLY called for class types.
  // Special case for c++11 to prevent default construction of "real" Pod types, also avoids warnings on msvc
  Construct(pDestination, uiCount, ezTraitInt < ezIsPodType<T>::value && std::is_trivial<T>::value > ());
}

template <typename T>
EZ_ALWAYS_INLINE ezMemoryUtils::ConstructorFunction ezMemoryUtils::MakeConstructorFunction()
{
  return MakeConstructorFunction<T>(ezTraitInt < ezIsPodType<T>::value && std::is_trivial<T>::value > ());
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::DefaultConstruct(T* pDestination, size_t uiCount)
{
  for (size_t i = 0; i < uiCount; i++)
  {
    ::new (pDestination + i) T();
  }
}

template <typename T>
EZ_ALWAYS_INLINE ezMemoryUtils::ConstructorFunction ezMemoryUtils::MakeDefaultConstructorFunction()
{
  struct Helper
  {
    static void DefaultConstruct(void* pDestination) { ezMemoryUtils::DefaultConstruct(static_cast<T*>(pDestination), 1); }
  };

  return &Helper::DefaultConstruct;
}

template <typename Destination, typename Source>
EZ_ALWAYS_INLINE void ezMemoryUtils::CopyConstruct(Destination* pDestination, const Source& copy, size_t uiCount)
{
  CopyConstruct<Destination, Source>(pDestination, copy, uiCount, ezIsPodType<Destination>());
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::CopyConstructArray(T* pDestination, const T* pSource, size_t uiCount)
{
  EZ_ASSERT_DEV(pDestination < pSource || pSource + uiCount <= pDestination, "Memory regions must not overlap when using CopyConstruct.");
  CopyConstructArray<T>(pDestination, pSource, uiCount, ezIsPodType<T>());
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
  EZ_ASSERT_DEV(pDestination < pSource || pSource + uiCount <= pDestination, "Memory regions must not overlap when using MoveConstruct.");

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
  typedef typename std::is_rvalue_reference<decltype(source)>::type IsRValueRef;
  CopyOrMoveConstruct<Destination, Source>(pDestination, std::forward<Source>(source), IsRValueRef());
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::RelocateConstruct(T* pDestination, T* pSource, size_t uiCount)
{
  EZ_ASSERT_DEV(pDestination < pSource || pSource + uiCount <= pDestination,
                "Memory regions must not overlap when using RelocateConstruct.");
  RelocateConstruct(pDestination, pSource, uiCount, ezGetTypeClass<T>());
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::Destruct(T* pDestination, size_t uiCount)
{
  Destruct(pDestination, uiCount, ezIsPodType<T>());
}

template <typename T>
EZ_ALWAYS_INLINE ezMemoryUtils::DestructorFunction ezMemoryUtils::MakeDestructorFunction()
{
  return MakeDestructorFunction<T>(ezIsPodType<T>());
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::Copy(T* pDestination, const T* pSource, size_t uiCount)
{
  EZ_ASSERT_DEV(pDestination < pSource || pSource + uiCount <= pDestination,
                "Memory regions must not overlap when using Copy. Use CopyOverlapped instead.");
  Copy(pDestination, pSource, uiCount, ezIsPodType<T>());
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::CopyOverlapped(T* pDestination, const T* pSource, size_t uiCount)
{
  CopyOverlapped(pDestination, pSource, uiCount, ezIsPodType<T>());
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::Relocate(T* pDestination, T* pSource, size_t uiCount)
{
  EZ_ASSERT_DEV(pDestination < pSource || pSource + uiCount <= pDestination, "Memory regions must not overlap when using Relocate.");
  Relocate(pDestination, pSource, uiCount, ezGetTypeClass<T>());
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::RelocateOverlapped(T* pDestination, T* pSource, size_t uiCount)
{
  RelocateOverlapped(pDestination, pSource, uiCount, ezGetTypeClass<T>());
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::Prepend(T* pDestination, const T& source, size_t uiCount)
{
  Prepend(pDestination, source, uiCount, ezGetTypeClass<T>());
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::Prepend(T* pDestination, T&& source, size_t uiCount)
{
  Prepend(pDestination, std::move(source), uiCount, ezGetTypeClass<T>());
}

template <typename T>
EZ_ALWAYS_INLINE bool ezMemoryUtils::IsEqual(const T* a, const T* b, size_t uiCount /*= 1*/)
{
  return IsEqual(a, b, uiCount, ezIsPodType<T>());
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::ZeroFill(T* pDestination, size_t uiCount)
{
  memset(pDestination, 0, uiCount * sizeof(T));
}

template <typename T, size_t N>
EZ_ALWAYS_INLINE void ezMemoryUtils::ZeroFill(T (&destination)[N])
{
  return ZeroFill(destination, N);
}

template <typename T>
EZ_ALWAYS_INLINE ezInt32 ezMemoryUtils::ByteCompare(const T* a, const T* b, size_t uiCount /*= 1*/)
{
  return memcmp(a, b, uiCount * sizeof(T));
}

template <typename T>
EZ_ALWAYS_INLINE T* ezMemoryUtils::AddByteOffset(T* ptr, ptrdiff_t iOffset)
{
  return reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(ptr) + iOffset);
}

template <typename T>
EZ_ALWAYS_INLINE T* ezMemoryUtils::Align(T* ptr, size_t uiAlignment)
{
  return reinterpret_cast<T*>(reinterpret_cast<size_t>(ptr) & ~(uiAlignment - 1));
}

template <typename T>
EZ_ALWAYS_INLINE T ezMemoryUtils::AlignSize(T uiSize, T uiAlignment)
{
  return ((uiSize + (uiAlignment - 1)) & ~(uiAlignment - 1));
}

template <typename T>
EZ_ALWAYS_INLINE bool ezMemoryUtils::IsAligned(const T* ptr, size_t uiAlignment)
{
  return (reinterpret_cast<size_t>(ptr) & (uiAlignment - 1)) == 0;
}

template <typename T>
EZ_ALWAYS_INLINE bool ezMemoryUtils::IsSizeAligned(T uiSize, T uiAlignment)
{
  return (uiSize & (uiAlignment - 1)) == 0;
}

// private methods

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::Construct(T* pDestination, size_t uiCount, ezTypeIsPod)
{
  EZ_CHECK_AT_COMPILETIME_MSG(std::is_trivial<T>::value, "This method should only be called for 'real' pod aka trivial types");
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::Construct(T* pDestination, size_t uiCount, ezTypeIsClass)
{
  EZ_CHECK_CLASS(T);

  for (size_t i = 0; i < uiCount; i++)
  {
    ::new (pDestination + i) T();
  }
}

template <typename T>
EZ_ALWAYS_INLINE ezMemoryUtils::ConstructorFunction ezMemoryUtils::MakeConstructorFunction(ezTypeIsPod)
{
  EZ_CHECK_AT_COMPILETIME_MSG(std::is_trivial<T>::value, "This method should only be called for 'real' pod aka trivial types");
  return nullptr;
}

template <typename T>
EZ_ALWAYS_INLINE ezMemoryUtils::ConstructorFunction ezMemoryUtils::MakeConstructorFunction(ezTypeIsClass)
{
  EZ_CHECK_CLASS(T);

  struct Helper
  {
    static void Construct(void* pDestination) { ezMemoryUtils::Construct(static_cast<T*>(pDestination), 1, ezTypeIsClass()); }
  };

  return &Helper::Construct;
}

template <typename Destination, typename Source>
EZ_ALWAYS_INLINE void ezMemoryUtils::CopyConstruct(Destination* pDestination, const Source& copy, size_t uiCount, ezTypeIsPod)
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

template <typename Destination, typename Source>
EZ_ALWAYS_INLINE void ezMemoryUtils::CopyConstruct(Destination* pDestination, const Source& copy, size_t uiCount, ezTypeIsClass)
{
  EZ_CHECK_CLASS(Destination);

  for (size_t i = 0; i < uiCount; i++)
  {
    ::new (pDestination + i) Destination(copy); // Note that until now copy has not been converted to Destination. This allows for calling
                                                // specialized constructors if available.
  }
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::CopyConstructArray(T* pDestination, const T* pSource, size_t uiCount, ezTypeIsPod)
{
  memcpy(pDestination, pSource, uiCount * sizeof(T));
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::CopyConstructArray(T* pDestination, const T* pSource, size_t uiCount, ezTypeIsClass)
{
  EZ_CHECK_CLASS(T);

  for (size_t i = 0; i < uiCount; i++)
  {
    ::new (pDestination + i) T(pSource[i]);
  }
}

template <typename Destination, typename Source>
EZ_ALWAYS_INLINE void ezMemoryUtils::CopyOrMoveConstruct(Destination* pDestination, const Source& source, NotRValueReference)
{
  CopyConstruct<Destination, Source>(pDestination, source, 1);
}

template <typename Destination, typename Source>
EZ_ALWAYS_INLINE void ezMemoryUtils::CopyOrMoveConstruct(Destination* pDestination, Source&& source, IsRValueReference)
{
  static_assert(std::is_rvalue_reference<decltype(source)>::value,
                "Implementation Error: This version of CopyOrMoveConstruct should only be called with a rvalue reference!");
  ::new (pDestination) Destination(std::move(source));
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::RelocateConstruct(T* pDestination, T* pSource, size_t uiCount, ezTypeIsPod)
{
  memcpy(pDestination, pSource, uiCount * sizeof(T));
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::RelocateConstruct(T* pDestination, T* pSource, size_t uiCount, ezTypeIsMemRelocatable)
{
  memcpy(pDestination, pSource, uiCount * sizeof(T));
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::RelocateConstruct(T* pDestination, T* pSource, size_t uiCount, ezTypeIsClass)
{
  EZ_CHECK_CLASS(T);

  for (size_t i = 0; i < uiCount; i++)
  {
    // Note that this calls the move constructor only if available and will copy otherwise.
    ::new (pDestination + i) T(std::move(pSource[i]));
  }

  Destruct(pSource, uiCount, ezTypeIsClass());
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::Destruct(T* pDestination, size_t uiCount, ezTypeIsPod)
{
  // Nothing to do here. See Construct of for more info.
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::Destruct(T* pDestination, size_t uiCount, ezTypeIsClass)
{
  EZ_CHECK_CLASS(T);

  for (size_t i = uiCount; i-- > 0;)
  {
    pDestination[i].~T();
  }
}

template <typename T>
EZ_ALWAYS_INLINE ezMemoryUtils::DestructorFunction ezMemoryUtils::MakeDestructorFunction(ezTypeIsPod)
{
  return nullptr;
}

template <typename T>
EZ_ALWAYS_INLINE ezMemoryUtils::DestructorFunction ezMemoryUtils::MakeDestructorFunction(ezTypeIsClass)
{
  EZ_CHECK_CLASS(T);

  struct Helper
  {
    static void Destruct(void* pDestination) { ezMemoryUtils::Destruct(static_cast<T*>(pDestination), 1, ezTypeIsClass()); }
  };

  return &Helper::Destruct;
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::Copy(T* pDestination, const T* pSource, size_t uiCount, ezTypeIsPod)
{
  memcpy(pDestination, pSource, uiCount * sizeof(T));
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::Copy(T* pDestination, const T* pSource, size_t uiCount, ezTypeIsClass)
{
  EZ_CHECK_CLASS(T);

  for (size_t i = 0; i < uiCount; i++)
  {
    pDestination[i] = pSource[i];
  }
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::CopyOverlapped(T* pDestination, const T* pSource, size_t uiCount, ezTypeIsPod)
{
  memmove(pDestination, pSource, uiCount * sizeof(T));
}

template <typename T>
inline void ezMemoryUtils::CopyOverlapped(T* pDestination, const T* pSource, size_t uiCount, ezTypeIsClass)
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
    for (size_t i = uiCount; i-- > 0;)
    {
      pDestination[i] = pSource[i];
    }
  }
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::Relocate(T* pDestination, T* pSource, size_t uiCount, ezTypeIsPod)
{
  memcpy(pDestination, pSource, uiCount * sizeof(T));
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::Relocate(T* pDestination, T* pSource, size_t uiCount, ezTypeIsMemRelocatable)
{
  memcpy(pDestination, pSource, uiCount * sizeof(T));
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::Relocate(T* pDestination, T* pSource, size_t uiCount, ezTypeIsClass)
{
  EZ_CHECK_CLASS(T);

  for (size_t i = 0; i < uiCount; i++)
  {
    // Note that this calls the move constructor only if available and will copy otherwise.
    pDestination[i] = std::move(pSource[i]);
  }

  Destruct(pSource, uiCount, ezTypeIsClass());
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::RelocateOverlapped(T* pDestination, T* pSource, size_t uiCount, ezTypeIsPod)
{
  memmove(pDestination, pSource, uiCount * sizeof(T));
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::RelocateOverlapped(T* pDestination, T* pSource, size_t uiCount, ezTypeIsMemRelocatable)
{
  if (pDestination < pSource)
  {
    size_t uiDestructCount = pSource - pDestination;
    Destruct(pDestination, uiDestructCount, ezTypeIsClass());
  }
  else
  {
    size_t uiDestructCount = pDestination - pSource;
    Destruct(pSource + uiCount, uiDestructCount, ezTypeIsClass());
  }
  memmove(pDestination, pSource, uiCount * sizeof(T));
}

template <typename T>
inline void ezMemoryUtils::RelocateOverlapped(T* pDestination, T* pSource, size_t uiCount, ezTypeIsClass)
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
    Destruct(pSource + uiCount - uiDestructCount, uiDestructCount, ezTypeIsClass());
  }
  else
  {
    for (size_t i = uiCount; i-- > 0;)
    {
      pDestination[i] = std::move(pSource[i]);
    }

    size_t uiDestructCount = pDestination - pSource;
    Destruct(pSource, uiDestructCount, ezTypeIsClass());
  }
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::Prepend(T* pDestination, const T& source, size_t uiCount, ezTypeIsPod)
{
  memmove(pDestination + 1, pDestination, uiCount * sizeof(T));
  CopyConstruct(pDestination, source, 1, ezTypeIsPod());
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::Prepend(T* pDestination, const T& source, size_t uiCount, ezTypeIsMemRelocatable)
{
  memmove(pDestination + 1, pDestination, uiCount * sizeof(T));
  CopyConstruct(pDestination, source, 1, ezTypeIsClass());
}

template <typename T>
inline void ezMemoryUtils::Prepend(T* pDestination, const T& source, size_t uiCount, ezTypeIsClass)
{
  EZ_CHECK_CLASS(T);

  if (uiCount > 0)
  {
    MoveConstruct(pDestination + uiCount, std::move(pDestination[uiCount - 1]));

    for (size_t i = uiCount - 1; i-- > 0;)
    {
      pDestination[i + 1] = std::move(pDestination[i]);
    }

    *pDestination = source;
  }
  else
  {
    CopyConstruct(pDestination, source, 1, ezTypeIsClass());
  }
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::Prepend(T* pDestination, T&& source, size_t uiCount, ezTypeIsPod)
{
  memmove(pDestination + 1, pDestination, uiCount * sizeof(T));
  MoveConstruct(pDestination, std::move(source));
}

template <typename T>
EZ_ALWAYS_INLINE void ezMemoryUtils::Prepend(T* pDestination, T&& source, size_t uiCount, ezTypeIsMemRelocatable)
{
  memmove(pDestination + 1, pDestination, uiCount * sizeof(T));
  MoveConstruct(pDestination, std::move(source));
}

template <typename T>
inline void ezMemoryUtils::Prepend(T* pDestination, T&& source, size_t uiCount, ezTypeIsClass)
{
  EZ_CHECK_CLASS(T);

  if (uiCount > 0)
  {
    MoveConstruct(pDestination + uiCount, std::move(pDestination[uiCount - 1]));

    for (size_t i = uiCount - 1; i-- > 0;)
    {
      pDestination[i + 1] = std::move(pDestination[i]);
    }

    *pDestination = std::move(source);
  }
  else
  {
    MoveConstruct(pDestination, std::move(source));
  }
}

template <typename T>
EZ_ALWAYS_INLINE bool ezMemoryUtils::IsEqual(const T* a, const T* b, size_t uiCount, ezTypeIsPod)
{
  return memcmp(a, b, uiCount * sizeof(T)) == 0;
}

template <typename T>
EZ_ALWAYS_INLINE bool ezMemoryUtils::IsEqual(const T* a, const T* b, size_t uiCount, ezTypeIsClass)
{
  EZ_CHECK_CLASS(T);

  for (size_t i = 0; i < uiCount; i++)
  {
    if (!(a[i] == b[i]))
      return false;
  }
  return true;
}


#undef EZ_CHECK_CLASS

