#include <FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Types/VarianceTypes.h>
#include <Foundation/Types/VariantTypeRegistry.h>

// ezAllocatorBase::Stats

void operator<<(ezStreamWriter& Stream, const ezAllocatorBase::Stats& rhs)
{
  Stream << rhs.m_uiNumAllocations;
  Stream << rhs.m_uiNumDeallocations;
  Stream << rhs.m_uiAllocationSize;
}

void operator>>(ezStreamReader& Stream, ezAllocatorBase::Stats& rhs)
{
  Stream >> rhs.m_uiNumAllocations;
  Stream >> rhs.m_uiNumDeallocations;
  Stream >> rhs.m_uiAllocationSize;
}

// ezTime

void operator<<(ezStreamWriter& Stream, ezTime Value)
{
  Stream << Value.GetSeconds();
}

void operator>>(ezStreamReader& Stream, ezTime& Value)
{
  double d = 0;
  Stream.ReadQWordValue(&d).IgnoreResult();

  Value = ezTime::Seconds(d);
}

// ezUuid

void operator<<(ezStreamWriter& Stream, const ezUuid& Value)
{
  Stream << Value.m_uiHigh;
  Stream << Value.m_uiLow;
}

void operator>>(ezStreamReader& Stream, ezUuid& Value)
{
  Stream >> Value.m_uiHigh;
  Stream >> Value.m_uiLow;
}

// ezHashedString

void operator<<(ezStreamWriter& Stream, const ezHashedString& Value)
{
  Stream.WriteString(Value.GetView()).IgnoreResult();
}

void operator>>(ezStreamReader& Stream, ezHashedString& Value)
{
  ezStringBuilder sTemp;
  Stream >> sTemp;
  Value.Assign(sTemp.GetData());
}

// ezTempHashedString

void operator<<(ezStreamWriter& Stream, const ezTempHashedString& Value)
{
  Stream << (ezUInt64)Value.GetHash();
}

void operator>>(ezStreamReader& Stream, ezTempHashedString& Value)
{
  ezUInt64 hash;
  Stream >> hash;
  Value = ezTempHashedString(hash);
}

// ezVariant

struct WriteValueFunc
{
  template <typename T>
  EZ_ALWAYS_INLINE void operator()()
  {
    (*m_pStream) << m_pValue->Get<T>();
  }

  ezStreamWriter* m_pStream;
  const ezVariant* m_pValue;
};

template <>
EZ_FORCE_INLINE void WriteValueFunc::operator()<ezVariantArray>()
{
  const ezVariantArray& values = m_pValue->Get<ezVariantArray>();
  const ezUInt32 iCount = values.GetCount();
  (*m_pStream) << iCount;
  for (ezUInt32 i = 0; i < iCount; i++)
  {
    (*m_pStream) << values[i];
  }
}

template <>
EZ_FORCE_INLINE void WriteValueFunc::operator()<ezVariantDictionary>()
{
  const ezVariantDictionary& values = m_pValue->Get<ezVariantDictionary>();
  const ezUInt32 iCount = values.GetCount();
  (*m_pStream) << iCount;
  for (auto it = values.GetIterator(); it.IsValid(); ++it)
  {
    (*m_pStream) << it.Key();
    (*m_pStream) << it.Value();
  }
}

template <>
inline void WriteValueFunc::operator()<ezTypedPointer>()
{
  EZ_REPORT_FAILURE("Type 'ezReflectedClass*' not supported in serialization.");
}

template <>
inline void WriteValueFunc::operator()<ezTypedObject>()
{
  ezTypedObject obj = m_pValue->Get<ezTypedObject>();
  if (const ezVariantTypeInfo* pTypeInfo = ezVariantTypeRegistry::GetSingleton()->FindVariantTypeInfo(obj.m_pType))
  {
    (*m_pStream) << obj.m_pType->GetTypeName();
    pTypeInfo->Serialize(*m_pStream, obj.m_pObject);
  }
  else
  {
    EZ_REPORT_FAILURE("The type '{0}' was declared but not defined, add EZ_DEFINE_CUSTOM_VARIANT_TYPE({0}); to a cpp to enable serialization of this variant type.", obj.m_pType->GetTypeName());
  }
}

template <>
EZ_FORCE_INLINE void WriteValueFunc::operator()<ezStringView>()
{
  ezStringBuilder s = m_pValue->Get<ezStringView>();
  (*m_pStream) << s;
}

template <>
EZ_FORCE_INLINE void WriteValueFunc::operator()<ezDataBuffer>()
{
  const ezDataBuffer& data = m_pValue->Get<ezDataBuffer>();
  const ezUInt32 iCount = data.GetCount();
  (*m_pStream) << iCount;
  m_pStream->WriteBytes(data.GetData(), data.GetCount()).IgnoreResult();
}

struct ReadValueFunc
{
  template <typename T>
  EZ_FORCE_INLINE void operator()()
  {
    T value;
    (*m_pStream) >> value;
    *m_pValue = value;
  }

  ezStreamReader* m_pStream;
  ezVariant* m_pValue;
};

template <>
EZ_FORCE_INLINE void ReadValueFunc::operator()<ezVariantArray>()
{
  ezVariantArray values;
  ezUInt32 iCount;
  (*m_pStream) >> iCount;
  values.SetCount(iCount);
  for (ezUInt32 i = 0; i < iCount; i++)
  {
    (*m_pStream) >> values[i];
  }
  *m_pValue = values;
}

