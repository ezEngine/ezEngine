
#include <Foundation/Types/Implementation/DelegateHelper_inl.h>

template <typename Function>
EZ_ALWAYS_INLINE ezDelegate<Function> ezMakeDelegate(Function* pFunction)
{
  return ezDelegate<Function>(pFunction);
}

template <typename Method, typename Class>
EZ_ALWAYS_INLINE typename ezMakeDelegateHelper<Method>::DelegateType ezMakeDelegate(Method method, Class* pClass)
{
  return typename ezMakeDelegateHelper<Method>::DelegateType(method, pClass);
}
