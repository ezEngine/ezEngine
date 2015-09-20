#pragma once

/// \file

#include <Foundation/Reflection/Implementation/AbstractProperty.h>

class ezRTTI;

/// \brief Do not cast into this class or any of its derived classes, use ezTypedArrayProperty instead.
template<typename Type>
class ezTypedArrayProperty : public ezAbstractArrayProperty
{
public:
  ezTypedArrayProperty(const char* szPropertyName) : ezAbstractArrayProperty(szPropertyName)
  {
    ezVariant::Type::Enum type = static_cast<ezVariant::Type::Enum>(ezVariant::TypeDeduction<typename ezTypeTraits<Type>::NonConstReferenceType>::value);
    if ((type >= ezVariant::Type::Bool && type <= ezVariant::Type::Uuid) || EZ_IS_SAME_TYPE(ezVariant, Type))
      m_Flags.Add(ezPropertyFlags::StandardType);

    if (type == ezVariant::Type::VoidPointer || type == ezVariant::Type::ReflectedPointer)
      m_Flags.Add(ezPropertyFlags::Pointer);
  }

  virtual const ezRTTI* GetSpecificType() const override
  {
    return ezGetStaticRTTI<typename ezTypeTraits<Type>::NonConstReferencePointerType>();
  }
};

/// \brief Specialization of ezTypedArrayProperty to retain the pointer in const char*.
template <>
class ezTypedArrayProperty<const char*> : public ezAbstractArrayProperty
{
public:
  ezTypedArrayProperty(const char* szPropertyName) : ezAbstractArrayProperty(szPropertyName)
  {
    m_Flags.Add(ezPropertyFlags::StandardType);
  }

  virtual const ezRTTI* GetSpecificType() const override
  {
    return ezGetStaticRTTI<const char*>();
  }
};


template<typename Class, typename Type>
class ezAccessorArrayProperty : public ezTypedArrayProperty<Type>
{
public:
  typedef typename ezTypeTraits<Type>::NonConstReferenceType RealType;
  typedef ezUInt32 (Class::*GetCountFunc)() const;
  typedef Type (Class::*GetValueFunc)(ezUInt32 uiIndex) const;
  typedef void (Class::*SetValueFunc)(ezUInt32 uiIndex, Type value);
  typedef void (Class::*InsertFunc)(ezUInt32 uiIndex, Type value);
  typedef void (Class::*RemoveFunc)(ezUInt32 uiIndex);


  ezAccessorArrayProperty(const char* szPropertyName, GetCountFunc getCount, GetValueFunc getter, SetValueFunc setter, InsertFunc insert, RemoveFunc remove)
    : ezTypedArrayProperty<Type>(szPropertyName)
  {
    EZ_ASSERT_DEBUG(getCount != nullptr, "The get count function of an array property cannot be nullptr.");
    EZ_ASSERT_DEBUG(m_Getter != nullptr, "The get value function of an array property cannot be nullptr.");

    m_GetCount = getCount;
    m_Getter = getter;
    m_Setter = setter;
    m_Insert = insert;
    m_Remove = remove;

    if (m_Setter == nullptr)
      ezAbstractArrayProperty::m_Flags.Add(ezPropertyFlags::ReadOnly);
  }


  virtual ezUInt32 GetCount(const void* pInstance) const override
  {
    return (static_cast<const Class*>(pInstance)->*m_GetCount)();
  }

  virtual void GetValue(const void* pInstance, ezUInt32 uiIndex, void* pObject) const override
  {
    EZ_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "GetValue: uiIndex ('%u') is out of range ('%u')", uiIndex, GetCount(pInstance));
    *static_cast<RealType*>(pObject) = (static_cast<const Class*>(pInstance)->*m_Getter)(uiIndex);
  }

  virtual void SetValue(void* pInstance, ezUInt32 uiIndex, const void* pObject) override
  {
    EZ_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "SetValue: uiIndex ('%u') is out of range ('%u')", uiIndex, GetCount(pInstance));
    EZ_ASSERT_DEBUG(m_Setter != nullptr, "The property '%s' has no setter function, thus it is read-only.", ezAbstractProperty::GetPropertyName());
    (static_cast<Class*>(pInstance)->*m_Setter)(uiIndex, *static_cast<const RealType*>(pObject));
  }

  virtual void Insert(void* pInstance, ezUInt32 uiIndex, void* pObject) override
  {
    EZ_ASSERT_DEBUG(uiIndex <= GetCount(pInstance), "Insert: uiIndex ('%u') is out of range ('%u')", uiIndex, GetCount(pInstance));
    EZ_ASSERT_DEBUG(m_Insert != nullptr, "The property '%s' has no insert function, thus it is read-only.", ezAbstractProperty::GetPropertyName());
    (static_cast<Class*>(pInstance)->*m_Insert)(uiIndex, *static_cast<const RealType*>(pObject));
  }

  virtual void Remove(void* pInstance, ezUInt32 uiIndex) override
  {
    EZ_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "Remove: uiIndex ('%u') is out of range ('%u')", uiIndex, GetCount(pInstance));
    EZ_ASSERT_DEBUG(m_Remove != nullptr, "The property '%s' has no setter function, thus it is read-only.", ezAbstractProperty::GetPropertyName());
    (static_cast<Class*>(pInstance)->*m_Remove)(uiIndex);
  }

  virtual void Clear(void* pInstance) override
  {
    SetCount(pInstance, 0);
  }

  virtual void SetCount(void* pInstance, ezUInt32 uiCount) override
  {
    EZ_ASSERT_DEBUG(m_Insert != nullptr && m_Remove != nullptr, "The property '%s' has no remove and insert function, thus it is fixed-size.", ezAbstractProperty::GetPropertyName());
    while (uiCount < GetCount(pInstance))
    {
      Remove(pInstance, GetCount(pInstance) - 1);
    }
    while (uiCount > GetCount(pInstance))
    {
      RealType elem = RealType();
      Insert(pInstance, GetCount(pInstance), &elem);
    }
  }

