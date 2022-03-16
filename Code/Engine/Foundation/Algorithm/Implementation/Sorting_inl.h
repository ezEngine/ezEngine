
template <typename Container, typename Comparer>
void ezSorting::QuickSort(Container& container, const Comparer& comparer)
{
  if (container.IsEmpty())
    return;

  QuickSort(container, 0, container.GetCount() - 1, comparer);
}

template <typename T, typename Comparer>
void ezSorting::QuickSort(ezArrayPtr<T>& arrayPtr, const Comparer& comparer)
{
  if (arrayPtr.IsEmpty())
    return;

  QuickSort(arrayPtr, 0, arrayPtr.GetCount() - 1, comparer);
}

template <typename Container, typename Comparer>
void ezSorting::InsertionSort(Container& container, const Comparer& comparer)
{
  if (container.IsEmpty())
    return;

  InsertionSort(container, 0, container.GetCount() - 1, comparer);
}

template <typename T, typename Comparer>
void ezSorting::InsertionSort(ezArrayPtr<T>& arrayPtr, const Comparer& comparer)
{
  if (arrayPtr.IsEmpty())
    return;

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
      const ezUInt32 uiPivotIndex = Partition(container, uiStartIndex, uiEndIndex, comparer);

      ezUInt32 uiFirstHalfEndIndex = uiPivotIndex > 0 ? uiPivotIndex - 1 : 0;
      ezUInt32 uiSecondHalfStartIndex = uiPivotIndex + 1;

      while (uiFirstHalfEndIndex > uiStartIndex && !DoCompare(comparer, container[uiFirstHalfEndIndex], container[uiPivotIndex]))
      {
        uiFirstHalfEndIndex--;
      }

      while (uiSecondHalfStartIndex <= uiEndIndex && !DoCompare(comparer, container[uiPivotIndex], container[uiSecondHalfStartIndex]))
      {
        uiSecondHalfStartIndex++;
      }

      if (uiStartIndex < uiFirstHalfEndIndex)
        QuickSort(container, uiStartIndex, uiFirstHalfEndIndex, comparer);

      if (uiSecondHalfStartIndex < uiEndIndex)
        QuickSort(container, uiSecondHalfStartIndex, uiEndIndex, comparer);
    }
  }
}

