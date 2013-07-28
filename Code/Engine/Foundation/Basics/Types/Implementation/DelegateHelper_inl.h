
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
    EZ_CHECK_AT_COMPILETIME_MSG(sizeof(Method) <= sizeof(m_Data), "Member function pointer must not be bigger than 16 bytes");
    EZ_ASSERT(ezMemoryUtils::IsAligned(&m_Data, EZ_ALIGNMENT_OF(Method)), "Wrong alignment. Expected %d bytes alignment", EZ_ALIGNMENT_OF(Method));

    memcpy(m_Data, &method, sizeof(Method));
    memset(m_Data + sizeof(Method), 0, sizeof(m_Data) - sizeof(Method));
    m_pInstance.m_Ptr = pInstance;
    m_pDispatchFunction = &DispatchToMethod<Method, Class>;
  }

  template <typename Method, typename Class>
  EZ_FORCE_INLINE ezDelegate(Method method, const Class* pInstance)
  {
    EZ_CHECK_AT_COMPILETIME_MSG(sizeof(Method) <= sizeof(m_Data), "Member function pointer must not be bigger than 16 bytes");
    EZ_ASSERT(ezMemoryUtils::IsAligned(&m_Data, EZ_ALIGNMENT_OF(Method)), "Wrong alignment. Expected %d bytes alignment", EZ_ALIGNMENT_OF(Method));

    memcpy(m_Data, &method, sizeof(Method));
    memset(m_Data + sizeof(Method), 0, sizeof(m_Data) - sizeof(Method));
    m_pInstance.m_ConstPtr = pInstance;
    m_pDispatchFunction = &DispatchToConstMethod<Method, Class>;
  }

  template <typename Function>
  EZ_FORCE_INLINE ezDelegate(Function function)
  {
    EZ_ASSERT(ezMemoryUtils::IsAligned(&m_Data, EZ_ALIGNMENT_OF(Function)), "Wrong alignment. Expected %d bytes alignment", EZ_ALIGNMENT_OF(Function));

    memcpy(m_Data, &function, sizeof(Function));
    memset(m_Data + sizeof(Function), 0, sizeof(m_Data) - sizeof(Function));
    m_pDispatchFunction = &DispatchToFunction<Function>;
  }

  EZ_FORCE_INLINE void operator=(const ezDelegate& other)
  {
    m_pInstance = other.m_pInstance;
    m_pDispatchFunction = other.m_pDispatchFunction;
    memcpy(m_Data, other.m_Data, sizeof(m_Data));
  }

  EZ_FORCE_INLINE R operator()(EZ_PAIR_LIST(ARG, arg, ARG_COUNT))
  {
    EZ_ASSERT(m_pDispatchFunction != NULL, "Delegate is not bound.");
    return (*m_pDispatchFunction)(*this EZ_COMMA_IF(ARG_COUNT) EZ_LIST(arg, ARG_COUNT));
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

  ezUInt8 m_Data[16];
};
