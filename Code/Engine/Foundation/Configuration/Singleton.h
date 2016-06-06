#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>

/// \file


/// \brief ezSingletonRegistry knows about all singleton instances of classes that use EZ_DECLARE_SINGLETON.
///
/// It allows to query for a specific interface implementation by type name only, which makes it possible to
/// get rid of unwanted library dependencies and use pure virtual interface classes, without singleton code
/// (and thus link dependencies).
///
/// See EZ_DECLARE_SINGLETON and EZ_DECLARE_SINGLETON_OF_INTERFACE for details.
class EZ_FOUNDATION_DLL ezSingletonRegistry
{
public:

  /// \todo Events for new/deleted singletons -> ezInspector integration

  /// \test This is new

  static void* GetSingletonInstance(const char* szSingletonClassType);

  template<typename Interface>
  static Interface* GetSingletonInstance(const char* szSingletonClassType)
  {
    return static_cast<Interface*>(GetSingletonInstance(szSingletonClassType));
  }

  /// \brief Allows to inspect all known singletons
  static const ezMap<ezString, void*>& GetAllRegisteredSingletons();

  static void Register(void* pSingletonInstance, const char* szTypeName);
  static void Unregister(const char* szTypeName);

private:
  template<typename>
  friend class ezSingletonRegistrar;

  static ezMap<ezString, void*> s_Singletons;
};


/// \brief Insert this into a class declaration to turn the class into a singleton.
///
///        You can access the singleton instance in two ways.
///        By calling the static GetSingleton() function on the specific type.
///        By querying the instance through ezSingletonRegistry giving the class type as a string.
///        The latter allows to get the implementation of an interface that is only declared through a simple header
///        but was not linked against.
///
///        Use EZ_DECLARE_SINGLETON for a typical singleton class.
///        Use EZ_DECLARE_SINGLETON_OF_INTERFACE for a singleton class that implements a specific interface,
///        which is itself not declared as a singleton and thus does not support to get to the interface implementation
///        through GetSingleton(). This is necessary, if you want to decouple library link dependencies and thus not put
///        any singleton code into the interface declaration, to keep it a pure virtual interface.
///        You can then query that class pointer also through the name of the interface using ezSingletonRegistry.
#define EZ_DECLARE_SINGLETON(self) \
  public: \
    static self* GetSingleton() { return s_pSingleton; } \
  private: \
    EZ_DISALLOW_COPY_AND_ASSIGN(self); \
    void RegisterSingleton() { s_pSingleton = this; ezSingletonRegistry::Register(this, #self); } \
    static void UnregisterSingleton() { if (s_pSingleton) { ezSingletonRegistry::Unregister(#self); s_pSingleton = nullptr; } } \
    friend class ezSingletonRegistrar<self>; \
    ezSingletonRegistrar<self> m_SingletonRegistrar; \
    static self* s_pSingleton

/// \brief Insert this into a class declaration to turn the class into a singleton.
///
///        You can access the singleton instance in two ways.
///        By calling the static GetSingleton() function on the specific type.
///        By querying the instance through ezSingletonRegistry giving the class type as a string.
///        The latter allows to get the implementation of an interface that is only declared through a simple header
///        but was not linked against.
///
///        Use EZ_DECLARE_SINGLETON for a typical singleton class.
///        Use EZ_DECLARE_SINGLETON_OF_INTERFACE for a singleton class that implements a specific interface,
///        which is itself not declared as a singleton and thus does not support to get to the interface implementation
///        through GetSingleton(). This is necessary, if you want to decouple library link dependencies and thus not put
///        any singleton code into the interface declaration, to keep it a pure virtual interface.
///        You can then query that class pointer also through the name of the interface using ezSingletonRegistry.
#define EZ_DECLARE_SINGLETON_OF_INTERFACE(self, interface) \
  public: \
    static self* GetSingleton() { return s_pSingleton; } \
  private: \
    void RegisterSingleton() { s_pSingleton = this; ezSingletonRegistry::Register(this, #self); ezSingletonRegistry::Register(this, #interface); } \
    static void UnregisterSingleton() { if (s_pSingleton) { ezSingletonRegistry::Unregister(#interface); ezSingletonRegistry::Unregister(#self); s_pSingleton = nullptr; } } \
    friend class ezSingletonRegistrar<self>; \
    ezSingletonRegistrar<self> m_SingletonRegistrar; \
    static self* s_pSingleton


/// \brief Put this into the cpp of a singleton class
#define EZ_IMPLEMENT_SINGLETON(self) \
  self* self::s_pSingleton = nullptr



/// \brief [internal] Helper class to implement ezSingletonRegistry and EZ_DECLARE_SINGLETON
///
/// Classes that use EZ_DECLARE_SINGLETON must pass their this pointer to their m_SingletonRegistrar member
/// during construction.
template<class TYPE>
class ezSingletonRegistrar
{
public:
  ezSingletonRegistrar(TYPE* pType)
  {
    pType->RegisterSingleton();
  }

  ~ezSingletonRegistrar()
  {
    TYPE::UnregisterSingleton();
  }
};
