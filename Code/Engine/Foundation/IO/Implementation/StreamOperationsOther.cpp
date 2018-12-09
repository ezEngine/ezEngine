#include <PCH.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>

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
  Stream.ReadQWordValue(&d);

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
  /// \todo Could be more efficient, ie add a "WriteString" method that can take a length+data and use that here

  Stream << Value.GetData();
}

void operator>>(ezStreamReader& Stream, ezHashedString& Value)
{
  ezStringBuilder sTemp;
  Stream >> sTemp;
  Value.Assign(sTemp.GetData());
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
inline void WriteValueFunc::operator()<ezReflectedClass*>()
{
  EZ_REPORT_FAILURE("Type 'ezReflectedClass*' not supported in serialization.");
}

template <>
inline void WriteValueFunc::operator()<void*>()
{
  EZ_REPORT_FAILURE("Type 'void*' not supported in serialization.");
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
  m_pStream->WriteBytes(data.GetData(), data.GetCount());
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
inline void ReadValueFunc::operator()<ezReflectedClass*>()
{
  EZ_REPORT_FAILURE("Type 'ezReflectedClass*' not supported in serialization.");
}

template <>
inline void ReadValueFunc::operator()<void*>()
{
  EZ_REPORT_FAILURE("Type 'void*' not supported in serialization.");
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

EZ_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_StreamOperationsOther);
