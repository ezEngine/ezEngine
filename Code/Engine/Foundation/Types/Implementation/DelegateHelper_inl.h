
/// \brief [Internal] Storage for lambdas with captures in ezDelegate.
struct EZ_FOUNDATION_DLL ezLambdaDelegateStorageBase
{
  ezLambdaDelegateStorageBase() = default;
  virtual ~ezLambdaDelegateStorageBase() = default;
  virtual ezLambdaDelegateStorageBase* Clone() const = 0;
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
  template < typename = typename std::enable_if< std::is_copy_constructible<Function>::value >>
  ezLambdaDelegateStorage(const Function& func)
    : m_func(func)
  {
  }

public:
  virtual ezLambdaDelegateStorageBase* Clone() const override
  {
    if constexpr (std::is_copy_constructible<Function>::value)
    {
      return EZ_DEFAULT_NEW(ezLambdaDelegateStorage<Function>, m_func);
    }
    else
    {
      EZ_REPORT_FAILURE("The ezDelegate stores a lambda that is not copyable. Copying this ezDelegate is not supported.");
      return nullptr;
    }
  }
  Function m_func;
};


template <typename R, class... Args>
struct ezDelegate<R(Args...)> : public ezDelegateBase
{
private:
  typedef ezDelegate<R(Args...)> SelfType;
  constexpr const void* HeapAllocated() const { return reinterpret_cast<const void*>((size_t)-1); }

public:
  EZ_DECLARE_MEM_RELOCATABLE_TYPE();

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
    EZ_CHECK_AT_COMPILETIME_MSG(sizeof(Method) <= DATA_SIZE, "Member function pointer must not be bigger than 16 bytes");
    EZ_ASSERT_DEBUG(
      ezMemoryUtils::IsAligned(&m_Data, EZ_ALIGNMENT_OF(Method)), "Wrong alignment. Expected {0} bytes alignment", EZ_ALIGNMENT_OF(Method));

    memcpy(m_Data, &method, sizeof(Method));
    memset(m_Data + sizeof(Method), 0, DATA_SIZE - sizeof(Method));

// Member Function Pointers in MSVC are 12 bytes in size and have 4 byte padding
// MSVC builds a member function pointer on the stack writing only 12 bytes and then copies it
// to the final location by copying 16 bytes. Thus the 4 byte padding get a random value (whatever is on the stack at that time).
// To make the delegate compareable by memcmp we zero out those 4 byte padding.
#if EZ_ENABLED(EZ_COMPILER_MSVC)
    *reinterpret_cast<ezUInt32*>(m_Data + 12) = 0;
#endif

