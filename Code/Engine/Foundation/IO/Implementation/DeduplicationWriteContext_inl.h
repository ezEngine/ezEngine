
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
    static const T* GetAddress(const T* pObj) { return pObj; }
  };
} // namespace ezInternal

template <typename T>
EZ_ALWAYS_INLINE ezResult ezDeduplicationWriteContext::WriteObject(ezStreamWriter& inout_stream, const T& obj)
{
  return WriteObjectInternal(inout_stream, ezInternal::WriteObjectHelper<T>::GetAddress(obj));
}

template <typename T>
EZ_ALWAYS_INLINE ezResult ezDeduplicationWriteContext::WriteObject(ezStreamWriter& inout_stream, const ezSharedPtr<T>& pObject)
{
  return WriteObjectInternal(inout_stream, pObject.Borrow());
}

template <typename T>
EZ_ALWAYS_INLINE ezResult ezDeduplicationWriteContext::WriteObject(ezStreamWriter& inout_stream, const ezUniquePtr<T>& pObject)
{
  return WriteObjectInternal(inout_stream, pObject.Borrow());
}

template <typename ArrayType, typename ValueType>
ezResult ezDeduplicationWriteContext::WriteArray(ezStreamWriter& inout_stream, const ezArrayBase<ValueType, ArrayType>& array)
{
  const ezUInt64 uiCount = array.GetCount();
  EZ_SUCCEED_OR_RETURN(inout_stream.WriteQWordValue(&uiCount));

  for (ezUInt32 i = 0; i < static_cast<ezUInt32>(uiCount); ++i)
  {
    EZ_SUCCEED_OR_RETURN(WriteObject(inout_stream, array[i]));
  }

  return EZ_SUCCESS;
}

template <typename KeyType, typename Comparer>
ezResult ezDeduplicationWriteContext::WriteSet(ezStreamWriter& inout_stream, const ezSetBase<KeyType, Comparer>& set)
{
  const ezUInt64 uiWriteSize = set.GetCount();
  EZ_SUCCEED_OR_RETURN(inout_stream.WriteQWordValue(&uiWriteSize));

  for (const auto& item : set)
  {
    EZ_SUCCEED_OR_RETURN(WriteObject(inout_stream, item));
  }

  return EZ_SUCCESS;
}

template <typename KeyType, typename ValueType, typename Comparer>
ezResult ezDeduplicationWriteContext::WriteMap(ezStreamWriter& inout_stream, const ezMapBase<KeyType, ValueType, Comparer>& map, WriteMapMode mode)
{
  const ezUInt64 uiWriteSize = map.GetCount();
  EZ_SUCCEED_OR_RETURN(inout_stream.WriteQWordValue(&uiWriteSize));

  if (mode == WriteMapMode::DedupKey)
  {
    for (auto It = map.GetIterator(); It.IsValid(); ++It)
    {
      EZ_SUCCEED_OR_RETURN(WriteObject(inout_stream, It.Key()));
      EZ_SUCCEED_OR_RETURN(ezStreamWriterUtil::Serialize<ValueType>(inout_stream, It.Value()));
    }
  }
  else if (mode == WriteMapMode::DedupValue)
  {
    for (auto It = map.GetIterator(); It.IsValid(); ++It)
    {
      EZ_SUCCEED_OR_RETURN(ezStreamWriterUtil::Serialize<KeyType>(inout_stream, It.Key()));
      EZ_SUCCEED_OR_RETURN(WriteObject(inout_stream, It.Value()));
    }
  }
  else
  {
    for (auto It = map.GetIterator(); It.IsValid(); ++It)
    {
      EZ_SUCCEED_OR_RETURN(WriteObject(inout_stream, It.Key()));
      EZ_SUCCEED_OR_RETURN(WriteObject(inout_stream, It.Value()));
    }
  }

  return EZ_SUCCESS;
}

template <typename T>
ezResult ezDeduplicationWriteContext::WriteObjectInternal(ezStreamWriter& stream, const T* pObject)
{
  ezUInt32 uiIndex = ezInvalidIndex;

  if (pObject)
  {
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
  }
  else
  {
    stream << false;
    stream << ezInvalidIndex;
  }

  return EZ_SUCCESS;
}
