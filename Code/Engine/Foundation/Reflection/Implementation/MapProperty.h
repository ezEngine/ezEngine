#pragma once

/// \file

#include <Foundation/Reflection/Implementation/AbstractProperty.h>

class ezRTTI;

template <typename Type>
class ezTypedMapProperty : public ezAbstractMapProperty
{
public:
  ezTypedMapProperty(const char* szPropertyName)
      : ezAbstractMapProperty(szPropertyName)
  {
    m_Flags = ezPropertyFlags::GetParameterFlags<Type>();
    EZ_CHECK_AT_COMPILETIME_MSG(!std::is_pointer<Type>::value ||
                                    ezVariant::TypeDeduction<typename ezTypeTraits<Type>::NonConstReferencePointerType>::value ==
                                        ezVariantType::Invalid,
                                "Pointer to standard types are not supported.");
  }

  virtual const ezRTTI* GetSpecificType() const override
  {
    return ezGetStaticRTTI<typename ezTypeTraits<Type>::NonConstReferencePointerType>();
  }
};


template <typename Class, typename Type, typename Container>
class ezAccessorMapProperty : public ezTypedMapProperty<Type>
{
public:
  typedef typename ezTypeTraits<Container>::NonConstReferenceType ContainerType;
  typedef typename ezTypeTraits<Type>::NonConstReferenceType RealType;

  typedef void (Class::*InsertFunc)(const char* szKey, Type value);
  typedef void (Class::*RemoveFunc)(const char* szKey);
  typedef bool (Class::*GetValueFunc)(const char* szKey, RealType& value) const;
  typedef Container (Class::*GetKeyRangeFunc)() const;

  ezAccessorMapProperty(const char* szPropertyName, GetKeyRangeFunc getKeys, GetValueFunc getValue, InsertFunc insert, RemoveFunc remove)
      : ezTypedMapProperty<Type>(szPropertyName)
  {
    EZ_ASSERT_DEBUG(getKeys != nullptr, "The getKeys function of a map property cannot be nullptr.");
    EZ_ASSERT_DEBUG(getValue != nullptr, "The GetValueFunc function of a map property cannot be nullptr.");

    m_GetKeyRange = getKeys;
    m_GetValue = getValue;
    m_Insert = insert;
    m_Remove = remove;

    if (m_Insert == nullptr || remove == nullptr)
      ezAbstractMapProperty::m_Flags.Add(ezPropertyFlags::ReadOnly);
  }

  virtual bool IsEmpty(const void* pInstance) const override
  {
    decltype(auto) c = (static_cast<const Class*>(pInstance)->*m_GetKeyRange)();
    return begin(c) == end(c);
  }

  virtual void Clear(void* pInstance) override
  {
    while (true)
    {
      decltype(auto) c = (static_cast<const Class*>(pInstance)->*m_GetKeyRange)();
      auto it = begin(c);
      if (it != end(c))
        Remove(pInstance, *it);
      else
        return;
    }
  }

  virtual void Insert(void* pInstance, const char* szKey, const void* pObject) override
  {
    EZ_ASSERT_DEBUG(m_Insert != nullptr, "The property '{0}' has no insert function, thus it is read-only.",
                    ezAbstractProperty::GetPropertyName());
    (static_cast<Class*>(pInstance)->*m_Insert)(szKey, *static_cast<const RealType*>(pObject));
  }

  virtual void Remove(void* pInstance, const char* szKey) override
  {
    EZ_ASSERT_DEBUG(m_Remove != nullptr, "The property '{0}' has no remove function, thus it is read-only.",
                    ezAbstractProperty::GetPropertyName());
    (static_cast<Class*>(pInstance)->*m_Remove)(szKey);
  }

  virtual bool Contains(const void* pInstance, const char* szKey) const override
  {
    RealType value;
    return (static_cast<const Class*>(pInstance)->*m_GetValue)(szKey, value);
  }

  virtual bool GetValue(const void* pInstance, const char* szKey, void* pObject) const override
  {
    return (static_cast<const Class*>(pInstance)->*m_GetValue)(szKey, *static_cast<RealType*>(pObject));
  }

