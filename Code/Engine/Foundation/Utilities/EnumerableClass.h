#pragma once

/// \file

/// \brief Base class to add the ability to another class to enumerate all active instance of it, across DLL boundaries.
///
/// This creates a new class-type that has the static information
/// about all instances that were created from that class. Another class now only has to derive from that class and will
/// then gain the ability to count and enumerate its instances.
///
/// Usage is as follows:
///
/// If you have a class A that you want to be enumerable, add this to its header:
///
///   class EZ_DLL_IMPORT_EXPORT_STUFF A : public ezEnumerable<A>
///   {
///     EZ_DECLARE_ENUMERABLE_CLASS(A); // since A is declared as DLL import/export all code embedded in its body will also work properly
///     ...
///   };
///
/// Also add this somewhere in its source-file:
///
///   EZ_ENUMERABLE_CLASS_IMPLEMENTATION(A);
///
/// That's it, now the class instances can be enumerated with 'GetFirstInstance' and 'GetNextInstance'
template <typename Derived, typename Base = ezNoBase>
class ezEnumerable : public Base
{
public:
  ezEnumerable()
  {
    if (Derived::s_pFirstInstance == nullptr)
      Derived::s_pFirstInstance = this;
    else
      Derived::s_pLastInstance->m_pNextInstance = this;
    
    Derived::s_pLastInstance = this;
    m_pNextInstance = nullptr;
    ++Derived::s_uiInstances;
  }
  
  virtual ~ezEnumerable()
  {
    --Derived::s_uiInstances;
    ezEnumerable* pPrev = nullptr;
    ezEnumerable* pCur = Derived::s_pFirstInstance;
    
    while (pCur)
    {
      if (pCur == this)
      {
        if (pPrev == nullptr)
          Derived::s_pFirstInstance = m_pNextInstance;
        else
          pPrev->m_pNextInstance = m_pNextInstance;
        
        if (Derived::s_pLastInstance == this)
          Derived::s_pLastInstance = pPrev;
        
        break;
      }
      
      pPrev = pCur;
      pCur = pCur->m_pNextInstance;
    }
  }

protected:
  ezEnumerable* m_pNextInstance;
};

/// \brief Insert this macro in a class that is supposed to be enumerable, and pass the class name as the parameter.
///
/// See class ezEnumerable for more details.
#define EZ_DECLARE_ENUMERABLE_CLASS(self) \
  EZ_DECLARE_ENUMERABLE_CLASS_WITH_BASE(self, ezNoBase)

/// \brief Insert this macro in a class that is supposed to be enumerable, and pass the class name as the parameter.
///
/// See class ezEnumerable for more details.
#define EZ_DECLARE_ENUMERABLE_CLASS_WITH_BASE(self, base) \
  private: \
    typedef base ezEnumerableBase; \
    friend class ezEnumerable<self, base>; \
    static ezEnumerable<self, base>* s_pFirstInstance; \
    static ezEnumerable<self, base>* s_pLastInstance; \
    static ezUInt32 s_uiInstances; \
  public: \
    static self* GetFirstInstance() { return (self*) s_pFirstInstance; } \
    self* GetNextInstance() { return (self*) m_pNextInstance; } \
  private:

/// \brief Insert this macro in a cpp file and pass the class name of the to-be-enumerable class as the parameter.
///
/// See class ezEnumerable for more details.
#define EZ_ENUMERABLE_CLASS_IMPLEMENTATION(self) \
  ezEnumerable<self, self::ezEnumerableBase>* self::s_pFirstInstance = nullptr; \
  ezEnumerable<self, self::ezEnumerableBase>* self::s_pLastInstance = nullptr; \
  ezUInt32 self::s_uiInstances = 0