template <>
EZ_FORCE_INLINE void ReadValueFunc::operator()<ezVariantDictionary>()
{
  ezVariantDictionary values;
  ezUInt32 iCount;
  (*m_pStream) >> iCount;
  for (ezUInt32 i = 0; i < iCount; i++)
  {
    ezString key;
    ezVariant value;
    (*m_pStream) >> key;
    (*m_pStream) >> value;
    values.Insert(key, value);
  }
  *m_pValue = values;
}

template <>
inline void ReadValueFunc::operator()<ezTypedPointer>()
{
  EZ_REPORT_FAILURE("Type 'ezTypedPointer' not supported in serialization.");
}

template <>
inline void ReadValueFunc::operator()<ezTypedObject>()
{
  ezStringBuilder sType;
  (*m_pStream) >> sType;
  const ezRTTI* pType = ezRTTI::FindTypeByName(sType);
  EZ_ASSERT_DEV(pType, "The type '{0}' could not be found.", sType);
  const ezVariantTypeInfo* pTypeInfo = ezVariantTypeRegistry::GetSingleton()->FindVariantTypeInfo(pType);
  EZ_ASSERT_DEV(pTypeInfo, "The type '{0}' was declared but not defined, add EZ_DEFINE_CUSTOM_VARIANT_TYPE({0}); to a cpp to enable serialization of this variant type.", sType);
  void* pObject = pType->GetAllocator()->Allocate<void>();
  pTypeInfo->Deserialize(*m_pStream, pObject);
  m_pValue->MoveTypedObject(pObject, pType);
}

template <>
inline void ReadValueFunc::operator()<ezStringView>()
{
  EZ_REPORT_FAILURE("Type 'ezStringView' not supported in serialization.");
}

template <>
EZ_FORCE_INLINE void ReadValueFunc::operator()<ezDataBuffer>()
{
  ezDataBuffer data;
  ezUInt32 iCount;
  (*m_pStream) >> iCount;
  data.SetCountUninitialized(iCount);

  m_pStream->ReadBytes(data.GetData(), iCount);
  *m_pValue = data;
}

void operator<<(ezStreamWriter& Stream, const ezVariant& Value)
{
  ezUInt8 variantVersion = (ezUInt8)ezGetStaticRTTI<ezVariant>()->GetTypeVersion();
  Stream << variantVersion;
  ezVariant::Type::Enum type = Value.GetType();
  ezUInt8 typeStorage = type;
  if (typeStorage == ezVariantType::StringView)
    typeStorage = ezVariantType::String;
  Stream << typeStorage;

  if (type != ezVariant::Type::Invalid)
  {
    WriteValueFunc func;
    func.m_pStream = &Stream;
    func.m_pValue = &Value;

    ezVariant::DispatchTo(func, type);
  }
}

void operator>>(ezStreamReader& Stream, ezVariant& Value)
{
  ezUInt8 variantVersion;
  Stream >> variantVersion;
  EZ_ASSERT_DEBUG(ezGetStaticRTTI<ezVariant>()->GetTypeVersion() == variantVersion, "Older variant serialization not supported!");

  ezUInt8 typeStorage;
  Stream >> typeStorage;
  ezVariant::Type::Enum type = (ezVariant::Type::Enum)typeStorage;

  if (type != ezVariant::Type::Invalid)
  {
    ReadValueFunc func;
    func.m_pStream = &Stream;
    func.m_pValue = &Value;

    ezVariant::DispatchTo(func, type);
  }
  else
  {
    Value = ezVariant();
  }
}

// ezTimestamp

void operator<<(ezStreamWriter& Stream, ezTimestamp Value)
{
  Stream << Value.GetInt64(ezSIUnitOfTime::Microsecond);
}

void operator>>(ezStreamReader& Stream, ezTimestamp& Value)
{
  ezInt64 value;
  Stream >> value;

  Value.SetInt64(value, ezSIUnitOfTime::Microsecond);
}

// ezVarianceTypeFloat

void operator<<(ezStreamWriter& Stream, const ezVarianceTypeFloat& Value)
{
  Stream << Value.m_fVariance;
  Stream << Value.m_Value;
}
void operator>>(ezStreamReader& Stream, ezVarianceTypeFloat& Value)
{
  Stream >> Value.m_fVariance;
  Stream >> Value.m_Value;
}

// ezVarianceTypeTime

void operator<<(ezStreamWriter& Stream, const ezVarianceTypeTime& Value)
{
  Stream << Value.m_fVariance;
  Stream << Value.m_Value;
}
void operator>>(ezStreamReader& Stream, ezVarianceTypeTime& Value)
{
  Stream >> Value.m_fVariance;
  Stream >> Value.m_Value;
}

// ezVarianceTypeAngle

void operator<<(ezStreamWriter& Stream, const ezVarianceTypeAngle& Value)
{
  Stream << Value.m_fVariance;
  Stream << Value.m_Value;
}
void operator>>(ezStreamReader& Stream, ezVarianceTypeAngle& Value)
{
  Stream >> Value.m_fVariance;
  Stream >> Value.m_Value;
}
EZ_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_StreamOperationsOther);
