
#include <Foundation/IO/Stream.h>

template <typename T>
EZ_ALWAYS_INLINE ezResult ezDeduplicationReadContext::ReadObjectInplace(ezStreamReader& inout_stream, T& inout_obj)
{
  return ReadObject(inout_stream, inout_obj, nullptr);
}

template <typename T>
ezResult ezDeduplicationReadContext::ReadObject(ezStreamReader& inout_stream, T& obj, ezAllocator* pAllocator)
{
  bool bIsRealObject;
  inout_stream >> bIsRealObject;

  EZ_ASSERT_DEV(bIsRealObject, "Reading an object inplace only works for the first occurrence");

  EZ_SUCCEED_OR_RETURN(ezStreamReaderUtil::Deserialize<T>(inout_stream, obj));

  m_Objects.PushBack(&obj);

  return EZ_SUCCESS;
}

template <typename T>
ezResult ezDeduplicationReadContext::ReadObject(ezStreamReader& inout_stream, T*& ref_pObject, ezAllocator* pAllocator)
{
  bool bIsRealObject;
  inout_stream >> bIsRealObject;

  if (bIsRealObject)
  {
    EZ_ASSERT_DEBUG(pAllocator != nullptr, "Valid allocator required");
    ref_pObject = EZ_NEW(pAllocator, T);
    EZ_SUCCEED_OR_RETURN(ezStreamReaderUtil::Deserialize<T>(inout_stream, *ref_pObject));

    m_Objects.PushBack(ref_pObject);
  }
  else
  {
    ezUInt32 uiIndex;
    inout_stream >> uiIndex;

    if (uiIndex < m_Objects.GetCount())
    {
      ref_pObject = static_cast<T*>(m_Objects[uiIndex]);
    }
    else if (uiIndex == ezInvalidIndex)
    {
      ref_pObject = nullptr;
    }
    else
    {
      return EZ_FAILURE;
    }
  }

  return EZ_SUCCESS;
}

template <typename T>
ezResult ezDeduplicationReadContext::ReadObject(ezStreamReader& inout_stream, ezSharedPtr<T>& ref_pObject, ezAllocator* pAllocator)
{
  T* ptr = nullptr;
  if (ReadObject(inout_stream, ptr, pAllocator).Succeeded())
  {
    ref_pObject = ezSharedPtr<T>(ptr, pAllocator);
    return EZ_SUCCESS;
  }
  return EZ_FAILURE;
}

template <typename T>
ezResult ezDeduplicationReadContext::ReadObject(ezStreamReader& inout_stream, ezUniquePtr<T>& ref_pObject, ezAllocator* pAllocator)
{
  T* ptr = nullptr;
  if (ReadObject(inout_stream, ptr, pAllocator).Succeeded())
  {
    ref_pObject = std::move(ezUniquePtr<T>(ptr, pAllocator));
    return EZ_SUCCESS;
  }
  return EZ_FAILURE;
}

template <typename ArrayType, typename ValueType>
ezResult ezDeduplicationReadContext::ReadArray(ezStreamReader& inout_stream, ezArrayBase<ValueType, ArrayType>& ref_array, ezAllocator* pAllocator)
{
  ezUInt64 uiCount = 0;
  EZ_SUCCEED_OR_RETURN(inout_stream.ReadQWordValue(&uiCount));

  EZ_ASSERT_DEV(uiCount < std::numeric_limits<ezUInt32>::max(), "Containers currently use 32 bit for counts internally. Value from file is too large.");

  ref_array.Clear();

  if (uiCount > 0)
  {
    static_cast<ArrayType&>(ref_array).Reserve(static_cast<ezUInt32>(uiCount));

    for (ezUInt32 i = 0; i < static_cast<ezUInt32>(uiCount); ++i)
    {
      EZ_SUCCEED_OR_RETURN(ReadObject(inout_stream, ref_array.ExpandAndGetRef(), pAllocator));
    }
  }

  return EZ_SUCCESS;
}

