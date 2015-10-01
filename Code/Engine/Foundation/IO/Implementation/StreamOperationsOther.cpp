#include <Foundation/PCH.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Uuid.h>
#include <Foundation/Types/Variant.h>
#include <Foundation/Reflection/Reflection.h>

// ezAllocatorBase::Stats

void operator<< (ezStreamWriterBase& Stream, const ezAllocatorBase::Stats& rhs)
{
  Stream << rhs.m_uiNumAllocations;
  Stream << rhs.m_uiNumDeallocations;
  Stream << rhs.m_uiAllocationSize;
}

void operator>> (ezStreamReaderBase& Stream, ezAllocatorBase::Stats& rhs)
{
  Stream >> rhs.m_uiNumAllocations;
  Stream >> rhs.m_uiNumDeallocations;
  Stream >> rhs.m_uiAllocationSize;
}

// ezTime

void operator<< (ezStreamWriterBase& Stream, ezTime Value)
{
  Stream << Value.GetSeconds();
}

void operator>> (ezStreamReaderBase& Stream, ezTime& Value)
{
  double d = 0;
  Stream.ReadQWordValue(&d);

  Value = ezTime::Seconds(d);
}

// ezUuid

void operator<< (ezStreamWriterBase& Stream, const ezUuid& Value)
{
  Stream << Value.m_uiHigh;
  Stream << Value.m_uiLow;
}

void operator>> (ezStreamReaderBase& Stream, ezUuid& Value)
{
  Stream >> Value.m_uiHigh;
  Stream >> Value.m_uiLow;
}




struct WriteValueFunc
{
  template <typename T>
  EZ_FORCE_INLINE void operator()()
  {
    (*m_pStream) << m_pValue->Get<T>();
  }

  ezStreamWriterBase* m_pStream;
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
EZ_FORCE_INLINE void WriteValueFunc::operator()<ezReflectedClass*>()
{
  EZ_REPORT_FAILURE("Type 'ezReflectedClass*' not supported in serialization.");
}

template <>
EZ_FORCE_INLINE void WriteValueFunc::operator()<void*>()
{
  EZ_REPORT_FAILURE("Type 'void*' not supported in serialization.");
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

  ezStreamReaderBase* m_pStream;
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
EZ_FORCE_INLINE void ReadValueFunc::operator()<ezReflectedClass*>()
{
  EZ_REPORT_FAILURE("Type 'ezReflectedClass*' not supported in serialization.");
}

template <>
EZ_FORCE_INLINE void ReadValueFunc::operator()<void*>()
{
  EZ_REPORT_FAILURE("Type 'void*' not supported in serialization.");
}


// ezVariant

void operator<< (ezStreamWriterBase& Stream, const ezVariant& Value)
{
  ezUInt8 variantVersion = (ezUInt8)ezGetStaticRTTI<ezVariant>()->GetTypeVersion();
  Stream << variantVersion;
  ezVariant::Type::Enum type = Value.GetType();
  ezUInt8 typeStorage = type;
  Stream << typeStorage;

  if (type != ezVariant::Type::Invalid)
  {
    WriteValueFunc func;
    func.m_pStream = &Stream;
    func.m_pValue = &Value;

    ezVariant::DispatchTo(func, type);
  }
}

void operator>> (ezStreamReaderBase& Stream, ezVariant& Value)
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

