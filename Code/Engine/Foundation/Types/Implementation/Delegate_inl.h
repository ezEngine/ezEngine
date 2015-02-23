
template <typename T>
struct ezMakeDelegateHelper
{

};

#define ARG_COUNT 0
#include <Foundation/Types/Implementation/DelegateHelper_inl.h>
#undef ARG_COUNT

#define ARG_COUNT 1
#include <Foundation/Types/Implementation/DelegateHelper_inl.h>
#undef ARG_COUNT

#define ARG_COUNT 2
#include <Foundation/Types/Implementation/DelegateHelper_inl.h>
#undef ARG_COUNT

#define ARG_COUNT 3
#include <Foundation/Types/Implementation/DelegateHelper_inl.h>
#undef ARG_COUNT

#define ARG_COUNT 4
#include <Foundation/Types/Implementation/DelegateHelper_inl.h>
#undef ARG_COUNT

#define ARG_COUNT 5
#include <Foundation/Types/Implementation/DelegateHelper_inl.h>
#undef ARG_COUNT

#define ARG_COUNT 6
#include <Foundation/Types/Implementation/DelegateHelper_inl.h>
#undef ARG_COUNT

template <typename Function>
ezDelegate<Function> ezMakeDelegate(Function* function)
{
  return ezDelegate<Function>(function);
}

template <typename Method, typename Class>
typename ezMakeDelegateHelper<Method>::DelegateType ezMakeDelegate(Method method, Class* pClass)
{
  return typename ezMakeDelegateHelper<Method>::DelegateType(method, pClass);
}
