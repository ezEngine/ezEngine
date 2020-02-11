#include <Foundation/Containers/HybridArray.h>

template <typename T, ezUInt32 Size, typename AllocatorWrapper /*= ezDefaultAllocatorWrapper*/>
ezHybridArray<T, Size, AllocatorWrapper>::ezHybridArray()
  : ezDynamicArray<T, AllocatorWrapper>(GetStaticArray(), Size, AllocatorWrapper::GetAllocator())
{
}

template <typename T, ezUInt32 Size, typename AllocatorWrapper /*= ezDefaultAllocatorWrapper*/>
ezHybridArray<T, Size, AllocatorWrapper>::ezHybridArray(ezAllocatorBase* pAllocator)
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
ezHybridArray<T, Size, AllocatorWrapper>::ezHybridArray(ezHybridArray<T, Size, AllocatorWrapper>&& other)
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
void ezHybridArray<T, Size, AllocatorWrapper>::operator=(ezHybridArray<T, Size, AllocatorWrapper>&& rhs)
{
  ezDynamicArray<T, AllocatorWrapper>::operator=(std::move(rhs));
}
