
template <typename R, class... Args>
struct ezDelegate<R(Args...)> : public ezDelegateBase
{
private:
  typedef ezDelegate<R(Args...)> SelfType;

public:
  EZ_DECLARE_POD_TYPE();

  EZ_ALWAYS_INLINE ezDelegate()
      : m_pDispatchFunction(nullptr)
  {
  }

  /// \brief Constructs the delegate from a member function type and takes the class instance on which to call the function later.
  template <typename Method, typename Class>
  EZ_FORCE_INLINE ezDelegate(Method method, Class* pInstance)
  {
    EZ_CHECK_AT_COMPILETIME_MSG(sizeof(Method) <= DATA_SIZE, "Member function pointer must not be bigger than 16 bytes");
    EZ_ASSERT_DEBUG(ezMemoryUtils::IsAligned(&m_Data, EZ_ALIGNMENT_OF(Method)), "Wrong alignment. Expected {0} bytes alignment",
                    EZ_ALIGNMENT_OF(Method));

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
    EZ_ASSERT_DEBUG(ezMemoryUtils::IsAligned(&m_Data, EZ_ALIGNMENT_OF(Method)), "Wrong alignment. Expected {0} bytes alignment",
                    EZ_ALIGNMENT_OF(Method));

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
    EZ_CHECK_AT_COMPILETIME_MSG(sizeof(Function) <= DATA_SIZE, "Function object must not be bigger than 16 bytes");
    EZ_ASSERT_DEBUG(ezMemoryUtils::IsAligned(&m_Data, EZ_ALIGNMENT_OF(Function)), "Wrong alignment. Expected {0} bytes alignment",
                    EZ_ALIGNMENT_OF(Function));

    memcpy(m_Data, &function, sizeof(Function));
    memset(m_Data + sizeof(Function), 0, DATA_SIZE - sizeof(Function));
    m_pDispatchFunction = &DispatchToFunction<Function>;
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  EZ_ALWAYS_INLINE ~ezDelegate() { m_pDispatchFunction = nullptr; }
#endif

  /// \brief Copies the data from another delegate.
  EZ_FORCE_INLINE void operator=(const SelfType& other)
  {
    m_pInstance = other.m_pInstance;
    m_pDispatchFunction = other.m_pDispatchFunction;
    memcpy(m_Data, other.m_Data, DATA_SIZE);
  }

  /// \brief Resets a delegate to an invalid state.
  EZ_FORCE_INLINE void operator=(std::nullptr_t) { m_pDispatchFunction = nullptr; }

  /// \brief Function call operator. This will call the function that is bound to the delegate, or assert if nothing was bound.
  EZ_FORCE_INLINE R operator()(Args... params) const
  {
    EZ_ASSERT_DEBUG(m_pDispatchFunction != nullptr, "Delegate is not bound.");
    return (*m_pDispatchFunction)(*this, params...);
  }

  /// \brief Checks whether two delegates are bound to the exact same function, including the class instance.
  EZ_ALWAYS_INLINE bool operator==(const SelfType& other) const
  {
    return m_pInstance.m_Ptr == other.m_pInstance.m_Ptr && m_pDispatchFunction == other.m_pDispatchFunction &&
           memcmp(m_Data, other.m_Data, DATA_SIZE) == 0;
  }

  /// \brief Checks whether two delegates are bound to the exact same function, including the class instance.
  EZ_ALWAYS_INLINE bool operator!=(const SelfType& other) const { return !(*this == other); }

  /// \brief Returns true when the delegate is bound to a valid non-nullptr function.
  EZ_ALWAYS_INLINE bool IsValid() const { return m_pDispatchFunction != nullptr; }

  /// \brief Resets a delegate to an invalid state.
  EZ_ALWAYS_INLINE void Invalidate() { m_pDispatchFunction = nullptr; }

  /// \brief Returns the class instance that is used to call a member function pointer on.
  EZ_ALWAYS_INLINE void* GetClassInstance() const { return m_pInstance.m_Ptr; }

private:
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

