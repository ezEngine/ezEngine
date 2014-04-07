#include <Foundation/PCH.h>
#include <Foundation/Basics/Types/Variant.h>

#if EZ_ENABLED(EZ_PLATFORM_64BIT)
  EZ_CHECK_AT_COMPILETIME(sizeof(ezVariant) == 24);
#else
  EZ_CHECK_AT_COMPILETIME(sizeof(ezVariant) == 20);
#endif

/// functors

struct CompareFunc
{
  template <typename T>
  EZ_FORCE_INLINE void operator()()
  {
    m_bResult = m_pThis->Cast<T>() == m_pOther->Cast<T>();
  }

  const ezVariant* m_pThis;
  const ezVariant* m_pOther;
  bool m_bResult;
};

struct ConvertFunc
{
  template <typename T>
  EZ_FORCE_INLINE void operator()()
  {
    T result;
    ezVariantHelper::To(*m_pThis, result, m_bSuccessful);
    m_Result = result;
  }

  const ezVariant* m_pThis;
  ezVariant m_Result;
  bool m_bSuccessful;
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
  if (IsFloatingPoint(m_Type) && IsNumber(other.m_Type))
  {
    return ConvertNumber<double>() == other.ConvertNumber<double>();
  }
  else if (IsNumber(m_Type) && IsNumber(other.m_Type))
  {
    return ConvertNumber<ezInt64>() == other.ConvertNumber<ezInt64>();
  }
  else if (m_Type == other.m_Type)
  {
    CompareFunc compareFunc;
    compareFunc.m_pThis = this;
    compareFunc.m_pOther = &other;

    DispatchTo(compareFunc, GetType());

    return compareFunc.m_bResult;
  }
    
  return false;
}

bool ezVariant::CanConvertTo(Type::Enum type) const
{
  if (m_Type == type) 
    return true;

  if (!IsValid() || type == Type::Invalid)
    return false;
  
  if (IsNumber(type) && (IsNumber(m_Type) || m_Type == Type::String))
    return true;

  if (type == Type::String && m_Type < Type::VariantArray)
    return true;

  return false;
}

ezVariant ezVariant::ConvertTo(Type::Enum type, ezResult* out_pConversionStatus /* = nullptr*/) const
{
  if (!CanConvertTo(type))
  {
    if (out_pConversionStatus != nullptr)
      *out_pConversionStatus = EZ_FAILURE;

    return ezVariant(); // creates an invalid variant
  }

  if (m_Type == type)
  {
    if (out_pConversionStatus != nullptr)
      *out_pConversionStatus = EZ_SUCCESS;

    return *this;
  }

  ConvertFunc convertFunc;
  convertFunc.m_pThis = this;
  convertFunc.m_bSuccessful = true;

  DispatchTo(convertFunc, type);

  if (out_pConversionStatus != nullptr)
    *out_pConversionStatus = convertFunc.m_bSuccessful ? EZ_SUCCESS : EZ_FAILURE;

  return convertFunc.m_Result;
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

    DispatchTo(destructFunc, GetType());
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

    DispatchTo(copyFunc, GetType());
  }
}


EZ_STATICLINK_FILE(Foundation, Foundation_Basics_Types_Implementation_Variant);

