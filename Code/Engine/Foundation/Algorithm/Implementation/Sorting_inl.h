
template <typename Container, typename Comparer>
void ezSorting::QuickSort(Container& container, const Comparer& comparer)
{
  QuickSort(container, 0, container.GetCount() - 1, comparer);
}

template <typename T, typename Comparer>
void ezSorting::QuickSort(ezArrayPtr<T>& arrayPtr, const Comparer& comparer)
{
  QuickSort(arrayPtr, 0, arrayPtr.GetCount() - 1, comparer);
}

template <typename Container, typename Comparer>
void ezSorting::InsertionSort(Container& container, const Comparer& comparer)
{
  InsertionSort(container, 0, container.GetCount() - 1, comparer);
}

template <typename T, typename Comparer>
void ezSorting::InsertionSort(ezArrayPtr<T>& arrayPtr, const Comparer& comparer)
{
  InsertionSort(arrayPtr, 0, arrayPtr.GetCount() - 1, comparer);
}


template <typename Container, typename Comparer>
void ezSorting::QuickSort(Container& container, ezUInt32 uiStartIndex, ezUInt32 uiEndIndex, const Comparer& comparer)
{
  if (uiStartIndex < uiEndIndex)
  {
    if (uiEndIndex - uiStartIndex <= INSERTION_THRESHOLD)
    {
      InsertionSort(container, uiStartIndex, uiEndIndex, comparer);
    }
    else
    {
      ezUInt32 uiPivotIndex = Partition(container, uiStartIndex, uiEndIndex, comparer);
      
      if (uiStartIndex < uiPivotIndex)
        QuickSort(container, uiStartIndex, uiPivotIndex - 1, comparer);

      QuickSort(container, uiPivotIndex + 1, uiEndIndex, comparer);
    }
  }
}

template <typename Container, typename Comparer>
ezUInt32 ezSorting::Partition(Container& container, ezUInt32 uiLeft, ezUInt32 uiRight, const Comparer& comparer)
{
  const ezUInt32 uiPivotIndex = (uiLeft + uiRight) / 2;
  
  ezMath::Swap(container[uiPivotIndex], container[uiRight]); // move pivot to right

  ezUInt32 uiIndex = uiLeft;
  for (ezUInt32 i = uiLeft; i < uiRight; ++i)
  {
    if (comparer.Less(container[i], container[uiRight]))
    {
      ezMath::Swap(container[i], container[uiIndex]);
      ++uiIndex;
    }
  }

  ezMath::Swap(container[uiIndex], container[uiRight]); // move pivot back in place

  return uiIndex;
}


template <typename T, typename Comparer>
void ezSorting::QuickSort(ezArrayPtr<T>& arrayPtr, ezUInt32 uiStartIndex, ezUInt32 uiEndIndex, const Comparer& comparer)
{
  if (uiStartIndex < uiEndIndex)
  {
    if (uiEndIndex - uiStartIndex <= INSERTION_THRESHOLD)
    {
      InsertionSort(arrayPtr, uiStartIndex, uiEndIndex, comparer);
    }
    else
    {
      ezUInt32 uiPivotIndex = Partition(arrayPtr, uiStartIndex, uiEndIndex, comparer);

      if (uiStartIndex < uiPivotIndex)
        QuickSort(arrayPtr, uiStartIndex, uiPivotIndex - 1, comparer);

      QuickSort(arrayPtr, uiPivotIndex + 1, uiEndIndex, comparer);
    }
  }
}

template <typename T, typename Comparer>
ezUInt32 ezSorting::Partition(ezArrayPtr<T>& arrayPtr, ezUInt32 uiLeft, ezUInt32 uiRight, const Comparer& comparer)
{
  const ezUInt32 uiPivotIndex = (uiLeft + uiRight) / 2;

  ezMath::Swap(arrayPtr[uiPivotIndex], arrayPtr[uiRight]); // move pivot to right

  ezUInt32 uiIndex = uiLeft;
  for (ezUInt32 i = uiLeft; i < uiRight; ++i)
  {
    if (comparer.Less(arrayPtr[i], arrayPtr[uiRight]))
    {
      ezMath::Swap(arrayPtr[i], arrayPtr[uiIndex]);
      ++uiIndex;
    }
  }

  ezMath::Swap(arrayPtr[uiIndex], arrayPtr[uiRight]); // move pivot back in place

  return uiIndex;
}


template <typename Container, typename Comparer>
void ezSorting::InsertionSort(Container& container, ezUInt32 uiStartIndex, ezUInt32 uiEndIndex, const Comparer& comparer)
{
  for (ezUInt32 i = uiStartIndex + 1; i <= uiEndIndex; ++i)
  { 
    ezUInt32 uiHoleIndex = i;
    while (uiHoleIndex > uiStartIndex && comparer.Less(container[uiHoleIndex], container[uiHoleIndex - 1]))
    {
      ezMath::Swap(container[uiHoleIndex], container[uiHoleIndex - 1]);
      --uiHoleIndex;
    }
  }
}

template <typename T, typename Comparer>
void ezSorting::InsertionSort(ezArrayPtr<T>& arrayPtr, ezUInt32 uiStartIndex, ezUInt32 uiEndIndex, const Comparer& comparer)
{
  T* ptr = arrayPtr.GetPtr();

  for (ezUInt32 i = uiStartIndex + 1; i <= uiEndIndex; ++i)
  { 
    ezUInt32 uiHoleIndex = i;
    T valueToInsert = std::move(ptr[uiHoleIndex]);

    while (uiHoleIndex > uiStartIndex && comparer.Less(valueToInsert, ptr[uiHoleIndex - 1]))
    {
      --uiHoleIndex;
    }

    const ezUInt32 uiMoveCount = i - uiHoleIndex;
    if (uiMoveCount > 0)
    {
      ezMemoryUtils::RelocateOverlapped(ptr + uiHoleIndex + 1, ptr + uiHoleIndex, uiMoveCount);
      ezMemoryUtils::MoveConstruct(ptr + uiHoleIndex, std::move(valueToInsert));
    }
    else
    {
      ptr[uiHoleIndex] = std::move(valueToInsert);
    }
  }
}

