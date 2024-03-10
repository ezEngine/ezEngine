#pragma once

#include <Foundation/Types/Variant.h>

template <typename T>
struct ezCleanType2
{
  using Type = T;
  using RttiType = T;
};

template <typename T>
struct ezCleanType2<ezEnum<T>>
{
  using Type = ezEnum<T>;
  using RttiType = T;
};

template <typename T>
struct ezCleanType2<ezBitflags<T>>
{
  using Type = ezBitflags<T>;
  using RttiType = T;
};

template <typename T>
struct ezCleanType
{
  using Type = typename ezTypeTraits<T>::NonConstReferencePointerType;
  using RttiType = typename ezCleanType2<typename ezTypeTraits<T>::NonConstReferencePointerType>::RttiType;
};

template <>
struct ezCleanType<const char*>
{
  using Type = const char*;
  using RttiType = const char*;
};

//////////////////////////////////////////////////////////////////////////

template <typename T>
struct ezIsOutParam
{
  enum
  {
    value = false,
  };
};

template <typename T>
struct ezIsOutParam<T&>
{
  enum
  {
    value = !std::is_const<typename ezTypeTraits<T>::NonReferencePointerType>::value,
  };
};

template <typename T>
struct ezIsOutParam<T*>
{
  enum
  {
    value = !std::is_const<typename ezTypeTraits<T>::NonReferencePointerType>::value,
  };
};

//////////////////////////////////////////////////////////////////////////

/// \brief Used to determine if the given type is a build-in standard variant type.
template <class T, class C = typename ezCleanType<T>::Type>
struct ezIsStandardType
{
  enum
  {
    value = ezVariant::TypeDeduction<C>::value >= ezVariantType::FirstStandardType && ezVariant::TypeDeduction<C>::value <= ezVariantType::LastStandardType,
  };
};

template <class T>
struct ezIsStandardType<T, ezVariant>
{
  enum
  {
    value = true,
  };
};

//////////////////////////////////////////////////////////////////////////

/// \brief Used to determine if the given type can be stored by value inside an ezVariant (either standard type or custom type).
template <class T, class C = typename ezCleanType<T>::Type>
struct ezIsValueType
{
  enum
  {
    value = (ezVariant::TypeDeduction<C>::value >= ezVariantType::FirstStandardType && ezVariant::TypeDeduction<C>::value <= ezVariantType::LastStandardType) || ezVariantTypeDeduction<C>::classification == ezVariantClass::CustomTypeCast,
  };
};

template <class T>
struct ezIsValueType<T, ezVariant>
{
  enum
  {
    value = true,
  };
};

//////////////////////////////////////////////////////////////////////////
/// \brief Used to automatically assign any value to an ezVariant using the assignment rules
/// outlined in ezAbstractFunctionProperty::Execute.
template <class T,                          ///< Only this parameter needs to be provided, the actual type of the value.
  class C = typename ezCleanType<T>::Type,  ///< Same as T but without the const&* fluff.
  int VALUE_TYPE = ezIsValueType<T>::value> ///< Is 1 if T is a ezTypeFlags::StandardType or a custom type
struct ezVariantAssignmentAdapter
{
  using RealType = typename ezTypeTraits<T>::NonConstReferencePointerType;
  ezVariantAssignmentAdapter(ezVariant& value)
    : m_value(value)
  {
  }

  void operator=(RealType* rhs) { m_value = rhs; }
  void operator=(RealType&& rhs)
  {
    if (m_value.IsValid())
      *m_value.Get<RealType*>() = rhs;
  }
  ezVariant& m_value;
};

template <class T, class S>
struct ezVariantAssignmentAdapter<T, ezEnum<S>, 0>
{
  using RealType = typename ezTypeTraits<T>::NonConstReferencePointerType;
  ezVariantAssignmentAdapter(ezVariant& value)
    : m_value(value)
  {
  }

  void operator=(ezEnum<S>&& rhs) { m_value = static_cast<ezInt64>(rhs.GetValue()); }

  ezVariant& m_value;
};

template <class T, class S>
struct ezVariantAssignmentAdapter<T, ezBitflags<S>, 0>
{
  using RealType = typename ezTypeTraits<T>::NonConstReferencePointerType;
  ezVariantAssignmentAdapter(ezVariant& value)
    : m_value(value)
  {
  }

  void operator=(ezBitflags<S>&& rhs) { m_value = static_cast<ezInt64>(rhs.GetValue()); }

  ezVariant& m_value;
};

template <class T, class C>
struct ezVariantAssignmentAdapter<T, C, 1>
{
  using RealType = typename ezTypeTraits<T>::NonConstReferencePointerType;
  ezVariantAssignmentAdapter(ezVariant& value)
    : m_value(value)
  {
  }

  void operator=(T&& rhs) { m_value = rhs; }

