#pragma once

/// \file

#include <Foundation/Reflection/Implementation/AbstractProperty.h>

class ezRTTI;

template<typename Type>
class ezTypedMapProperty : public ezAbstractMapProperty
{
public:
  ezTypedMapProperty(const char* szPropertyName) : ezAbstractMapProperty(szPropertyName)
  {
    ezVariant::Type::Enum type = static_cast<ezVariant::Type::Enum>(ezVariant::TypeDeduction<typename ezTypeTraits<Type>::NonConstReferenceType>::value);
    if ((type >= ezVariant::Type::FirstStandardType && type <= ezVariant::Type::LastStandardType) || EZ_IS_SAME_TYPE(ezVariant, Type))
      m_Flags.Add(ezPropertyFlags::StandardType);

    if (type == ezVariant::Type::VoidPointer || type == ezVariant::Type::ReflectedPointer)
      m_Flags.Add(ezPropertyFlags::Pointer);

    if (!m_Flags.IsAnySet(ezPropertyFlags::StandardType | ezPropertyFlags::Pointer))
      m_Flags.Add(ezPropertyFlags::EmbeddedClass);
  }

  virtual const ezRTTI* GetSpecificType() const override
  {
    return ezGetStaticRTTI<typename ezTypeTraits<Type>::NonConstReferencePointerType>();
  }
};


template<typename Class, typename Type, typename Container>
class ezAccessorMapProperty : public ezTypedMapProperty<Type>
{
public:
  typedef typename ezTypeTraits<Container>::NonConstReferenceType ContainerType;
  typedef typename ezContainerSubTypeResolver<ContainerType>::Type ContainerSubType;
  typedef typename ezTypeTraits<Type>::NonConstReferenceType RealType;

  typedef void (Class::*InsertFunc)(const char* szKey, Type value);
  typedef void (Class::*RemoveFunc)(const char* szKey);
  typedef Container (Class::*GetContainerFunc)() const;

  ezAccessorMapProperty(const char* szPropertyName, GetContainerFunc getContainer, InsertFunc insert, RemoveFunc remove)
    : ezTypedMapProperty<Type>(szPropertyName)
  {
    EZ_ASSERT_DEBUG(getContainer != nullptr, "The get count function of a map property cannot be nullptr.");

    m_GetContainer = getContainer;
    m_Insert = insert;
    m_Remove = remove;

    if (m_Insert == nullptr)
      ezAbstractMapProperty::m_Flags.Add(ezPropertyFlags::ReadOnly);
  }

  virtual bool IsEmpty(const void* pInstance) const override
  {
    return (static_cast<const Class*>(pInstance)->*m_GetContainer)().IsEmpty();
  }

  virtual void Clear(void* pInstance) override
  {
    decltype(auto) c = (static_cast<const Class*>(pInstance)->*m_GetContainer)();
    while (!IsEmpty(pInstance))
    {
      auto it = c.GetIterator();
      Remove(pInstance, it.Key());
    }
  }

  virtual void Insert(void* pInstance, const char* szKey, const void* pObject) override
  {
    EZ_ASSERT_DEBUG(m_Insert != nullptr, "The property '{0}' has no insert function, thus it is read-only.", ezAbstractProperty::GetPropertyName());
    (static_cast<Class*>(pInstance)->*m_Insert)(szKey, *static_cast<const RealType*>(pObject));
  }

  virtual void Remove(void* pInstance, const char* szKey) override
  {
    EZ_ASSERT_DEBUG(m_Remove != nullptr, "The property '{0}' has no remove function, thus it is read-only.", ezAbstractProperty::GetPropertyName());
    (static_cast<Class*>(pInstance)->*m_Remove)(szKey);
  }

  virtual bool Contains(const void* pInstance, const char* szKey) const override
  {
    return (static_cast<const Class*>(pInstance)->*m_GetContainer)().Contains(szKey);
  }