template <typename Container, typename Comparer>
ezUInt32 ezSorting::Partition(Container& container, ezUInt32 uiLeft, ezUInt32 uiRight, const Comparer& comparer)
{
  ezUInt32 uiPivotIndex = (uiLeft + uiRight) / 2;

  if (DoCompare(comparer, container[uiLeft], container[uiRight]))
  {
    // left < right

    if (DoCompare(comparer, container[uiRight], container[uiPivotIndex]))
    {
      // left < right < pivot
      uiPivotIndex = uiRight;
    }
    else if (DoCompare(comparer, container[uiLeft], container[uiPivotIndex]))
    {
      // left < pivot < right
      uiPivotIndex = uiPivotIndex;
    }
    else
    {
      // pivot < left < right
      uiPivotIndex = uiLeft;
    }
  }
  else
  {
    // right < left

    if (DoCompare(comparer, container[uiLeft], container[uiPivotIndex]))
    {
      uiPivotIndex = uiLeft; // right < left < pivot
    }
    else if (DoCompare(comparer, container[uiRight], container[uiPivotIndex]))
    {
      // right < pivot < left
      uiPivotIndex = uiPivotIndex;
    }
    else
    {
      // pivot < right < left
      uiPivotIndex = uiRight;
    }
  }

  ezMath::Swap(container[uiPivotIndex], container[uiRight]); // move pivot to right

  ezUInt32 uiIndex = uiLeft;
  for (ezUInt32 i = uiLeft; i < uiRight; ++i)
  {
    if (DoCompare(comparer, container[i], container[uiRight]))
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
  T* ptr = arrayPtr.GetPtr();

  if (uiStartIndex < uiEndIndex)
  {
    if (uiEndIndex - uiStartIndex <= INSERTION_THRESHOLD)
    {
      InsertionSort(arrayPtr, uiStartIndex, uiEndIndex, comparer);
    }
    else
    {
      const ezUInt32 uiPivotIndex = Partition(ptr, uiStartIndex, uiEndIndex, comparer);

      ezUInt32 uiFirstHalfEndIndex = uiPivotIndex > 0 ? uiPivotIndex - 1 : 0;
      ezUInt32 uiSecondHalfStartIndex = uiPivotIndex + 1;

      while (uiFirstHalfEndIndex > uiStartIndex && !DoCompare(comparer, ptr[uiFirstHalfEndIndex], ptr[uiPivotIndex]))
      {
        uiFirstHalfEndIndex--;
      }

      while (uiSecondHalfStartIndex <= uiEndIndex && !DoCompare(comparer, ptr[uiPivotIndex], ptr[uiSecondHalfStartIndex]))
      {
        uiSecondHalfStartIndex++;
      }

      if (uiStartIndex < uiFirstHalfEndIndex)
        QuickSort(arrayPtr, uiStartIndex, uiFirstHalfEndIndex, comparer);

      if (uiSecondHalfStartIndex < uiEndIndex)
        QuickSort(arrayPtr, uiSecondHalfStartIndex, uiEndIndex, comparer);
    }
  }
}

template <typename T, typename Comparer>
ezUInt32 ezSorting::Partition(T* ptr, ezUInt32 uiLeft, ezUInt32 uiRight, const Comparer& comparer)
{
  ezUInt32 uiPivotIndex = (uiLeft + uiRight) / 2;

  if (DoCompare(comparer, ptr[uiLeft], ptr[uiRight]))
  {
    // left < right

    if (DoCompare(comparer, ptr[uiRight], ptr[uiPivotIndex]))
    {
      // left < right < pivot
      uiPivotIndex = uiRight;
    }
    else if (DoCompare(comparer, ptr[uiLeft], ptr[uiPivotIndex]))
    {
      // left < pivot < right
      uiPivotIndex = uiPivotIndex;
    }
    else
    {
      // pivot < left < right
      uiPivotIndex = uiLeft;
    }
  }
  else
  {
    // right < left

    if (DoCompare(comparer, ptr[uiLeft], ptr[uiPivotIndex]))
    {
      uiPivotIndex = uiLeft; // right < left < pivot
    }
    else if (DoCompare(comparer, ptr[uiRight], ptr[uiPivotIndex]))
    {
      // right < pivot < left
      uiPivotIndex = uiPivotIndex;
    }
    else
    {
      // pivot < right < left
      uiPivotIndex = uiRight;
    }
  }

  ezMath::Swap(ptr[uiPivotIndex], ptr[uiRight]); // move pivot to right

  ezUInt32 uiIndex = uiLeft;
  for (ezUInt32 i = uiLeft; i < uiRight; ++i)
  {
    if (DoCompare(comparer, ptr[i], ptr[uiRight]))
    {
      ezMath::Swap(ptr[i], ptr[uiIndex]);
      ++uiIndex;
    }
  }

  ezMath::Swap(ptr[uiIndex], ptr[uiRight]); // move pivot back in place

  return uiIndex;
}


template <typename Container, typename Comparer>
void ezSorting::InsertionSort(Container& container, ezUInt32 uiStartIndex, ezUInt32 uiEndIndex, const Comparer& comparer)
{
  for (ezUInt32 i = uiStartIndex + 1; i <= uiEndIndex; ++i)
  {
    ezUInt32 uiHoleIndex = i;
    while (uiHoleIndex > uiStartIndex && DoCompare(comparer, container[uiHoleIndex], container[uiHoleIndex - 1]))
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

    while (uiHoleIndex > uiStartIndex && DoCompare(comparer, valueToInsert, ptr[uiHoleIndex - 1]))
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