  ezVariant& m_value;
};

template <class T>
struct ezVariantAssignmentAdapter<T, ezVariantArray, 0>
{
  ezVariantAssignmentAdapter(ezVariant& value)
    : m_value(value)
  {
  }

  void operator=(T&& rhs) { m_value = rhs; }

  ezVariant& m_value;
};

template <class T>
struct ezVariantAssignmentAdapter<T, ezVariantDictionary, 0>
{
  ezVariantAssignmentAdapter(ezVariant& value)
    : m_value(value)
  {
  }

  void operator=(T&& rhs) { m_value = rhs; }

  ezVariant& m_value;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Used to implicitly retrieve any value from an ezVariant to be used as a function argument
/// using the assignment rules outlined in ezAbstractFunctionProperty::Execute.
template <class T,                          ///< Only this parameter needs to be provided, the actual type of the argument. Rest is used to force specializations.
  class C = typename ezCleanType<T>::Type,  ///< Same as T but without the const&* fluff.
  int VALUE_TYPE = ezIsValueType<T>::value, ///< Is 1 if T is a ezTypeFlags::StandardType or a custom type
  int OUT_PARAM = ezIsOutParam<T>::value>   ///< Is 1 if T a non-const reference or pointer.
struct ezVariantAdapter
{
  using RealType = typename ezTypeTraits<T>::NonConstReferencePointerType;

  ezVariantAdapter(ezVariant& value)
    : m_value(value)
  {
  }

  operator RealType&() { return *m_value.Get<RealType*>(); }

  operator RealType*() { return m_value.IsValid() ? m_value.Get<RealType*>() : nullptr; }

  ezVariant& m_value;
};

template <class T, class S>
struct ezVariantAdapter<T, ezEnum<S>, 0, 0>
{
  using RealType = typename ezTypeTraits<T>::NonConstReferencePointerType;
  ezVariantAdapter(ezVariant& value)
    : m_value(value)
  {
    if (m_value.IsValid())
      m_realValue = static_cast<typename S::Enum>(m_value.ConvertTo<ezInt64>());
  }

  operator const ezEnum<S>&() { return m_realValue; }
  operator const ezEnum<S>*() { return m_value.IsValid() ? &m_realValue : nullptr; }

  ezVariant& m_value;
  ezEnum<S> m_realValue;
};

template <class T, class S>
struct ezVariantAdapter<T, ezEnum<S>, 0, 1>
{
  using RealType = typename ezTypeTraits<T>::NonConstReferencePointerType;
  ezVariantAdapter(ezVariant& value)
    : m_value(value)
  {
    if (m_value.IsValid())
      m_realValue = static_cast<typename S::Enum>(m_value.ConvertTo<ezInt64>());
  }
  ~ezVariantAdapter()
  {
    if (m_value.IsValid())
      m_value = static_cast<ezInt64>(m_realValue.GetValue());
  }

  operator ezEnum<S>&() { return m_realValue; }
  operator ezEnum<S>*() { return m_value.IsValid() ? &m_realValue : nullptr; }

  ezVariant& m_value;
  ezEnum<S> m_realValue;
};

template <class T, class S>
struct ezVariantAdapter<T, ezBitflags<S>, 0, 0>
{
  using RealType = typename ezTypeTraits<T>::NonConstReferencePointerType;
  ezVariantAdapter(ezVariant& value)
    : m_value(value)
  {
    if (m_value.IsValid())
      m_realValue.SetValue(static_cast<typename S::StorageType>(m_value.ConvertTo<ezInt64>()));
  }

  operator const ezBitflags<S>&() { return m_realValue; }
  operator const ezBitflags<S>*() { return m_value.IsValid() ? &m_realValue : nullptr; }

  ezVariant& m_value;
  ezBitflags<S> m_realValue;
};

template <class T, class S>
struct ezVariantAdapter<T, ezBitflags<S>, 0, 1>
{
  using RealType = typename ezTypeTraits<T>::NonConstReferencePointerType;
  ezVariantAdapter(ezVariant& value)
    : m_value(value)
  {
    if (m_value.IsValid())
      m_realValue.SetValue(static_cast<typename S::StorageType>(m_value.ConvertTo<ezInt64>()));
  }
  ~ezVariantAdapter()
  {
    if (m_value.IsValid())
      m_value = static_cast<ezInt64>(m_realValue.GetValue());
  }

  operator ezBitflags<S>&() { return m_realValue; }
  operator ezBitflags<S>*() { return m_value.IsValid() ? &m_realValue : nullptr; }

  ezVariant& m_value;
  ezBitflags<S> m_realValue;
};

template <class T, class C>
struct ezVariantAdapter<T, C, 1, 0>
{
  using RealType = typename ezTypeTraits<T>::NonConstReferencePointerType;
  ezVariantAdapter(ezVariant& value)
    : m_value(value)
  {
  }

