#pragma once

#include <Foundation/Types/Variant.h>

template <typename T>
struct ezCleanType2
{
  typedef T Type;
  typedef T RttiType;
};

template <typename T>
struct ezCleanType2<ezEnum<T>>
{
  typedef ezEnum<T> Type;
  typedef T RttiType;
};

template <typename T>
struct ezCleanType2<ezBitflags<T>>
{
  typedef ezBitflags<T> Type;
  typedef T RttiType;
};

template <typename T>
struct ezCleanType
{
  typedef typename ezTypeTraits<T>::NonConstReferencePointerType Type;
  typedef typename ezCleanType2<typename ezTypeTraits<T>::NonConstReferencePointerType>::RttiType RttiType;
};

template <>
struct ezCleanType<const char*>
{
  typedef const char* Type;
  typedef const char* RttiType;
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

/// \brief Used to determine if the given type is a
template <class T, class C = typename ezCleanType<T>::Type>
struct ezIsStandardType
{
  enum
  {
    // TODO: Not quite correct.
    value = ezVariant::TypeDeduction<C>::value != ezVariantType::Invalid,
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

/// \brief Used to automatically assign any value to an ezVariant using the assignment rules
/// outlined in ezAbstractFunctionProperty::Execute.
template <class T,                                        ///< Only this parameter needs to be provided, the actual type of the value.
          class C = typename ezCleanType<T>::Type,        ///< Same as T but without the const&* fluff.
          int STANDARD_TYPE = ezIsStandardType<T>::value> ///< Is 1 if T is a ezTypeFlags::StandardType
struct ezVariantAssignmentAdapter
{
  typedef typename ezTypeTraits<T>::NonConstReferencePointerType RealType;
  ezVariantAssignmentAdapter(ezVariant& value)
      : m_value(value)
  {
  }

  void operator=(T* rhs) { m_value = rhs; }
  void operator=(T&& rhs)
  {
    if (m_value.IsValid())
      *static_cast<RealType*>(m_value.ConvertTo<void*>()) = rhs;
  }
  ezVariant& m_value;
};

template <class T, class S>
struct ezVariantAssignmentAdapter<T, ezEnum<S>, 0>
{
  typedef typename ezTypeTraits<T>::NonConstReferencePointerType RealType;
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
  typedef typename ezTypeTraits<T>::NonConstReferencePointerType RealType;
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
  typedef typename ezTypeTraits<T>::NonConstReferencePointerType RealType;
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
template <class T, ///< Only this parameter needs to be provided, the actual type of the argument. Rest is used to force specializations.
          class C = typename ezCleanType<T>::Type,        ///< Same as T but without the const&* fluff.
          int STANDARD_TYPE = ezIsStandardType<T>::value, ///< Is 1 if T is a ezTypeFlags::StandardType
          int OUT_PARAM = ezIsOutParam<T>::value>         ///< Is 1 if T a non-const reference or pointer.
struct ezVariantAdapter
{
  typedef typename ezTypeTraits<T>::NonConstReferencePointerType RealType;

  ezVariantAdapter(ezVariant& value)
      : m_value(value)
  {
  }

  operator RealType&() { return *static_cast<RealType*>(m_value.ConvertTo<void*>()); }

  operator RealType*() { return m_value.IsValid() ? static_cast<RealType*>(m_value.ConvertTo<void*>()) : nullptr; }

  ezVariant& m_value;
};

template <class T, class S>
struct ezVariantAdapter<T, ezEnum<S>, 0, 0>
{
  typedef typename ezTypeTraits<T>::NonConstReferencePointerType RealType;
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
  typedef typename ezTypeTraits<T>::NonConstReferencePointerType RealType;
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
  typedef typename ezTypeTraits<T>::NonConstReferencePointerType RealType;
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
  typedef typename ezTypeTraits<T>::NonConstReferencePointerType RealType;
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
  typedef typename ezTypeTraits<T>::NonConstReferencePointerType RealType;
  ezVariantAdapter(ezVariant& value)
      : m_value(value)
  {
  }

  operator const C&() { return m_value.Get<RealType>(); }

  operator const C*() { return m_value.IsValid() ? &m_value.Get<RealType>() : nullptr; }

  ezVariant& m_value;
};

template <class T, class C>
struct ezVariantAdapter<T, C, 1, 1>
{
  typedef typename ezTypeTraits<T>::NonConstReferencePointerType RealType;
  ezVariantAdapter(ezVariant& value)
      : m_value(value)
  {
    if (m_value.IsValid())
      m_realValue = m_value.Get<RealType>();
  }
  ~ezVariantAdapter()
  {
    if (m_value.IsValid())
      m_value = m_realValue;
  }

  operator C&() { return m_realValue; }
  operator C*() { return m_value.IsValid() ? &m_realValue : nullptr; }

  ezVariant& m_value;
  RealType m_realValue;
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