    m_pInstance.m_Ptr = pInstance;
    m_pDispatchFunction = &DispatchToMethod<Method, Class>;
  }

  /// \brief Constructs the delegate from a member function type and takes the (const) class instance on which to call the function later.
  template <typename Method, typename Class>
  EZ_FORCE_INLINE ezDelegate(Method method, const Class* pInstance)
  {
    EZ_CHECK_AT_COMPILETIME_MSG(sizeof(Method) <= DATA_SIZE, "Member function pointer must not be bigger than 16 bytes");
    EZ_ASSERT_DEBUG(
      ezMemoryUtils::IsAligned(&m_Data, EZ_ALIGNMENT_OF(Method)), "Wrong alignment. Expected {0} bytes alignment", EZ_ALIGNMENT_OF(Method));

    memcpy(m_Data, &method, sizeof(Method));
    memset(m_Data + sizeof(Method), 0, DATA_SIZE - sizeof(Method));

// Member Function Pointers in MSVC are 12 bytes in size and have 4 byte padding
// MSVC builds a member function pointer on the stack writing only 12 bytes and then copies it
// to the final location by copying 16 bytes. Thus the 4 byte padding get a random value (whatever is on the stack at that time).
// To make the delegate compareable by memcmp we zero out those 4 byte padding.
#if EZ_ENABLED(EZ_COMPILER_MSVC)
    *reinterpret_cast<ezUInt32*>(m_Data + 12) = 0;
#endif

    m_pInstance.m_ConstPtr = pInstance;
    m_pDispatchFunction = &DispatchToConstMethod<Method, Class>;
  }

  /// \brief Constructs the delegate from a regular C function type.
  template <typename Function>
  EZ_FORCE_INLINE ezDelegate(Function function)
  {
    // Only memcpy pure function pointers or lambdas that can be cast into pure functions.
    // Not lambdas with captures, as they can capture non-pod, non-memmoveable data.
    using signature = R(Args...);
    if constexpr (sizeof(Function) <= DATA_SIZE && std::is_assignable<signature*&, Function>::value)
    {
      EZ_ASSERT_DEBUG(ezMemoryUtils::IsAligned(&m_Data, EZ_ALIGNMENT_OF(Function)), "Wrong alignment. Expected {0} bytes alignment",
        EZ_ALIGNMENT_OF(Function));

      m_pInstance.m_ConstPtr = nullptr;
      memcpy(m_Data, &function, sizeof(Function));
      memset(m_Data + sizeof(Function), 0, DATA_SIZE - sizeof(Function));
      m_pDispatchFunction = &DispatchToFunction<Function>;
    }
    else
    {
      m_pInstance.m_ConstPtr = HeapAllocated();
      ezLambdaDelegateStorageBase* storage = EZ_DEFAULT_NEW(ezLambdaDelegateStorage<Function>, std::move(function));
      memcpy(m_Data, &storage, sizeof(storage));
      memset(m_Data + sizeof(storage), 0, DATA_SIZE - sizeof(storage));
      m_pDispatchFunction = &DispatchToLambdaStorageFunction<Function>;
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
    m_pInstance = other.m_pInstance;
    m_pDispatchFunction = other.m_pDispatchFunction;
    memcpy(m_Data, other.m_Data, DATA_SIZE);
    if (other.IsHeapAllocated())
    {
      ezLambdaDelegateStorageBase* otherStorage = *reinterpret_cast<ezLambdaDelegateStorageBase**>(other.m_Data);
      ezLambdaDelegateStorageBase* storage = otherStorage->Clone();
      memcpy(m_Data, &storage, sizeof(storage));
    }
  }

  /// \brief Moves the data from another delegate.
  EZ_FORCE_INLINE void operator=(SelfType&& other)
  {
    Invalidate();
    m_pInstance = other.m_pInstance;
    m_pDispatchFunction = other.m_pDispatchFunction;
    memcpy(m_Data, other.m_Data, DATA_SIZE);

    other.m_pInstance.m_Ptr = nullptr;
    other.m_pDispatchFunction = nullptr;
    memset(other.m_Data, 0, DATA_SIZE);
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
  /// \note If \a this or \a other or both return true for IsHeapAllocated(), the function always false!
  /// Therefore, do not use this to search for delegates that are heap allocated. ezEvent uses this function, but goes to great lengths to
  /// assert that it is used correctly. It is best to not use this function at all.
  EZ_ALWAYS_INLINE bool IsEqualIfNotHeapAllocated(const SelfType& other) const
  {
    return m_pInstance.m_Ptr == other.m_pInstance.m_Ptr && m_pDispatchFunction == other.m_pDispatchFunction &&
           memcmp(m_Data, other.m_Data, DATA_SIZE) == 0;
  }

  /// \brief Returns true when the delegate is bound to a valid non-nullptr function.
  EZ_ALWAYS_INLINE bool IsValid() const { return m_pDispatchFunction != nullptr; }

  /// \brief Resets a delegate to an invalid state.
  EZ_ALWAYS_INLINE void Invalidate()
  {
    m_pDispatchFunction = nullptr;
    if (IsHeapAllocated())
    {
      FreeHeapData();
    }
  }

  /// \brief Returns the class instance that is used to call a member function pointer on.
  EZ_ALWAYS_INLINE void* GetClassInstance() const { return IsHeapAllocated() ? nullptr : m_pInstance.m_Ptr; }

  /// \brief Returns whether the target function is stored on the heap, i.e. a lambda with captures.
  EZ_ALWAYS_INLINE bool IsHeapAllocated() const { return m_pInstance.m_ConstPtr == HeapAllocated(); } // [tested]

private:
  void FreeHeapData()
  {
    ezLambdaDelegateStorageBase* data = *reinterpret_cast<ezLambdaDelegateStorageBase**>(m_Data);
    EZ_DEFAULT_DELETE(data);
    m_pInstance.m_ConstPtr = 0;
  }

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
  static EZ_ALWAYS_INLINE R DispatchToLambdaStorageFunction(const SelfType& self, Args... params)
  {
    return (*reinterpret_cast<ezLambdaDelegateStorage<Function>**>(self.m_Data))->m_func(params...);
  }

  typedef R (*DispatchFunction)(const SelfType& self, Args...);
  DispatchFunction m_pDispatchFunction;

  enum
  {
    DATA_SIZE = 16
  };
  mutable ezUInt8 m_Data[DATA_SIZE];
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
