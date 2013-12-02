#include <PCH.h>

namespace
{
  struct CustomComparer
  {
    EZ_FORCE_INLINE static bool Less(ezInt32 a, ezInt32 b)
    {
      return a > b;
    }

    EZ_FORCE_INLINE static bool Equal(ezInt32 a, ezInt32 b)
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

  ezSorting<CustomComparer>::QuickSort(a1); // quicksort uses insertion sort for partitions smaller than 16 elements

  for (ezUInt32 i = 1; i < a1.GetCount(); ++i)
  {
    EZ_TEST(a1[i-1] >= a1[i]);
  }

  ezArrayPtr<ezInt32> arrayPtr = a2;
  ezSorting<CustomComparer>::QuickSort(arrayPtr); // quicksort uses insertion sort for partitions smaller than 16 elements

  for (ezUInt32 i = 1; i < arrayPtr.GetCount(); ++i)
  {
    EZ_TEST(arrayPtr[i-1] >= arrayPtr[i]);
  }
}
