
template <typename T, ezUInt32 Size, typename AllocatorWrapper /*= ezDefaultAllocatorWrapper*/>
ezHybridArray<T, Size, AllocatorWrapper>::ezHybridArray()
  : ezDynamicArray<T, AllocatorWrapper>(GetStaticArray(), Size, AllocatorWrapper::GetAllocator())
{
}

template <typename T, ezUInt32 Size, typename AllocatorWrapper /*= ezDefaultAllocatorWrapper*/>
ezHybridArray<T, Size, AllocatorWrapper>::ezHybridArray(ezAllocator* pAllocator)
  : ezDynamicArray<T, AllocatorWrapper>(GetStaticArray(), Size, pAllocator)
{
}

template <typename T, ezUInt32 Size, typename AllocatorWrapper /*= ezDefaultAllocatorWrapper*/>
ezHybridArray<T, Size, AllocatorWrapper>::ezHybridArray(const ezHybridArray<T, Size, AllocatorWrapper>& other)
  : ezDynamicArray<T, AllocatorWrapper>(GetStaticArray(), Size, AllocatorWrapper::GetAllocator())
{
  *this = other;
}

template <typename T, ezUInt32 Size, typename AllocatorWrapper /*= ezDefaultAllocatorWrapper*/>
ezHybridArray<T, Size, AllocatorWrapper>::ezHybridArray(const ezArrayPtr<const T>& other)
  : ezDynamicArray<T, AllocatorWrapper>(GetStaticArray(), Size, AllocatorWrapper::GetAllocator())
{
  *this = other;
}

template <typename T, ezUInt32 Size, typename AllocatorWrapper /*= ezDefaultAllocatorWrapper*/>
ezHybridArray<T, Size, AllocatorWrapper>::ezHybridArray(ezHybridArray<T, Size, AllocatorWrapper>&& other) noexcept
  : ezDynamicArray<T, AllocatorWrapper>(GetStaticArray(), Size, other.GetAllocator())
{
  *this = std::move(other);
}

template <typename T, ezUInt32 Size, typename AllocatorWrapper /*= ezDefaultAllocatorWrapper*/>
void ezHybridArray<T, Size, AllocatorWrapper>::operator=(const ezHybridArray<T, Size, AllocatorWrapper>& rhs)
{
  ezDynamicArray<T, AllocatorWrapper>::operator=(rhs);
}

template <typename T, ezUInt32 Size, typename AllocatorWrapper /*= ezDefaultAllocatorWrapper*/>
void ezHybridArray<T, Size, AllocatorWrapper>::operator=(const ezArrayPtr<const T>& rhs)
{
  ezDynamicArray<T, AllocatorWrapper>::operator=(rhs);
}

template <typename T, ezUInt32 Size, typename AllocatorWrapper /*= ezDefaultAllocatorWrapper*/>
void ezHybridArray<T, Size, AllocatorWrapper>::operator=(ezHybridArray<T, Size, AllocatorWrapper>&& rhs) noexcept
{
  ezDynamicArray<T, AllocatorWrapper>::operator=(std::move(rhs));
}

//////////////////////////////////////////////////////////////////////////

template <typename T, ezUInt32 Size>
ezTempHybridArray<T, Size>::ezTempHybridArray()
  : ezHybridArray<T, Size>(ezTempAllocator::Get())
{
}

template <typename T, ezUInt32 Size>
template <typename AllocatorWrapper>
ezTempHybridArray<T, Size>::ezTempHybridArray(const ezHybridArray<T, Size, AllocatorWrapper>& other)
  : ezHybridArray<T, Size>(ezTempAllocator::Get())
{
  *this = other;
}

template <typename T, ezUInt32 Size>
ezTempHybridArray<T, Size>::ezTempHybridArray(const ezArrayPtr<const T>& other)
  : ezHybridArray<T, Size>(ezTempAllocator::Get())
{
  *this = other;
}

template <typename T, ezUInt32 Size>
template <typename AllocatorWrapper>
void ezTempHybridArray<T, Size>::operator=(const ezHybridArray<T, Size, AllocatorWrapper>& rhs)
{
  ezDynamicArray<T>::operator=(rhs);
}

template <typename T, ezUInt32 Size>
void ezTempHybridArray<T, Size>::operator=(const ezArrayPtr<const T>& rhs)
{
  ezDynamicArray<T>::operator=(rhs);
}

template <typename T, ezUInt32 Size>
void ezTempHybridArray<T, Size>::operator=(ezHybridArray<T, Size>&& rhs) noexcept
{
  ezDynamicArray<T>::operator=(std::move(rhs));
}