  operator const C&()
  {
    if constexpr (ezVariantTypeDeduction<C>::classification == ezVariantClass::CustomTypeCast)
    {
      if (m_value.GetType() == ezVariantType::TypedPointer)
        return *m_value.Get<RealType*>();
    }
    return m_value.Get<RealType>();
  }

  operator const C*()
  {
    if constexpr (ezVariantTypeDeduction<C>::classification == ezVariantClass::CustomTypeCast)
    {
      if (m_value.GetType() == ezVariantType::TypedPointer)
        return m_value.IsValid() ? m_value.Get<RealType*>() : nullptr;
    }
    return m_value.IsValid() ? &m_value.Get<RealType>() : nullptr;
  }

  ezVariant& m_value;
};

template <class T, class C>
struct ezVariantAdapter<T, C, 1, 1>
{
  using RealType = typename ezTypeTraits<T>::NonConstReferencePointerType;
  ezVariantAdapter(ezVariant& value)
    : m_value(value)
  {
    // We ignore the return value here instead const_cast the Get<> result to profit from the Get methods runtime type checks.
    m_value.GetWriteAccess();
  }

  operator C&()
  {
    if (m_value.GetType() == ezVariantType::TypedPointer)
      return *m_value.Get<RealType*>();
    else
      return const_cast<RealType&>(m_value.Get<RealType>());
  }
  operator C*()
  {
    if (m_value.GetType() == ezVariantType::TypedPointer)
      return m_value.IsValid() ? m_value.Get<RealType*>() : nullptr;
    else
      return m_value.IsValid() ? &const_cast<RealType&>(m_value.Get<RealType>()) : nullptr;
  }

  ezVariant& m_value;
};

template <class T>
struct ezVariantAdapter<T, ezVariant, 1, 0>
{
  ezVariantAdapter(ezVariant& value)
    : m_value(value)
  {
  }

  operator const ezVariant&() { return m_value; }
  operator const ezVariant*() { return &m_value; }

  ezVariant& m_value;
};

template <class T>
struct ezVariantAdapter<T, ezVariant, 1, 1>
{
  ezVariantAdapter(ezVariant& value)
    : m_value(value)
  {
  }

  operator ezVariant&() { return m_value; }
  operator ezVariant*() { return &m_value; }

  ezVariant& m_value;
};

template <class T>
struct ezVariantAdapter<T, ezVariantArray, 0, 0>
{
  ezVariantAdapter(ezVariant& value)
    : m_value(value)
  {
  }

  operator const ezVariantArray&() { return m_value.Get<ezVariantArray>(); }
  operator const ezVariantArray*() { return m_value.IsValid() ? &m_value.Get<ezVariantArray>() : nullptr; }

  ezVariant& m_value;
};

template <class T>
struct ezVariantAdapter<T, ezVariantArray, 0, 1>
{
  ezVariantAdapter(ezVariant& value)
    : m_value(value)
  {
  }

  operator ezVariantArray&() { return m_value.GetWritable<ezVariantArray>(); }
  operator ezVariantArray*() { return m_value.IsValid() ? &m_value.GetWritable<ezVariantArray>() : nullptr; }

  ezVariant& m_value;
};

template <class T>
struct ezVariantAdapter<T, ezVariantDictionary, 0, 0>
{
  ezVariantAdapter(ezVariant& value)
    : m_value(value)
  {
  }

  operator const ezVariantDictionary&() { return m_value.Get<ezVariantDictionary>(); }
  operator const ezVariantDictionary*() { return m_value.IsValid() ? &m_value.Get<ezVariantDictionary>() : nullptr; }

  ezVariant& m_value;
};

template <class T>
struct ezVariantAdapter<T, ezVariantDictionary, 0, 1>
{
  ezVariantAdapter(ezVariant& value)
    : m_value(value)
  {
  }

  operator ezVariantDictionary&() { return m_value.GetWritable<ezVariantDictionary>(); }
  operator ezVariantDictionary*() { return m_value.IsValid() ? &m_value.GetWritable<ezVariantDictionary>() : nullptr; }

  ezVariant& m_value;
};

template <>
struct ezVariantAdapter<const char*, const char*, 1, 0>
{
  ezVariantAdapter(ezVariant& value)
    : m_value(value)
  {
  }

  operator const char*() { return m_value.IsValid() ? m_value.Get<ezString>().GetData() : nullptr; }

  ezVariant& m_value;
};

template <class T>
struct ezVariantAdapter<T, ezStringView, 1, 0>
{
  ezVariantAdapter(ezVariant& value)
    : m_value(value)
  {
  }

  operator const ezStringView() { return m_value.IsA<ezStringView>() ? m_value.Get<ezStringView>() : m_value.Get<ezString>().GetView(); }

  ezVariant& m_value;
};
