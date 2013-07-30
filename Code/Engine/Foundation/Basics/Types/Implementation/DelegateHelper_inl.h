
template <typename R EZ_COMMA_IF(ARG_COUNT) EZ_LIST(typename ARG, ARG_COUNT)>
class ezDelegate<R (EZ_LIST(ARG, ARG_COUNT))> : public ezDelegateBase
{
private:
  typedef ezDelegate<R (EZ_LIST(ARG, ARG_COUNT))> SelfType;

public:
  EZ_DECLARE_POD_TYPE();

  EZ_FORCE_INLINE ezDelegate() : m_pDispatchFunction(NULL)
  {
  }

  template <typename Method, typename Class>
  EZ_FORCE_INLINE ezDelegate(Method method, Class* pInstance)
  {
    EZ_CHECK_AT_COMPILETIME_MSG(sizeof(Method) <= DATA_SIZE, "Member function pointer must not be bigger than 16 bytes");
    EZ_ASSERT(ezMemoryUtils::IsAligned(&m_Data, EZ_ALIGNMENT_OF(Method)), "Wrong alignment. Expected %d bytes alignment", EZ_ALIGNMENT_OF(Method));

    memcpy(m_Data, &method, sizeof(Method));
    memset(m_Data + sizeof(Method), 0, DATA_SIZE - sizeof(Method));
    m_pInstance.m_Ptr = pInstance;
    m_pDispatchFunction = &DispatchToMethod<Method, Class>;
  }

  template <typename Method, typename Class>
  EZ_FORCE_INLINE ezDelegate(Method method, const Class* pInstance)
  {
    EZ_CHECK_AT_COMPILETIME_MSG(sizeof(Method) <= DATA_SIZE, "Member function pointer must not be bigger than 16 bytes");
    EZ_ASSERT(ezMemoryUtils::IsAligned(&m_Data, EZ_ALIGNMENT_OF(Method)), "Wrong alignment. Expected %d bytes alignment", EZ_ALIGNMENT_OF(Method));

    memcpy(m_Data, &method, sizeof(Method));
    memset(m_Data + sizeof(Method), 0, DATA_SIZE - sizeof(Method));
    m_pInstance.m_ConstPtr = pInstance;
    m_pDispatchFunction = &DispatchToConstMethod<Method, Class>;
  }

  template <typename Function>
  EZ_FORCE_INLINE ezDelegate(Function function)
  {
    EZ_ASSERT(ezMemoryUtils::IsAligned(&m_Data, EZ_ALIGNMENT_OF(Function)), "Wrong alignment. Expected %d bytes alignment", EZ_ALIGNMENT_OF(Function));

    memcpy(m_Data, &function, sizeof(Function));
    memset(m_Data + sizeof(Function), 0, DATA_SIZE - sizeof(Function));
    m_pDispatchFunction = &DispatchToFunction<Function>;
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  EZ_FORCE_INLINE ~ezDelegate()
  {
    m_pDispatchFunction = NULL;
  }
#endif

  EZ_FORCE_INLINE void operator=(const SelfType& other)
  {
    m_pInstance = other.m_pInstance;
    m_pDispatchFunction = other.m_pDispatchFunction;
    memcpy(m_Data, other.m_Data, DATA_SIZE);
  }

  EZ_FORCE_INLINE R operator()(EZ_PAIR_LIST(ARG, arg, ARG_COUNT))
  {
    EZ_ASSERT(m_pDispatchFunction != NULL, "Delegate is not bound.");
    return (*m_pDispatchFunction)(*this EZ_COMMA_IF(ARG_COUNT) EZ_LIST(arg, ARG_COUNT));
  }

  EZ_FORCE_INLINE bool operator==(const SelfType& other) const
  {
    return memcmp(m_Data, other.m_Data, DATA_SIZE) == 0 && m_pInstance.m_Ptr == other.m_pInstance.m_Ptr;
  }

  EZ_FORCE_INLINE bool operator!=(const SelfType& other) const
  {
    return !(*this == other);
  }

  EZ_FORCE_INLINE void* GetInstance() const
  {
    return m_pInstance.m_Ptr;
  }

private:
  template <typename Method, typename Class>
  static EZ_FORCE_INLINE R DispatchToMethod(SelfType& self EZ_COMMA_IF(ARG_COUNT) EZ_PAIR_LIST(ARG, arg, ARG_COUNT))
  {
    EZ_ASSERT(self.m_pInstance.m_Ptr != NULL, "Instance must not be null.");
    Method method = *reinterpret_cast<Method*>(&self.m_Data);
    return (static_cast<Class*>(self.m_pInstance.m_Ptr)->*method)(EZ_LIST(arg, ARG_COUNT));
  }

  template <typename Method, typename Class>
  static EZ_FORCE_INLINE R DispatchToConstMethod(SelfType& self EZ_COMMA_IF(ARG_COUNT) EZ_PAIR_LIST(ARG, arg, ARG_COUNT))
  {
    EZ_ASSERT(self.m_pInstance.m_ConstPtr != NULL, "Instance must not be null.");
    Method method = *reinterpret_cast<Method*>(&self.m_Data);
    return (static_cast<const Class*>(self.m_pInstance.m_ConstPtr)->*method)(EZ_LIST(arg, ARG_COUNT));
  }

  template <typename Function>
  static EZ_FORCE_INLINE R DispatchToFunction(SelfType& self EZ_COMMA_IF(ARG_COUNT) EZ_PAIR_LIST(ARG, arg, ARG_COUNT))
  {
    Function function = *reinterpret_cast<Function*>(&self.m_Data);
    return (*function)(EZ_LIST(arg, ARG_COUNT));
  }

  typedef R (*DispatchFunction)(SelfType& self EZ_COMMA_IF(ARG_COUNT) EZ_LIST(ARG, ARG_COUNT));
  DispatchFunction m_pDispatchFunction;

  enum { DATA_SIZE = 16 };
  ezUInt8 m_Data[DATA_SIZE];
};
