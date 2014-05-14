#include <PCH.h>
#include <Foundation/Containers/DynamicArray.h>

namespace
{
  struct CustomComparer
  {
    EZ_FORCE_INLINE bool Less(ezInt32 a, ezInt32 b) const
    {
      return a > b;
    }

    EZ_FORCE_INLINE bool Equal(ezInt32 a, ezInt32 b) const
    {
      return a == b;
    }
  };
}

EZ_CREATE_SIMPLE_TEST(Algorithm, Sorting)
{
  ezDynamicArray<ezInt32> a1;
    
  for (ezUInt32 i = 0; i < 2000; ++i)
  {
    a1.PushBack(rand() % 100000);
  }

  ezDynamicArray<ezInt32> a2 = a1;

  ezSorting::QuickSort(a1, CustomComparer()); // quicksort uses insertion sort for partitions smaller than 16 elements

  for (ezUInt32 i = 1; i < a1.GetCount(); ++i)
  {
    EZ_TEST_BOOL(a1[i-1] >= a1[i]);
  }

  ezArrayPtr<ezInt32> arrayPtr = a2;
  ezSorting::QuickSort(arrayPtr, CustomComparer()); // quicksort uses insertion sort for partitions smaller than 16 elements

  for (ezUInt32 i = 1; i < arrayPtr.GetCount(); ++i)
  {
    EZ_TEST_BOOL(arrayPtr[i-1] >= arrayPtr[i]);
  }
}
