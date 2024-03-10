#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Types/VarianceTypes.h>
#include <Foundation/Types/VariantTypeRegistry.h>

// ezAllocator::Stats

void operator<<(ezStreamWriter& inout_stream, const ezAllocator::Stats& rhs)
{
  inout_stream << rhs.m_uiNumAllocations;
  inout_stream << rhs.m_uiNumDeallocations;
  inout_stream << rhs.m_uiAllocationSize;
}

void operator>>(ezStreamReader& inout_stream, ezAllocator::Stats& rhs)
{
  inout_stream >> rhs.m_uiNumAllocations;
  inout_stream >> rhs.m_uiNumDeallocations;
  inout_stream >> rhs.m_uiAllocationSize;
}

// ezTime

void operator<<(ezStreamWriter& inout_stream, ezTime value)
{
  inout_stream << value.GetSeconds();
}

void operator>>(ezStreamReader& inout_stream, ezTime& ref_value)
{
  double d = 0;
  inout_stream.ReadQWordValue(&d).IgnoreResult();

  ref_value = ezTime::MakeFromSeconds(d);
}

// ezUuid

void operator<<(ezStreamWriter& inout_stream, const ezUuid& value)
{
  inout_stream << value.m_uiHigh;
  inout_stream << value.m_uiLow;
}

void operator>>(ezStreamReader& inout_stream, ezUuid& ref_value)
{
  inout_stream >> ref_value.m_uiHigh;
  inout_stream >> ref_value.m_uiLow;
}

// ezHashedString

void operator<<(ezStreamWriter& inout_stream, const ezHashedString& sValue)
{
  inout_stream.WriteString(sValue.GetView()).AssertSuccess();
}

void operator>>(ezStreamReader& inout_stream, ezHashedString& ref_sValue)
{
  ezStringBuilder sTemp;
  inout_stream >> sTemp;
  ref_sValue.Assign(sTemp);
}

// ezTempHashedString

void operator<<(ezStreamWriter& inout_stream, const ezTempHashedString& sValue)
{
  inout_stream << (ezUInt64)sValue.GetHash();
}

void operator>>(ezStreamReader& inout_stream, ezTempHashedString& ref_sValue)
{
  ezUInt64 hash;
  inout_stream >> hash;
  ref_sValue = ezTempHashedString(hash);
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
  m_pStream->WriteBytes(data.GetData(), data.GetCount()).AssertSuccess();
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
  EZ_MSVC_ANALYSIS_ASSUME(pType != nullptr);
  EZ_MSVC_ANALYSIS_ASSUME(pTypeInfo != nullptr);
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

void operator<<(ezStreamWriter& inout_stream, const ezVariant& value)
{
  ezUInt8 variantVersion = (ezUInt8)ezGetStaticRTTI<ezVariant>()->GetTypeVersion();
  inout_stream << variantVersion;
  ezVariant::Type::Enum type = value.GetType();
  ezUInt8 typeStorage = type;
  if (typeStorage == ezVariantType::StringView)
    typeStorage = ezVariantType::String;
  inout_stream << typeStorage;

  if (type != ezVariant::Type::Invalid)
  {
    WriteValueFunc func;
    func.m_pStream = &inout_stream;
    func.m_pValue = &value;

    ezVariant::DispatchTo(func, type);
  }
}

void operator>>(ezStreamReader& inout_stream, ezVariant& ref_value)
{
  ezUInt8 variantVersion;
  inout_stream >> variantVersion;
  EZ_ASSERT_DEBUG(ezGetStaticRTTI<ezVariant>()->GetTypeVersion() == variantVersion, "Older variant serialization not supported!");

  ezUInt8 typeStorage;
  inout_stream >> typeStorage;
  ezVariant::Type::Enum type = (ezVariant::Type::Enum)typeStorage;

  if (type != ezVariant::Type::Invalid)
  {
    ReadValueFunc func;
    func.m_pStream = &inout_stream;
    func.m_pValue = &ref_value;

    ezVariant::DispatchTo(func, type);
  }
  else
  {
    ref_value = ezVariant();
  }
}

// ezTimestamp

void operator<<(ezStreamWriter& inout_stream, ezTimestamp value)
{
  inout_stream << value.GetInt64(ezSIUnitOfTime::Microsecond);
}

void operator>>(ezStreamReader& inout_stream, ezTimestamp& ref_value)
{
  ezInt64 value;
  inout_stream >> value;

  ref_value = ezTimestamp::MakeFromInt(value, ezSIUnitOfTime::Microsecond);
}

// ezVarianceTypeFloat

void operator<<(ezStreamWriter& inout_stream, const ezVarianceTypeFloat& value)
{
  inout_stream << value.m_fVariance;
  inout_stream << value.m_Value;
}
void operator>>(ezStreamReader& inout_stream, ezVarianceTypeFloat& ref_value)
{
  inout_stream >> ref_value.m_fVariance;
  inout_stream >> ref_value.m_Value;
}

// ezVarianceTypeTime

void operator<<(ezStreamWriter& inout_stream, const ezVarianceTypeTime& value)
{
  inout_stream << value.m_fVariance;
  inout_stream << value.m_Value;
}
void operator>>(ezStreamReader& inout_stream, ezVarianceTypeTime& ref_value)
{
  inout_stream >> ref_value.m_fVariance;
  inout_stream >> ref_value.m_Value;
}

// ezVarianceTypeAngle

void operator<<(ezStreamWriter& inout_stream, const ezVarianceTypeAngle& value)
{
  inout_stream << value.m_fVariance;
  inout_stream << value.m_Value;
}
void operator>>(ezStreamReader& inout_stream, ezVarianceTypeAngle& ref_value)
{
  inout_stream >> ref_value.m_fVariance;
  inout_stream >> ref_value.m_Value;
}
