#pragma once

#include <Foundation/Basics.h>
#include "TypedProperty.h"
#include <vector>
using namespace std;

class ezTypeRTTI
{
public:

  ezTypeRTTI(const char* szTypeName, const ezTypeRTTI* pParentType);

  const char* GetTypeName() const { return m_szTypeName; }
  ezInt32 GetTypeID() const { return m_iTypeID; }
  const ezTypeRTTI* GetParentRTTI() const { return m_pParentType; }

  static ezInt32 GetRTTIClassID(const char* szClassName);

  bool IsDerivedFrom(const ezTypeRTTI* pRTTI) const
  {
    ezInt32 iTypeID = pRTTI->GetTypeID();

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

  template<class DERIVED>
  bool IsDerivedFrom() const
  {
    return IsDerivedFrom(GetStaticRTTI<DERIVED>());
  }

  bool IsOfType(const ezTypeRTTI* pRTTI) const
  {
    return GetTypeID() == pRTTI->GetTypeID();
  }

  template<class TYPE>
  bool IsOfType() const
  {
    return IsOfType(GetStaticRTTI<TYPE>());
  }

  vector<ezAbstractProperty*> m_Properties;

private:
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

#define EZ_ADD_PROPERTY_WITH_ACCESSOR(CLASS, NAME) \
  static void StupidVS2010bla_##CLASS_##NAME() { GetStaticRTTI<CLASS>()->m_Properties.push_back(new ezAccessorProperty<CLASS, decltype(((CLASS*) NULL)->Get##NAME())>(#NAME, &CLASS::Get##NAME, &CLASS::Set##NAME)); } \
  static ezCallAtStartup RegisterPropertyAccessor_##CLASS_##NAME(StupidVS2010bla_##CLASS_##NAME);

// all classes of type Blaaaa (including specializations) are a friend of reflected classes
// so we just generate some unique dummy type to represent the specialization for this particular property
// then we specialize Blaaaa under that type and implement the getter and setter for the property, even though that member might be private
// then we just register those static functions from Blaaaa as the getter/setter functions for the using ezGeneratedAccessorProperty

#define EZ_ADD_MEMBER_PROPERTY(CLASS, NAME) \
  class DummyType_##CLASS_##NAME { }; \
  template<> \
  struct Blaaaa<DummyType_##CLASS_##NAME> \
  { \
    typedef decltype(((CLASS*) NULL)->NAME) MemberType; \
    static MemberType GetProperty(const CLASS* pClass) { return pClass->NAME; } \
    static void SetProperty(CLASS* pClass, MemberType value) { pClass->NAME = value; } \
    static void* GetPropertyPointer(const CLASS* pClass) { return (void*)(&pClass->NAME); } \
  }; \
  static void StupidVS2010_##CLASS_##NAME() { GetStaticRTTI<CLASS>()->m_Properties.push_back(new ezGeneratedAccessorProperty<CLASS, Blaaaa<DummyType_##CLASS_##NAME>::MemberType >(#NAME, \
      Blaaaa<DummyType_##CLASS_##NAME>::GetProperty, \
      Blaaaa<DummyType_##CLASS_##NAME>::SetProperty, \
      Blaaaa<DummyType_##CLASS_##NAME>::GetPropertyPointer)); \
  } \
  static ezCallAtStartup RegisterPropertyMember_##CLASS_##NAME(StupidVS2010_##CLASS_##NAME);


#define EZ_ADD_ARRAY_PROPERTY(CLASS, NAME, SUBTYPE) \
  class DummyType_##CLASS_##NAME { }; \
  template<> \
  struct Blaaaa<DummyType_##CLASS_##NAME> \
  { \
    typedef decltype(((CLASS*) NULL)->NAME) MemberType; \
    \
    class Array : public ezArrayProperty \
    { \
    public: \
    Array(const char* szName) : ezArrayProperty(szName) { } \
      virtual const ezTypeRTTI* GetElementType() const EZ_OVERRIDE { return GetStaticRTTI<SUBTYPE>(); } \
      virtual ezUInt32 GetCount(const void* pProperty) const EZ_OVERRIDE { return ((MemberType*) pProperty)->GetCount(); } \
      virtual void* GetElement(const void* pProperty, ezUInt32 at) EZ_OVERRIDE { return (void*) &((*((MemberType*) pProperty))[at]); } \
      virtual void* GetPropertyPointer(const void* pReflected) const EZ_OVERRIDE { return (void*) &(((CLASS*) pReflected)->NAME); } \
    }; \
  }; \
  static void StupidVS2010_##CLASS_##NAME() { GetStaticRTTI<CLASS>()->m_Properties.push_back(new Blaaaa<DummyType_##CLASS_##NAME>::Array(#NAME)); } \
  \
  static ezCallAtStartup RegisterPropertyMember_##CLASS_##NAME(StupidVS2010_##CLASS_##NAME);