private:
  GetCountFunc m_GetCount;
  GetValueFunc m_Getter;
  SetValueFunc m_Setter;
  InsertFunc m_Insert;
  RemoveFunc m_Remove;
};



template <typename Class, typename Container, Container Class::* Member>
struct ezArrayPropertyAccessor
{
  typedef typename ezTypeTraits<Container>::NonConstReferenceType ContainerType;
  typedef typename ezTypeTraits<typename ezContainerSubTypeResolver<ContainerType>::Type>::NonConstReferenceType Type;

  static const ContainerType& GetConstContainer(const Class* pInstance)
  {
    return (*pInstance).*Member;
  }

  static ContainerType& GetContainer(Class* pInstance)
  {
    return (*pInstance).*Member;
  }
};


template<typename Class, typename Container, typename Type>
class ezMemberArrayProperty : public ezTypedArrayProperty< typename ezTypeTraits<Type>::NonConstReferenceType >
{
public:
  typedef typename ezTypeTraits<Type>::NonConstReferenceType RealType;
  typedef const Container& (*GetConstContainerFunc)(const Class* pInstance);
  typedef Container& (*GetContainerFunc)(Class* pInstance);

  ezMemberArrayProperty(const char* szPropertyName, GetConstContainerFunc constGetter, GetContainerFunc getter)
    : ezTypedArrayProperty<RealType>(szPropertyName)
  {
    EZ_ASSERT_DEBUG(constGetter != nullptr, "The const get count function of an array property cannot be nullptr.");

    m_ConstGetter = constGetter;
    m_Getter = getter;

    if (m_Getter == nullptr)
      ezAbstractArrayProperty::m_Flags.Add(ezPropertyFlags::ReadOnly);
  }

  virtual ezUInt32 GetCount(const void* pInstance) const override
  {
    return m_ConstGetter(static_cast<const Class*>(pInstance)).GetCount();
  }

  virtual void GetValue(const void* pInstance, ezUInt32 uiIndex, void* pObject) const override
  {
    EZ_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "GetValue: uiIndex ('%u') is out of range ('%u')", uiIndex, GetCount(pInstance));
    *static_cast<RealType*>(pObject) = m_ConstGetter(static_cast<const Class*>(pInstance))[uiIndex];
  }

  virtual void SetValue(void* pInstance, ezUInt32 uiIndex, const void* pObject) override
  {
    EZ_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "SetValue: uiIndex ('%u') is out of range ('%u')", uiIndex, GetCount(pInstance));
    EZ_ASSERT_DEBUG(m_Getter != nullptr, "The property '%s' has no non-const array accessor function, thus it is read-only.", ezAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance))[uiIndex] = *static_cast<const RealType*>(pObject);
  }

  virtual void Insert(void* pInstance, ezUInt32 uiIndex, void* pObject) override
  {
    EZ_ASSERT_DEBUG(uiIndex <= GetCount(pInstance), "Insert: uiIndex ('%u') is out of range ('%u')", uiIndex, GetCount(pInstance));
    EZ_ASSERT_DEBUG(m_Getter != nullptr, "The property '%s' has no non-const array accessor function, thus it is read-only.", ezAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance)).Insert(*static_cast<const RealType*>(pObject), uiIndex);
  }

  virtual void Remove(void* pInstance, ezUInt32 uiIndex) override
  {
    EZ_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "Remove: uiIndex ('%u') is out of range ('%u')", uiIndex, GetCount(pInstance));
    EZ_ASSERT_DEBUG(m_Getter != nullptr, "The property '%s' has no non-const array accessor function, thus it is read-only.", ezAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance)).RemoveAt(uiIndex);
  }

  virtual void Clear(void* pInstance) override
  {
    EZ_ASSERT_DEBUG(m_Getter != nullptr, "The property '%s' has no non-const array accessor function, thus it is read-only.", ezAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance)).Clear();
  }

  virtual void SetCount(void* pInstance, ezUInt32 uiCount) override
  {
    EZ_ASSERT_DEBUG(m_Getter != nullptr, "The property '%s' has no non-const array accessor function, thus it is read-only.", ezAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance)).SetCount(uiCount);
  }

private:
  GetConstContainerFunc m_ConstGetter;
  GetContainerFunc m_Getter;
};