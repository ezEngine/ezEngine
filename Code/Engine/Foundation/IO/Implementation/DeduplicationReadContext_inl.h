
#include <Foundation/IO/Stream.h>

template <typename T>
EZ_ALWAYS_INLINE ezResult ezDeduplicationReadContext::ReadObjectInplace(ezStreamReader& stream, T& obj)
{
  return ReadObject(stream, obj, nullptr);
}

template <typename T>
ezResult ezDeduplicationReadContext::ReadObject(ezStreamReader& stream, T& obj, ezAllocatorBase* pAllocator)
{
  bool bIsRealObject;
  stream >> bIsRealObject;

  EZ_ASSERT_DEV(bIsRealObject, "Reading an object inplace only works for the first occurrence");

  EZ_SUCCEED_OR_RETURN(ezStreamReaderUtil::Deserialize<T>(stream, obj));

  m_Objects.PushBack(&obj);

  return EZ_SUCCESS;
}

template <typename T>
ezResult ezDeduplicationReadContext::ReadObject(ezStreamReader& stream, T*& pObject, ezAllocatorBase* pAllocator)
{
  bool bIsRealObject;
  stream >> bIsRealObject;

  if (bIsRealObject)
  {
    pObject = EZ_NEW(pAllocator, T);
    EZ_SUCCEED_OR_RETURN(ezStreamReaderUtil::Deserialize<T>(stream, *pObject));

    m_Objects.PushBack(pObject);
  }
  else
  {
    ezUInt32 uiIndex;
    stream >> uiIndex;

    if (uiIndex >= m_Objects.GetCount())
      return EZ_FAILURE;

    pObject = static_cast<T*>(m_Objects[uiIndex]);
  }

  return EZ_SUCCESS;
}

template <typename T>
ezResult ezDeduplicationReadContext::ReadObject(ezStreamReader& stream, ezSharedPtr<T>& pObject, ezAllocatorBase* pAllocator)
{
  T* ptr = nullptr;
  if (ReadObject(stream, ptr, pAllocator).Succeeded())
  {
    pObject = ezSharedPtr<T>(ptr, pAllocator);
    return EZ_SUCCESS;
  }
  return EZ_FAILURE;
}

template <typename T>
ezResult ezDeduplicationReadContext::ReadObject(ezStreamReader& stream, ezUniquePtr<T>& pObject, ezAllocatorBase* pAllocator)
{
  T* ptr = nullptr;
  if (ReadObject(stream, ptr, pAllocator).Succeeded())
  {
    pObject = std::move(ezUniquePtr<T>(ptr, pAllocator));
    return EZ_SUCCESS;
  }
  return EZ_FAILURE;
}

template <typename ArrayType, typename ValueType>
ezResult ezDeduplicationReadContext::ReadArray(ezStreamReader& stream, ezArrayBase<ValueType, ArrayType>& Array,
                                               ezAllocatorBase* pAllocator)
{
  ezUInt64 uiCount = 0;
  stream.ReadQWordValue(&uiCount);

  EZ_ASSERT_DEV(uiCount < std::numeric_limits<ezUInt32>::max(),
                "Containers currently use 32 bit for counts internally. Value from file is too large.");

  Array.Clear();

  if (uiCount > 0)
  {
    static_cast<ArrayType&>(Array).Reserve(uiCount);

    for (ezUInt32 i = 0; i < static_cast<ezUInt32>(uiCount); ++i)
    {
      EZ_SUCCEED_OR_RETURN(ReadObject(stream, Array.ExpandAndGetRef(), pAllocator));
    }
  }

  return EZ_SUCCESS;
}

template <typename KeyType, typename Comparer>
ezResult ezDeduplicationReadContext::ReadSet(ezStreamReader& stream, ezSetBase<KeyType, Comparer>& Set, ezAllocatorBase* pAllocator)
{
  ezUInt64 uiCount = 0;
  stream.ReadQWordValue(&uiCount);

  EZ_ASSERT_DEV(uiCount < std::numeric_limits<ezUInt32>::max(),
                "Containers currently use 32 bit for counts internally. Value from file is too large.");

  Set.Clear();

  for (ezUInt32 i = 0; i < static_cast<ezUInt32>(uiCount); ++i)
  {
    KeyType key;
    EZ_SUCCEED_OR_RETURN(ReadObject(stream, key, pAllocator));

    Set.Insert(std::move(key));
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
    static auto Deserialize(ezStreamReader& stream, T& obj, int) -> decltype(ezStreamReaderUtil::Deserialize(stream, obj))
    {
      return ezStreamReaderUtil::Deserialize(stream, obj);
    }

    template <typename T>
    static ezResult Deserialize(ezStreamReader& stream, T& obj, float)
    {
      EZ_REPORT_FAILURE("No deserialize method available");
      return EZ_FAILURE;
    }
  };
} // namespace ezInternal

template <typename KeyType, typename ValueType, typename Comparer>
ezResult ezDeduplicationReadContext::ReadMap(ezStreamReader& stream, ezMapBase<KeyType, ValueType, Comparer>& Map, ReadMapMode mode,
                                             ezAllocatorBase* pKeyAllocator, ezAllocatorBase* pValueAllocator)
{
  ezUInt64 uiCount = 0;
  stream.ReadQWordValue(&uiCount);

  EZ_ASSERT_DEV(uiCount < std::numeric_limits<ezUInt32>::max(),
                "Containers currently use 32 bit for counts internally. Value from file is too large.");

  Map.Clear();

  if (mode == ReadMapMode::DedupKey)
  {
    for (ezUInt32 i = 0; i < static_cast<ezUInt32>(uiCount); ++i)
    {
      KeyType key;
      ValueType value;
      EZ_SUCCEED_OR_RETURN(ReadObject(stream, key, pKeyAllocator));
      EZ_SUCCEED_OR_RETURN(ezInternal::DeserializeHelper::Deserialize<ValueType>(stream, value, 0));

      Map.Insert(std::move(key), std::move(value));
    }
  }
  else if (mode == ReadMapMode::DedupValue)
  {
    for (ezUInt32 i = 0; i < static_cast<ezUInt32>(uiCount); ++i)
    {
      KeyType key;
      ValueType value;
      EZ_SUCCEED_OR_RETURN(ezInternal::DeserializeHelper::Deserialize<KeyType>(stream, key, 0));
      EZ_SUCCEED_OR_RETURN(ReadObject(stream, value, pValueAllocator));

      Map.Insert(std::move(key), std::move(value));
    }
  }
  else
  {
    for (ezUInt32 i = 0; i < static_cast<ezUInt32>(uiCount); ++i)
    {
      KeyType key;
      ValueType value;
      EZ_SUCCEED_OR_RETURN(ReadObject(stream, key, pKeyAllocator));
      EZ_SUCCEED_OR_RETURN(ReadObject(stream, value, pValueAllocator));

      Map.Insert(std::move(key), std::move(value));
    }
  }

  return EZ_SUCCESS;
}
