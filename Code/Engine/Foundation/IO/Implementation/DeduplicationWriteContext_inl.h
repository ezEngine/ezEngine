
#include <Foundation/IO/Stream.h>

namespace ezInternal
{
  // This internal helper is needed to differentiate between reference and pointer which is not possible with regular function overloading
  // in this case.
  template <typename T>
  struct WriteObjectHelper
  {
    static const T* GetAddress(const T& obj) { return &obj; }
  };

  template <typename T>
  struct WriteObjectHelper<T*>
  {
    static const T* GetAddress(const T* obj) { return obj; }
  };
} // namespace ezInternal

template <typename T>
EZ_ALWAYS_INLINE ezResult ezDeduplicationWriteContext::WriteObject(ezStreamWriter& stream, const T& obj)
{
  return WriteObjectInternal(stream, ezInternal::WriteObjectHelper<T>::GetAddress(obj));
}

template <typename T>
EZ_ALWAYS_INLINE ezResult ezDeduplicationWriteContext::WriteObject(ezStreamWriter& stream, const ezSharedPtr<T>& pObject)
{
  return WriteObjectInternal(stream, pObject.Borrow());
}

template <typename T>
EZ_ALWAYS_INLINE ezResult ezDeduplicationWriteContext::WriteObject(ezStreamWriter& stream, const ezUniquePtr<T>& pObject)
{
  return WriteObjectInternal(stream, pObject.Borrow());
}

template <typename ArrayType, typename ValueType>
ezResult ezDeduplicationWriteContext::WriteArray(ezStreamWriter& stream, const ezArrayBase<ValueType, ArrayType>& Array)
{
  const ezUInt64 uiCount = Array.GetCount();
  stream.WriteQWordValue(&uiCount);

  for (ezUInt32 i = 0; i < static_cast<ezUInt32>(uiCount); ++i)
  {
    EZ_SUCCEED_OR_RETURN(WriteObject(stream, Array[i]));
  }

  return EZ_SUCCESS;
}

template <typename KeyType, typename Comparer>
ezResult ezDeduplicationWriteContext::WriteSet(ezStreamWriter& stream, const ezSetBase<KeyType, Comparer>& Set)
{
  const ezUInt64 uiWriteSize = Set.GetCount();
  stream.WriteQWordValue(&uiWriteSize);

  for (const auto& item : Set)
  {
    EZ_SUCCEED_OR_RETURN(WriteObject(stream, item));
  }

  return EZ_SUCCESS;
}

template <typename KeyType, typename ValueType, typename Comparer>
ezResult ezDeduplicationWriteContext::WriteMap(ezStreamWriter& stream, const ezMapBase<KeyType, ValueType, Comparer>& Map,
                                               WriteMapMode mode)
{
  const ezUInt64 uiWriteSize = Map.GetCount();
  stream.WriteQWordValue(&uiWriteSize);

  if (mode == WriteMapMode::DedupKey)
  {
    for (auto It = Map.GetIterator(); It.IsValid(); ++It)
    {
      EZ_SUCCEED_OR_RETURN(WriteObject(stream, It.Key()));
      EZ_SUCCEED_OR_RETURN(ezStreamWriterUtil::Serialize<ValueType>(stream, It.Value()));
    }
  }
  else if (mode == WriteMapMode::DedupValue)
  {
    for (auto It = Map.GetIterator(); It.IsValid(); ++It)
    {
      EZ_SUCCEED_OR_RETURN(ezStreamWriterUtil::Serialize<KeyType>(stream, It.Key()));
      EZ_SUCCEED_OR_RETURN(WriteObject(stream, It.Value()));
    }
  }
  else
  {
    for (auto It = Map.GetIterator(); It.IsValid(); ++It)
    {
      EZ_SUCCEED_OR_RETURN(WriteObject(stream, It.Key()));
      EZ_SUCCEED_OR_RETURN(WriteObject(stream, It.Value()));
    }
  }

  return EZ_SUCCESS;
}

template <typename T>
ezResult ezDeduplicationWriteContext::WriteObjectInternal(ezStreamWriter& stream, const T* pObject)
{
  ezUInt32 uiIndex = ezInvalidIndex;
  bool bIsRealObject = !m_Objects.TryGetValue(pObject, uiIndex);
  stream << bIsRealObject;

  if (bIsRealObject)
  {
    uiIndex = m_Objects.GetCount();
    m_Objects.Insert(pObject, uiIndex);

    return ezStreamWriterUtil::Serialize<T>(stream, *pObject);
  }
  else
  {
    stream << uiIndex;
  }

  return EZ_SUCCESS;
}
