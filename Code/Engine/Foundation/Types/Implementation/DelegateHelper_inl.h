
/// \brief [Internal] Storage for lambdas with captures in ezDelegate.
struct EZ_FOUNDATION_DLL ezLambdaDelegateStorageBase
{
  ezLambdaDelegateStorageBase() = default;
  virtual ~ezLambdaDelegateStorageBase() = default;
  virtual ezLambdaDelegateStorageBase* Clone(ezAllocatorBase* pAllocator) const = 0;
  virtual void InplaceCopy(ezUInt8* pBuffer) const = 0;
  virtual void InplaceMove(ezUInt8* pBuffer) = 0;

private:
  ezLambdaDelegateStorageBase(const ezLambdaDelegateStorageBase&) = delete;
  ezLambdaDelegateStorageBase& operator=(const ezLambdaDelegateStorageBase&) = delete;
  ezLambdaDelegateStorageBase(ezLambdaDelegateStorageBase&&) = delete;
  ezLambdaDelegateStorageBase& operator=(ezLambdaDelegateStorageBase&&) = delete;
};

template <typename Function>
struct ezLambdaDelegateStorage : public ezLambdaDelegateStorageBase
{
  ezLambdaDelegateStorage(Function&& func)
    : m_func(std::move(func))
  {
  }

private:
  template <typename = typename std::enable_if<std::is_copy_constructible<Function>::value>>
  ezLambdaDelegateStorage(const Function& func)
    : m_func(func)
  {
  }

public:
  virtual ezLambdaDelegateStorageBase* Clone(ezAllocatorBase* pAllocator) const override
  {
    if constexpr (std::is_copy_constructible<Function>::value)
    {
      return EZ_NEW(pAllocator, ezLambdaDelegateStorage<Function>, m_func);
    }
    else
    {
      EZ_REPORT_FAILURE("The ezDelegate stores a lambda that is not copyable. Copying this ezDelegate is not supported.");
      return nullptr;
    }
  }

  virtual void InplaceCopy(ezUInt8* pBuffer) const override
  {
    if constexpr (std::is_copy_constructible<Function>::value)
    {
      new (pBuffer) ezLambdaDelegateStorage<Function>(m_func);
    }
    else
    {
      EZ_REPORT_FAILURE("The ezDelegate stores a lambda that is not copyable. Copying this ezDelegate is not supported.");
    }
  }

  virtual void InplaceMove(ezUInt8* pBuffer) override
  {
    if constexpr (std::is_move_constructible<Function>::value)
    {
      new (pBuffer) ezLambdaDelegateStorage<Function>(std::move(m_func));
    }
    else
    {
      EZ_REPORT_FAILURE("The ezDelegate stores a lambda that is not movable. Moving this ezDelegate is not supported.");
    }
  }

  Function m_func;
};


template <typename R, class... Args, ezUInt32 DataSize>
struct ezDelegate<R(Args...), DataSize> : public ezDelegateBase
{
private:
  typedef ezDelegate<R(Args...), DataSize> SelfType;
  constexpr const void* HeapLambda() const { return reinterpret_cast<const void*>((size_t)-1); }
  constexpr const void* InplaceLambda() const { return reinterpret_cast<const void*>((size_t)-2); }

public:
  EZ_ALWAYS_INLINE ezDelegate()
    : m_pDispatchFunction(nullptr)
  {
  }

  EZ_ALWAYS_INLINE ezDelegate(const SelfType& other) { *this = other; }

  EZ_ALWAYS_INLINE ezDelegate(SelfType&& other) { *this = std::move(other); }

  /// \brief Constructs the delegate from a member function type and takes the class instance on which to call the function later.
  template <typename Method, typename Class>
  EZ_FORCE_INLINE ezDelegate(Method method, Class* pInstance)
  {
    CopyMemberFunctionToInplaceStorage(method);

    m_pInstance.m_Ptr = pInstance;
    m_pDispatchFunction = &DispatchToMethod<Method, Class>;
  }

  /// \brief Constructs the delegate from a member function type and takes the (const) class instance on which to call the function later.
  template <typename Method, typename Class>
  EZ_FORCE_INLINE ezDelegate(Method method, const Class* pInstance)
  {
    CopyMemberFunctionToInplaceStorage(method);

    m_pInstance.m_ConstPtr = pInstance;
    m_pDispatchFunction = &DispatchToConstMethod<Method, Class>;
  }

