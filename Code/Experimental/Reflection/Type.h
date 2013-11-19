#pragma once

#include <Foundation/Basics.h>
#include "TypedProperty.h"
#include <vector>
using namespace std;

class ezTypeRTTI
{
public:
  enum TypeMode
  {
    Struct,
    Class
  };

  ezTypeRTTI(const char* szTypeName, const ezTypeRTTI* pParentType, TypeMode mode);

  const char* GetTypeName() const { return m_szTypeName; }
  ezInt32 GetTypeID() const { return m_iTypeID; }
  const ezTypeRTTI* GetParentRTTI() const { return m_pParentType; }
  TypeMode GetTypeMode() const { return m_TypeMode; }

  static ezInt32 GetRTTIClassID(const char* szClassName);

  template<class DERIVED>
  bool IsDerivedFrom() const
  {
    ezInt32 iTypeID = DERIVED::GetStaticRTTI()->GetTypeID();

    const ezTypeRTTI* pCur = this;

    // ID ranges, I know...
    while (pCur)
    {
      if (pCur->m_iTypeID == iTypeID)
        return true;

      pCur = pCur->GetParentRTTI();
    }

    return false;
  }

  template<class TYPE>
  bool IsOfType() const
  {
    return GetTypeID() == TYPE::GetStaticRTTI()->GetTypeID();
  }

  vector<ezAbstractProperty*> m_Properties;

private:
  TypeMode m_TypeMode;
  const char* m_szTypeName;
  ezInt32 m_iTypeID;
  const ezTypeRTTI* m_pParentType;
};

class ezCallAtStartup
{
public:
  typedef void (*CallAtStartup)();

  ezCallAtStartup(CallAtStartup Lambda)
  {
    Lambda();
  }

private:

};

#define EZ_ADD_PROPERTY(CLASS, TYPE, NAME) \
  static ezCallAtStartup RegisterProperty_##CLASS_##NAME([] { CLASS::GetStaticRTTI()->m_Properties.push_back(new ezCustomAccessorProperty<CLASS, TYPE>(#NAME, &CLASS::Get##NAME, &CLASS::Set##NAME)); } );

// all classes of type Blaaaa (including specializations) are a friend of reflected classes
// so we just generate some unique dummy type to represent the specialization for this particular property
// then we specialize Blaaaa under that type and implement the getter and setter for the property, even though that member might be private
// then we just register those static functions from Blaaaa as the getter/setter functions for the using ezGeneratedAccessorProperty

#define EZ_ADD_PROPERTY_NO_ACCESSOR(CLASS, TYPE, NAME) \
  class DummyType_##CLASS_##NAME { }; \
  template<> \
  struct Blaaaa<DummyType_##CLASS_##NAME> \
  { \
    static TYPE GetProperty(const CLASS* pClass) { return pClass->NAME; } \
    static void SetProperty(CLASS* pClass, TYPE value) { pClass->NAME = value; } \
  }; \
  static ezCallAtStartup RegisterProperty_##CLASS_##NAME([] { CLASS::GetStaticRTTI()->m_Properties.push_back(new ezGeneratedAccessorProperty<CLASS, TYPE>(#NAME, \
      Blaaaa<DummyType_##CLASS_##NAME>::GetProperty, \
     Blaaaa<DummyType_##CLASS_##NAME>::SetProperty)); \
  });