  virtual void GetKeys(const void* pInstance, ezHybridArray<ezString, 16>& out_keys) const override
  {
    out_keys.Clear();
    decltype(auto) c = (static_cast<const Class*>(pInstance)->*m_GetKeyRange)();
    for (const auto& key : c)
    {
      out_keys.PushBack(key);
    }
  }

private:
  GetKeyRangeFunc m_GetKeyRange;
  GetValueFunc m_GetValue;
  InsertFunc m_Insert;
  RemoveFunc m_Remove;
};


template <typename Class, typename Type, typename Container>
class ezWriteAccessorMapProperty : public ezTypedMapProperty<Type>
{
public:
  typedef typename ezTypeTraits<Container>::NonConstReferenceType ContainerType;
  typedef typename ezContainerSubTypeResolver<ContainerType>::Type ContainerSubType;
  typedef typename ezTypeTraits<Type>::NonConstReferenceType RealType;

  typedef void (Class::*InsertFunc)(const char* szKey, Type value);
  typedef void (Class::*RemoveFunc)(const char* szKey);
  typedef Container (Class::*GetContainerFunc)() const;

  ezWriteAccessorMapProperty(const char* szPropertyName, GetContainerFunc getContainer, InsertFunc insert, RemoveFunc remove)
      : ezTypedMapProperty<Type>(szPropertyName)
  {
    EZ_ASSERT_DEBUG(getContainer != nullptr, "The get count function of a map property cannot be nullptr.");

    m_GetContainer = getContainer;
    m_Insert = insert;
    m_Remove = remove;

    if (m_Insert == nullptr)
      ezAbstractMapProperty::m_Flags.Add(ezPropertyFlags::ReadOnly);
  }

  virtual bool IsEmpty(const void* pInstance) const override { return (static_cast<const Class*>(pInstance)->*m_GetContainer)().IsEmpty(); }

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
    EZ_ASSERT_DEBUG(m_Insert != nullptr, "The property '{0}' has no insert function, thus it is read-only.",
                    ezAbstractProperty::GetPropertyName());
    (static_cast<Class*>(pInstance)->*m_Insert)(szKey, *static_cast<const RealType*>(pObject));
  }

  virtual void Remove(void* pInstance, const char* szKey) override
  {
    EZ_ASSERT_DEBUG(m_Remove != nullptr, "The property '{0}' has no remove function, thus it is read-only.",
                    ezAbstractProperty::GetPropertyName());
    (static_cast<Class*>(pInstance)->*m_Remove)(szKey);
  }

  virtual bool Contains(const void* pInstance, const char* szKey) const override
  {
    return (static_cast<const Class*>(pInstance)->*m_GetContainer)().Contains(szKey);
  }

  virtual bool GetValue(const void* pInstance, const char* szKey, void* pObject) const override
  {
    decltype(auto) c = (static_cast<const Class*>(pInstance)->*m_GetContainer)();
    const RealType* value = c.GetValue(szKey);
    if (value)
    {
      *static_cast<RealType*>(pObject) = *value;
    }
    return value != nullptr;
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



template <typename Class, typename Container, Container Class::*Member>
struct ezMapPropertyAccessor
{
  typedef typename ezTypeTraits<Container>::NonConstReferenceType ContainerType;
  typedef typename ezTypeTraits<typename ezContainerSubTypeResolver<ContainerType>::Type>::NonConstReferenceType Type;

  static const ContainerType& GetConstContainer(const Class* pInstance) { return (*pInstance).*Member; }

  static ContainerType& GetContainer(Class* pInstance) { return (*pInstance).*Member; }
};


template <typename Class, typename Container, typename Type>
class ezMemberMapProperty : public ezTypedMapProperty<typename ezTypeTraits<Type>::NonConstReferenceType>
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

  virtual bool IsEmpty(const void* pInstance) const override { return m_ConstGetter(static_cast<const Class*>(pInstance)).IsEmpty(); }

  virtual void Clear(void* pInstance) override
  {
    EZ_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const set accessor function, thus it is read-only.",
                    ezAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance)).Clear();
  }

  virtual void Insert(void* pInstance, const char* szKey, const void* pObject) override
  {
    EZ_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const set accessor function, thus it is read-only.",
                    ezAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance)).Insert(szKey, *static_cast<const RealType*>(pObject));
  }

  virtual void Remove(void* pInstance, const char* szKey) override
  {
    EZ_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const set accessor function, thus it is read-only.",
                    ezAbstractProperty::GetPropertyName());
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
