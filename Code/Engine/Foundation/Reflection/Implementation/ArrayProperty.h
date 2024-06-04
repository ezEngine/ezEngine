#pragma once

/// \file

#include <Foundation/Reflection/Implementation/AbstractProperty.h>

class ezRTTI;

/// \brief Do not cast into this class or any of its derived classes, use ezTypedArrayProperty instead.
template <typename Type>
class ezTypedArrayProperty : public ezAbstractArrayProperty
{
public:
  ezTypedArrayProperty(const char* szPropertyName)
    : ezAbstractArrayProperty(szPropertyName)
  {
    m_Flags = ezPropertyFlags::GetParameterFlags<Type>();
    static_assert(!std::is_pointer<Type>::value ||
                                  ezVariantTypeDeduction<typename ezTypeTraits<Type>::NonConstReferencePointerType>::value ==
                                    ezVariantType::Invalid,
      "Pointer to standard types are not supported.");
  }

  virtual const ezRTTI* GetSpecificType() const override { return ezGetStaticRTTI<typename ezTypeTraits<Type>::NonConstReferencePointerType>(); }
};

/// \brief Specialization of ezTypedArrayProperty to retain the pointer in const char*.
template <>
class ezTypedArrayProperty<const char*> : public ezAbstractArrayProperty
{
public:
  ezTypedArrayProperty(const char* szPropertyName)
    : ezAbstractArrayProperty(szPropertyName)
  {
    m_Flags = ezPropertyFlags::GetParameterFlags<const char*>();
  }

  virtual const ezRTTI* GetSpecificType() const override { return ezGetStaticRTTI<const char*>(); }
};


template <typename Class, typename Type>
class ezAccessorArrayProperty : public ezTypedArrayProperty<Type>
{
public:
  using RealType = typename ezTypeTraits<Type>::NonConstReferenceType;
  using GetCountFunc = ezUInt32 (Class::*)() const;
  using GetValueFunc = Type (Class::*)(ezUInt32 uiIndex) const;
  using SetValueFunc = void (Class::*)(ezUInt32 uiIndex, Type value);
  using InsertFunc = void (Class::*)(ezUInt32 uiIndex, Type value);
  using RemoveFunc = void (Class::*)(ezUInt32 uiIndex);


  ezAccessorArrayProperty(
    const char* szPropertyName, GetCountFunc getCount, GetValueFunc getter, SetValueFunc setter, InsertFunc insert, RemoveFunc remove)
    : ezTypedArrayProperty<Type>(szPropertyName)
  {
    EZ_ASSERT_DEBUG(getCount != nullptr, "The get count function of an array property cannot be nullptr.");
    EZ_ASSERT_DEBUG(getter != nullptr, "The get value function of an array property cannot be nullptr.");

    m_GetCount = getCount;
    m_Getter = getter;
    m_Setter = setter;
    m_Insert = insert;
    m_Remove = remove;

    if (m_Setter == nullptr)
      ezAbstractArrayProperty::m_Flags.Add(ezPropertyFlags::ReadOnly);
  }


  virtual ezUInt32 GetCount(const void* pInstance) const override { return (static_cast<const Class*>(pInstance)->*m_GetCount)(); }

  virtual void GetValue(const void* pInstance, ezUInt32 uiIndex, void* pObject) const override
  {
    EZ_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "GetValue: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));
    *static_cast<RealType*>(pObject) = (static_cast<const Class*>(pInstance)->*m_Getter)(uiIndex);
  }

  virtual void SetValue(void* pInstance, ezUInt32 uiIndex, const void* pObject) const override
  {
    EZ_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "SetValue: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));
    EZ_ASSERT_DEBUG(m_Setter != nullptr, "The property '{0}' has no setter function, thus it is read-only.", ezAbstractProperty::GetPropertyName());
    (static_cast<Class*>(pInstance)->*m_Setter)(uiIndex, *static_cast<const RealType*>(pObject));
  }

