#pragma once

#include <Foundation/Basics.h>

/// \brief Base class for ezDelegate
class ezDelegateBase
{
public:
  union InstancePtr
  {
    void* m_Ptr;
    const void* m_ConstPtr;
  };

  EZ_FORCE_INLINE ezDelegateBase()
  {
    m_pInstance.m_Ptr = nullptr;
  }

protected:
  InstancePtr m_pInstance;
};

/// \brief A generic delegate class which supports static functions and member functions.
///
/// A delegate is a function pointer that may be used to call both simple C functions, as well
/// as member functions of some class, which requires a 'this' pointer to go along with.
/// The delegate can capture both these use cases dynamically, so this distinction does not need
/// to be made statically, but is the choice of the user who assigns some function to the delegate.
///
/// Delegates have a rather strange syntax:
/// 
/// \code{.cpp}
///   typedef ezDelegate<void (ezUInt32, float)> SomeCallback;
/// \endcode
///
/// This defines a type 'SomeCallback' that can call any function that returns void and
/// takes two parameters, the first being ezUInt32, the second being a float parameter.
/// Now you can use SomeCallback just like any other function pointer type.
///
/// Assigning a C function as the value to the delegate is straight-forward:
///
/// \code{.cpp}
///   void SomeFunction(ezUInt32 i, float f);
///   SomeCallback callback = SomeFunction;
/// \endcode
///
/// Assigning a member function to the delegate is a bit more complex:
///
/// \code{.cpp}
///   class SomeClass { 
///     void SomeFunction(ezUInt32 i, float f);
///   };
///   SomeClass instance;
///   SomeCallback callback = ezDelegate<void (ezUInt32, float)>(&SomeClass::SomeFunction, &instance);
/// \endcode
///
/// Here you have to construct a delegate of the proper type and pass along both the member function pointer
/// as well as the class instance to call the function on.
///
/// Using the delegate to call a function is straight forward again:
///
/// \code{.cpp}
///   SomeCallback callback = ...;
///   if (callback.IsValid())
///     callback(4, 2.0f);
/// \endcode
///
/// Just treat the delegate like a simple C function and call it. Internally it will dispatch the call
/// to whatever function is bound to it, independent of whether it is a regular C function or a member function.
/// 
/// The check to 'IsValid' is only required when the delegate might have a nullptr bound (i.e. no function has
/// been bound to it). When you know the delegate is always bound, this is not necessary.
///
/// \note If you are wondering where the code is, the delegate is implemented with macro and template magic in
/// Delegate_inl.h and DelegateHelper_inl.h.
template <typename T>
struct ezDelegate : public ezDelegateBase
{


};

template <typename T>
struct ezMakeDelegateHelper;

/// \brief A helper function to create delegates from function pointers.
///
/// \code{.cpp}
///   void foo() { }
///   auto delegate = ezMakeDelegate(&foo);
/// \endcode
template <typename Function>
ezDelegate<Function> ezMakeDelegate(Function* function);

/// \brief A helper function to create delegates from methods.
///
/// \code{.cpp}
///   class Example
///   {
///   public:
///     void foo() {}
///   };
///   Example instance;
///   auto delegate = ezMakeDelegate(&Example::foo, &instance);
/// \endcode
template <typename Method, typename Class>
typename ezMakeDelegateHelper<Method>::DelegateType ezMakeDelegate(Method method, Class* pClass);

#include <Foundation/Types/Implementation/Delegate_inl.h>

