#include <Foundation/PCH.h>
#include <Foundation/Basics/Types/Variant.h>

#if EZ_ENABLED(EZ_PLATFORM_64BIT)
  EZ_CHECK_AT_COMPILETIME(sizeof(ezVariant) == 24);
#else
  EZ_CHECK_AT_COMPILETIME(sizeof(ezVariant) == 20);
#endif

const ezVariant ezVariant::Invalid;

/// functors

struct CompareFunc
{
  template <typename T>
  EZ_FORCE_INLINE bool operator()() const
  {
    return m_pThis->Cast<T>() == m_pOther->Cast<T>();
  }

  const ezVariant* m_pThis;
  const ezVariant* m_pOther;
};

struct ConvertFunc
{
  template <typename T>
  EZ_FORCE_INLINE ezVariant operator()() const
  {
    T result;
    ezVariantConversion::To(result, *m_pThis);
    return result;
  }

  const ezVariant* m_pThis;
};

struct DestructFunc
{
  template <typename T>
  EZ_FORCE_INLINE void operator()()
  {
    ezMemoryUtils::Destruct(&m_pThis->Cast<T>(), 1);
  }

  ezVariant* m_pThis;
};

struct CopyFunc
{
  template <typename T>
  EZ_FORCE_INLINE void operator()()
  {
    m_pThis->Init(m_pOther->Cast<T>());
  }

  ezVariant* m_pThis;
  const ezVariant* m_pOther;
};


/// public methods

bool ezVariant::operator==(const ezVariant& other) const
{
  if (m_Type != other.m_Type)
    return false;

  CompareFunc compareFunc;
  compareFunc.m_pThis = this;
  compareFunc.m_pOther = &other;

  return DispatchTo<bool>(compareFunc, GetType());
}

bool ezVariant::CanConvertTo(Type::Enum type) const
{
  if (m_Type == type) 
    return true;

  if (m_Type == Type::Invalid)
    return false;
  
  if (type <= Type::Double && m_Type <= Type::Double)
    return true;

  if (type == Type::String || m_Type == Type::String)
    return true;

  return false;
}

ezVariant ezVariant::ConvertTo(Type::Enum type) const
{
  EZ_ASSERT(CanConvertTo(type), "Cannot convert to type '%d'", type);

  if (m_Type == type) 
    return *this;

  ConvertFunc convertFunc;
  convertFunc.m_pThis = this;

  return DispatchTo<ezVariant>(convertFunc, type);
}

/// private methods

void ezVariant::Release()
{
  if (m_bIsShared)
  {
    if (m_Data.shared->m_uiRef.Decrement() == 0)
    {
      EZ_DEFAULT_DELETE(m_Data.shared);
    }
  }
  else if (IsValid())
  {
    DestructFunc destructFunc;
    destructFunc.m_pThis = this;

    DispatchTo<void>(destructFunc, GetType());
  }
}

void ezVariant::CopyFrom(const ezVariant& other)
{
  m_Type = other.m_Type;
  m_bIsShared = other.m_bIsShared;
  
  if (m_bIsShared)
  {
    m_Data.shared = other.m_Data.shared;
    m_Data.shared->m_uiRef.Increment();
  }
  else if (other.IsValid())
  {
    CopyFunc copyFunc;
    copyFunc.m_pThis = this;
    copyFunc.m_pOther = &other;

    DispatchTo<void>(copyFunc, GetType());
  }
}