  virtual void Insert(void* pInstance, ezUInt32 uiIndex, const void* pObject) const override
  {
    EZ_ASSERT_DEBUG(uiIndex <= GetCount(pInstance), "Insert: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));
    EZ_ASSERT_DEBUG(m_Insert != nullptr, "The property '{0}' has no insert function, thus it is read-only.", ezAbstractProperty::GetPropertyName());
    (static_cast<Class*>(pInstance)->*m_Insert)(uiIndex, *static_cast<const RealType*>(pObject));
  }

  virtual void Remove(void* pInstance, ezUInt32 uiIndex) const override
  {
    EZ_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "Remove: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));
    EZ_ASSERT_DEBUG(m_Remove != nullptr, "The property '{0}' has no setter function, thus it is read-only.", ezAbstractProperty::GetPropertyName());
    (static_cast<Class*>(pInstance)->*m_Remove)(uiIndex);
  }

  virtual void Clear(void* pInstance) const override { SetCount(pInstance, 0); }

  virtual void SetCount(void* pInstance, ezUInt32 uiCount) const override
  {
    EZ_ASSERT_DEBUG(m_Insert != nullptr && m_Remove != nullptr, "The property '{0}' has no remove and insert function, thus it is fixed-size.",
      ezAbstractProperty::GetPropertyName());
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



template <typename Class, typename Container, Container Class::*Member>
struct ezArrayPropertyAccessor
{
  using ContainerType = typename ezTypeTraits<Container>::NonConstReferenceType;
  using Type = typename ezTypeTraits<typename ezContainerSubTypeResolver<ContainerType>::Type>::NonConstReferenceType;

  static const ContainerType& GetConstContainer(const Class* pInstance) { return (*pInstance).*Member; }

  static ContainerType& GetContainer(Class* pInstance) { return (*pInstance).*Member; }
};


template <typename Class, typename Container, typename Type>
class ezMemberArrayProperty : public ezTypedArrayProperty<typename ezTypeTraits<Type>::NonConstReferenceType>
{
public:
  using RealType = typename ezTypeTraits<Type>::NonConstReferenceType;
  using GetConstContainerFunc = const Container& (*)(const Class* pInstance);
  using GetContainerFunc = Container& (*)(Class* pInstance);

  ezMemberArrayProperty(const char* szPropertyName, GetConstContainerFunc constGetter, GetContainerFunc getter)
    : ezTypedArrayProperty<RealType>(szPropertyName)
  {
    EZ_ASSERT_DEBUG(constGetter != nullptr, "The const get count function of an array property cannot be nullptr.");

    m_ConstGetter = constGetter;
    m_Getter = getter;

    if (m_Getter == nullptr)
      ezAbstractArrayProperty::m_Flags.Add(ezPropertyFlags::ReadOnly);
  }

  virtual ezUInt32 GetCount(const void* pInstance) const override { return m_ConstGetter(static_cast<const Class*>(pInstance)).GetCount(); }

  virtual void GetValue(const void* pInstance, ezUInt32 uiIndex, void* pObject) const override
  {
    EZ_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "GetValue: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));
    *static_cast<RealType*>(pObject) = m_ConstGetter(static_cast<const Class*>(pInstance))[uiIndex];
  }

  virtual void SetValue(void* pInstance, ezUInt32 uiIndex, const void* pObject) const override
  {
    EZ_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "SetValue: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));
    EZ_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const array accessor function, thus it is read-only.",
      ezAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance))[uiIndex] = *static_cast<const RealType*>(pObject);
  }

  virtual void Insert(void* pInstance, ezUInt32 uiIndex, const void* pObject) const override
  {
    EZ_ASSERT_DEBUG(uiIndex <= GetCount(pInstance), "Insert: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));
    EZ_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const array accessor function, thus it is read-only.",
      ezAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance)).InsertAt(uiIndex, *static_cast<const RealType*>(pObject));
  }

  virtual void Remove(void* pInstance, ezUInt32 uiIndex) const override
  {
    EZ_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "Remove: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));
    EZ_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const array accessor function, thus it is read-only.",
      ezAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance)).RemoveAtAndCopy(uiIndex);
  }

  virtual void Clear(void* pInstance) const override
  {
    EZ_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const array accessor function, thus it is read-only.",
      ezAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance)).Clear();
  }

  virtual void SetCount(void* pInstance, ezUInt32 uiCount) const override
  {
    EZ_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const array accessor function, thus it is read-only.",
      ezAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance)).SetCount(uiCount);
  }

  virtual void* GetValuePointer(void* pInstance, ezUInt32 uiIndex) const override
  {
    EZ_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "GetValue: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));
    return &(m_Getter(static_cast<Class*>(pInstance))[uiIndex]);
  }

private:
  GetConstContainerFunc m_ConstGetter;
  GetContainerFunc m_Getter;
};

/// \brief Read only version of ezMemberArrayProperty that does not call any functions that modify the array. This is needed to reflect ezArrayPtr members.
template <typename Class, typename Container, typename Type>
class ezMemberArrayReadOnlyProperty : public ezTypedArrayProperty<typename ezTypeTraits<Type>::NonConstReferenceType>
{
public:
  using RealType = typename ezTypeTraits<Type>::NonConstReferenceType;
  using GetConstContainerFunc = const Container& (*)(const Class* pInstance);

  ezMemberArrayReadOnlyProperty(const char* szPropertyName, GetConstContainerFunc constGetter)
    : ezTypedArrayProperty<RealType>(szPropertyName)
  {
    EZ_ASSERT_DEBUG(constGetter != nullptr, "The const get count function of an array property cannot be nullptr.");

    m_ConstGetter = constGetter;
    ezAbstractArrayProperty::m_Flags.Add(ezPropertyFlags::ReadOnly);
  }

  virtual ezUInt32 GetCount(const void* pInstance) const override { return m_ConstGetter(static_cast<const Class*>(pInstance)).GetCount(); }

  virtual void GetValue(const void* pInstance, ezUInt32 uiIndex, void* pObject) const override
  {
    EZ_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "GetValue: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));
    *static_cast<RealType*>(pObject) = m_ConstGetter(static_cast<const Class*>(pInstance))[uiIndex];
  }

  virtual void SetValue(void* pInstance, ezUInt32 uiIndex, const void* pObject) const override
  {
    EZ_REPORT_FAILURE("The property '{0}' is read-only.", ezAbstractProperty::GetPropertyName());
  }

  virtual void Insert(void* pInstance, ezUInt32 uiIndex, const void* pObject) const override
  {
    EZ_REPORT_FAILURE("The property '{0}' is read-only.", ezAbstractProperty::GetPropertyName());
  }

  virtual void Remove(void* pInstance, ezUInt32 uiIndex) const override
  {
    EZ_REPORT_FAILURE("The property '{0}' is read-only.", ezAbstractProperty::GetPropertyName());
  }

  virtual void Clear(void* pInstance) const override
  {
    EZ_REPORT_FAILURE("The property '{0}' is read-only.", ezAbstractProperty::GetPropertyName());
  }

  virtual void SetCount(void* pInstance, ezUInt32 uiCount) const override
  {
    EZ_REPORT_FAILURE("The property '{0}' is read-only.", ezAbstractProperty::GetPropertyName());
  }

private:
  GetConstContainerFunc m_ConstGetter;
};