  virtual bool GetValue(const void* pInstance, const char* szKey, void* pObject) const override
  {
    const RealType* value = (static_cast<const Class*>(pInstance)->*m_GetContainer)().GetValue(szKey);
    if (value)
    {
      *static_cast<RealType*>(pObject) = *value;
    }
    return value != nullptr;
  }

  virtual const void* GetValue(const void* pInstance, const char* szKey) const override
  {
    const RealType* value = (static_cast<const Class*>(pInstance)->*m_GetContainer)().GetValue(szKey);
    return value;
  }

  virtual void GetKeys(const void* pInstance, ezHybridArray<ezString, 16>& out_keys) const override
  {
    decltype(auto) c = (static_cast<const Class*>(pInstance)->*m_GetContainer)();
    out_keys.Clear();
    for (auto it = c.GetIterator(); it.IsValid(); ++it)
    {
      out_keys.PushBack(it.Key());
    }
  }

private:
  GetContainerFunc m_GetContainer;
  InsertFunc m_Insert;
  RemoveFunc m_Remove;
};



template <typename Class, typename Container, Container Class::* Member>
struct ezMapPropertyAccessor
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
class ezMemberMapProperty : public ezTypedMapProperty< typename ezTypeTraits<Type>::NonConstReferenceType >
{
public:
  typedef typename ezTypeTraits<Type>::NonConstReferenceType RealType;
  typedef const Container& (*GetConstContainerFunc)(const Class* pInstance);
  typedef Container& (*GetContainerFunc)(Class* pInstance);

  ezMemberMapProperty(const char* szPropertyName, GetConstContainerFunc constGetter, GetContainerFunc getter)
    : ezTypedMapProperty<RealType>(szPropertyName)
  {
    EZ_ASSERT_DEBUG(constGetter != nullptr, "The const get count function of an array property cannot be nullptr.");

    m_ConstGetter = constGetter;
    m_Getter = getter;

    if (m_Getter == nullptr)
      ezAbstractMapProperty::m_Flags.Add(ezPropertyFlags::ReadOnly);
  }

  virtual bool IsEmpty(const void* pInstance) const override
  {
    return m_ConstGetter(static_cast<const Class*>(pInstance)).IsEmpty();
  }

  virtual void Clear(void* pInstance) override
  {
    EZ_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const set accessor function, thus it is read-only.", ezAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance)).Clear();
  }

  virtual void Insert(void* pInstance, const char* szKey, const void* pObject) override
  {
    EZ_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const set accessor function, thus it is read-only.", ezAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance)).Insert(szKey, *static_cast<const RealType*>(pObject));
  }

  virtual void Remove(void* pInstance, const char* szKey) override
  {
    EZ_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const set accessor function, thus it is read-only.", ezAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance)).Remove(szKey);
  }

  virtual bool Contains(const void* pInstance, const char* szKey) const override
  {
    return m_ConstGetter(static_cast<const Class*>(pInstance)).Contains(szKey);
  }

  virtual bool GetValue(const void* pInstance, const char* szKey, void* pObject) const override
  {
    const RealType* value = m_ConstGetter(static_cast<const Class*>(pInstance)).GetValue(szKey);
    if (value)
    {
      *static_cast<RealType*>(pObject) = *value;
    }
    return value != nullptr;
  }

  virtual const void* GetValue(const void* pInstance, const char* szKey) const override
  {
    const RealType* value = m_ConstGetter(static_cast<const Class*>(pInstance)).GetValue(szKey);
    return value;
  }

  virtual void GetKeys(const void* pInstance, ezHybridArray<ezString, 16>& out_keys) const override
  {
    decltype(auto) c = m_ConstGetter(static_cast<const Class*>(pInstance));
    out_keys.Clear();
    for (auto it = c.GetIterator(); it.IsValid(); ++it)
    {
      out_keys.PushBack(it.Key());
    }
  }

private:
  GetConstContainerFunc m_ConstGetter;
  GetContainerFunc m_Getter;
};