  /// \brief Constructs the delegate from a regular C function type.
  template <typename Function>
  EZ_FORCE_INLINE ezDelegate(Function function, ezAllocatorBase* pAllocator = ezFoundation::GetDefaultAllocator())
  {
    EZ_CHECK_AT_COMPILETIME_MSG(DataSize >= 16, "DataSize must be at least 16 bytes");

    // Pure function pointers or lambdas that can be cast into pure functions (no captures) can be
    // copied directly into the inplace storage of the delegate.
    // Lambdas with captures need to be wrapped into an ezLambdaDelegateStorage object as they can
    // capture non-pod or non-memmoveable data. This wrapper can also be stored inplace if it is small enough,
    // otherwise it will be heap allocated with the specified allocator.
    constexpr size_t functionSize = sizeof(Function);
    using signature = R(Args...);
    if constexpr (functionSize <= DataSize && std::is_assignable<signature*&, Function>::value)
    {
      CopyFunctionToInplaceStorage(function);

      m_pInstance.m_ConstPtr = nullptr;
      m_pDispatchFunction = &DispatchToFunction<Function>;
    }
    else
    {
      constexpr size_t storageSize = sizeof(ezLambdaDelegateStorage<Function>);
      if constexpr (storageSize <= DataSize)
      {
        m_pInstance.m_ConstPtr = InplaceLambda();
        new (m_Data) ezLambdaDelegateStorage<Function>(std::move(function));
        memset(m_Data + storageSize, 0, DataSize - storageSize);
        m_pDispatchFunction = &DispatchToInplaceLambda<Function>;
      }
      else
      {
        m_pInstance.m_ConstPtr = HeapLambda();
        m_pLambdaStorage = EZ_NEW(pAllocator, ezLambdaDelegateStorage<Function>, std::move(function));
        m_pAllocator = pAllocator;
        memset(m_Data + 2 * sizeof(void*), 0, DataSize - 2 * sizeof(void*));
        m_pDispatchFunction = &DispatchToHeapLambda<Function>;
      }
    }
  }

  EZ_ALWAYS_INLINE ~ezDelegate()
  {
    Invalidate();
  }

  /// \brief Copies the data from another delegate.
  EZ_FORCE_INLINE void operator=(const SelfType& other)
  {
    Invalidate();

    if (other.IsHeapLambda())
    {
      m_pAllocator = other.m_pAllocator;
      m_pLambdaStorage = other.m_pLambdaStorage->Clone(m_pAllocator);
    }
    else if (other.IsInplaceLambda())
    {
      auto pOtherLambdaStorage = reinterpret_cast<ezLambdaDelegateStorageBase*>(&other.m_Data);
      pOtherLambdaStorage->InplaceCopy(m_Data);
    }
    else
    {
      memcpy(m_Data, other.m_Data, DataSize);
    }

    m_pInstance = other.m_pInstance;
    m_pDispatchFunction = other.m_pDispatchFunction;
  }

  /// \brief Moves the data from another delegate.
  EZ_FORCE_INLINE void operator=(SelfType&& other)
  {
    Invalidate();
    m_pInstance = other.m_pInstance;
    m_pDispatchFunction = other.m_pDispatchFunction;

    if (other.IsInplaceLambda())
    {
      auto pOtherLambdaStorage = reinterpret_cast<ezLambdaDelegateStorageBase*>(&other.m_Data);
      pOtherLambdaStorage->InplaceMove(m_Data);
    }
    else
    {
      memcpy(m_Data, other.m_Data, DataSize);
    }

    other.m_pInstance.m_Ptr = nullptr;
    other.m_pDispatchFunction = nullptr;
    memset(other.m_Data, 0, DataSize);
  }

  /// \brief Resets a delegate to an invalid state.
  EZ_FORCE_INLINE void operator=(std::nullptr_t)
  {
    Invalidate();
  }

  /// \brief Function call operator. This will call the function that is bound to the delegate, or assert if nothing was bound.
  EZ_FORCE_INLINE R operator()(Args... params) const
  {
    EZ_ASSERT_DEBUG(m_pDispatchFunction != nullptr, "Delegate is not bound.");
    return (*m_pDispatchFunction)(*this, params...);
  }

  /// \brief This function only exists to make code compile, but it will assert when used. Use IsEqualIfNotHeapAllocated() instead.
  EZ_ALWAYS_INLINE bool operator==(const SelfType& other) const
  {
    EZ_REPORT_FAILURE("operator== for ezDelegate must not be used. Use IsEqualIfNotHeapAllocated() and read its documentation!");
    return false;
  }

  /// \brief Checks whether two delegates are bound to the exact same function, including the class instance.
  /// \note If \a this or \a other or both return false for IsComparable(), the function returns always false!
  /// Therefore, do not use this to search for delegates that are not comparable. ezEvent uses this function, but goes to great lengths to
  /// assert that it is used correctly. It is best to not use this function at all.
  EZ_ALWAYS_INLINE bool IsEqualIfComparable(const SelfType& other) const
  {
    return m_pInstance.m_Ptr == other.m_pInstance.m_Ptr && m_pDispatchFunction == other.m_pDispatchFunction &&
           memcmp(m_Data, other.m_Data, DataSize) == 0;
  }

  /// \brief Returns true when the delegate is bound to a valid non-nullptr function.
  EZ_ALWAYS_INLINE bool IsValid() const { return m_pDispatchFunction != nullptr; }

  /// \brief Resets a delegate to an invalid state.
  EZ_FORCE_INLINE void Invalidate()
  {
    m_pDispatchFunction = nullptr;
    if (IsHeapLambda())
    {
      EZ_DELETE(m_pAllocator, m_pLambdaStorage);
    }
    else if (IsInplaceLambda())
    {
      auto pLambdaStorage = reinterpret_cast<ezLambdaDelegateStorageBase*>(&m_Data);
      pLambdaStorage->~ezLambdaDelegateStorageBase();
    }

    m_pInstance.m_Ptr = nullptr;
    memset(m_Data, 0, DataSize);
  }