template <typename KeyType, typename Comparer>
ezResult ezDeduplicationReadContext::ReadSet(ezStreamReader& inout_stream, ezSetBase<KeyType, Comparer>& ref_set, ezAllocator* pAllocator)
{
  ezUInt64 uiCount = 0;
  EZ_SUCCEED_OR_RETURN(inout_stream.ReadQWordValue(&uiCount));

  EZ_ASSERT_DEV(uiCount < std::numeric_limits<ezUInt32>::max(), "Containers currently use 32 bit for counts internally. Value from file is too large.");

  ref_set.Clear();

  for (ezUInt32 i = 0; i < static_cast<ezUInt32>(uiCount); ++i)
  {
    KeyType key;
    EZ_SUCCEED_OR_RETURN(ReadObject(inout_stream, key, pAllocator));

    ref_set.Insert(std::move(key));
  }

  return EZ_SUCCESS;
}

namespace ezInternal
{
  // Internal helper to prevent the compiler from trying to find a de-serialization method for pointer types or other types which don't have
  // one.
  struct DeserializeHelper
  {
    template <typename T>
    static auto Deserialize(ezStreamReader& inout_stream, T& ref_obj, int) -> decltype(ezStreamReaderUtil::Deserialize(inout_stream, ref_obj))
    {
      return ezStreamReaderUtil::Deserialize(inout_stream, ref_obj);
    }

    template <typename T>
    static ezResult Deserialize(ezStreamReader& inout_stream, T& ref_obj, float)
    {
      EZ_REPORT_FAILURE("No deserialize method available");
      return EZ_FAILURE;
    }
  };
} // namespace ezInternal

template <typename KeyType, typename ValueType, typename Comparer>
ezResult ezDeduplicationReadContext::ReadMap(ezStreamReader& inout_stream, ezMapBase<KeyType, ValueType, Comparer>& ref_map, ReadMapMode mode, ezAllocator* pKeyAllocator, ezAllocator* pValueAllocator)
{
  ezUInt64 uiCount = 0;
  EZ_SUCCEED_OR_RETURN(inout_stream.ReadQWordValue(&uiCount));

  EZ_ASSERT_DEV(uiCount < std::numeric_limits<ezUInt32>::max(), "Containers currently use 32 bit for counts internally. Value from file is too large.");

  ref_map.Clear();

  if (mode == ReadMapMode::DedupKey)
  {
    for (ezUInt32 i = 0; i < static_cast<ezUInt32>(uiCount); ++i)
    {
      KeyType key;
      ValueType value;
      EZ_SUCCEED_OR_RETURN(ReadObject(inout_stream, key, pKeyAllocator));
      EZ_SUCCEED_OR_RETURN(ezInternal::DeserializeHelper::Deserialize<ValueType>(inout_stream, value, 0));

      ref_map.Insert(std::move(key), std::move(value));
    }
  }
  else if (mode == ReadMapMode::DedupValue)
  {
    for (ezUInt32 i = 0; i < static_cast<ezUInt32>(uiCount); ++i)
    {
      KeyType key;
      ValueType value;
      EZ_SUCCEED_OR_RETURN(ezInternal::DeserializeHelper::Deserialize<KeyType>(inout_stream, key, 0));
      EZ_SUCCEED_OR_RETURN(ReadObject(inout_stream, value, pValueAllocator));

      ref_map.Insert(std::move(key), std::move(value));
    }
  }
  else
  {
    for (ezUInt32 i = 0; i < static_cast<ezUInt32>(uiCount); ++i)
    {
      KeyType key;
      ValueType value;
      EZ_SUCCEED_OR_RETURN(ReadObject(inout_stream, key, pKeyAllocator));
      EZ_SUCCEED_OR_RETURN(ReadObject(inout_stream, value, pValueAllocator));

      ref_map.Insert(std::move(key), std::move(value));
    }
  }

  return EZ_SUCCESS;
}