  /// \brief Returns the class instance that is used to call a member function pointer on.
  EZ_ALWAYS_INLINE void* GetClassInstance() const { return IsComparable() ? m_pInstance.m_Ptr : nullptr; }

  /// \brief Returns whether the delegate is comparable with other delegates of the same type. This is not the case for i.e. lambdas with captures.
  EZ_ALWAYS_INLINE bool IsComparable() const { return m_pInstance.m_ConstPtr < InplaceLambda(); } // [tested]

private:

  template <typename Function>
  EZ_FORCE_INLINE void CopyFunctionToInplaceStorage(Function function)
  {
    EZ_ASSERT_DEBUG(ezMemoryUtils::IsAligned(&m_Data, EZ_ALIGNMENT_OF(Function)), "Wrong alignment. Expected {0} bytes alignment",
      EZ_ALIGNMENT_OF(Function));

    memcpy(m_Data, &function, sizeof(Function));
    memset(m_Data + sizeof(Function), 0, DataSize - sizeof(Function));
  }

  template <typename Method>
  EZ_FORCE_INLINE void CopyMemberFunctionToInplaceStorage(Method method)
  {
    EZ_CHECK_AT_COMPILETIME_MSG(DataSize >= 16, "DataSize must be at least 16 bytes");
    EZ_CHECK_AT_COMPILETIME_MSG(sizeof(Method) <= DataSize, "Member function pointer must not be bigger than 16 bytes");

    CopyFunctionToInplaceStorage(method);

    // Member Function Pointers in MSVC are 12 bytes in size and have 4 byte padding
    // MSVC builds a member function pointer on the stack writing only 12 bytes and then copies it
    // to the final location by copying 16 bytes. Thus the 4 byte padding get a random value (whatever is on the stack at that time).
    // To make the delegate comparable by memcmp we zero out those 4 byte padding.
#if EZ_ENABLED(EZ_COMPILER_MSVC)
    *reinterpret_cast<ezUInt32*>(m_Data + 12) = 0;
#endif
  }

  EZ_ALWAYS_INLINE bool IsInplaceLambda() const { return m_pInstance.m_ConstPtr == InplaceLambda(); }
  EZ_ALWAYS_INLINE bool IsHeapLambda() const { return m_pInstance.m_ConstPtr == HeapLambda(); }

  template <typename Method, typename Class>
  static EZ_FORCE_INLINE R DispatchToMethod(const SelfType& self, Args... params)
  {
    EZ_ASSERT_DEBUG(self.m_pInstance.m_Ptr != nullptr, "Instance must not be null.");
    Method method = *reinterpret_cast<Method*>(&self.m_Data);
    return (static_cast<Class*>(self.m_pInstance.m_Ptr)->*method)(params...);
  }

  template <typename Method, typename Class>
  static EZ_FORCE_INLINE R DispatchToConstMethod(const SelfType& self, Args... params)
  {
    EZ_ASSERT_DEBUG(self.m_pInstance.m_ConstPtr != nullptr, "Instance must not be null.");
    Method method = *reinterpret_cast<Method*>(&self.m_Data);
    return (static_cast<const Class*>(self.m_pInstance.m_ConstPtr)->*method)(params...);
  }

  template <typename Function>
  static EZ_ALWAYS_INLINE R DispatchToFunction(const SelfType& self, Args... params)
  {
    return (*reinterpret_cast<Function*>(&self.m_Data))(params...);
  }

  template <typename Function>
  static EZ_ALWAYS_INLINE R DispatchToHeapLambda(const SelfType& self, Args... params)
  {
    return static_cast<ezLambdaDelegateStorage<Function>*>(self.m_pLambdaStorage)->m_func(params...);
  }

  template <typename Function>
  static EZ_ALWAYS_INLINE R DispatchToInplaceLambda(const SelfType& self, Args... params)
  {
    return reinterpret_cast<ezLambdaDelegateStorage<Function>*>(&self.m_Data)->m_func(params...);
  }

  typedef R (*DispatchFunction)(const SelfType& self, Args...);
  DispatchFunction m_pDispatchFunction;

  union
  {
    mutable ezUInt8 m_Data[DataSize];
    struct
    {
      ezLambdaDelegateStorageBase* m_pLambdaStorage;
      ezAllocatorBase* m_pAllocator;
    };
  };
};

template <typename T>
struct ezMakeDelegateHelper
{
};

template <typename Class, typename R, typename... Args>
struct ezMakeDelegateHelper<R (Class::*)(Args...)>
{
  typedef ezDelegate<R(Args...)> DelegateType;
};

template <typename Class, typename R, typename... Args>
struct ezMakeDelegateHelper<R (Class::*)(Args...) const>
{
  typedef ezDelegate<R(Args...)> DelegateType;
};
