
template <typename Comparer>
template <typename Container>
void ezSorting<Comparer>::QuickSort(Container& container)
{
  QuickSort(container, 0, container.GetCount() - 1);
}

template <typename Comparer>
template <typename T>
void ezSorting<Comparer>::QuickSort(ezArrayPtr<T>& arrayPtr)
{
  QuickSort(arrayPtr, 0, arrayPtr.GetCount() - 1);
}

template <typename Comparer>
template <typename Container>
void ezSorting<Comparer>::InsertionSort(Container& container)
{
  InsertionSort(container, 0, container.GetCount() - 1);
}

template <typename Comparer>
template <typename T>
void ezSorting<Comparer>::InsertionSort(ezArrayPtr<T>& arrayPtr)
{
  InsertionSort(arrayPtr, 0, arrayPtr.GetCount() - 1);
}


template <typename Comparer>
template <typename Container>
void ezSorting<Comparer>::QuickSort(Container& container, ezUInt32 uiStartIndex, ezUInt32 uiEndIndex)
{
  if (uiStartIndex < uiEndIndex)
  {
    if (uiEndIndex - uiStartIndex <= INSERTION_THRESHOLD)
    {
      InsertionSort(container, uiStartIndex, uiEndIndex);
    }
    else
    {
      ezUInt32 uiPivotIndex = Partition(container, uiStartIndex, uiEndIndex);
      
      if (uiStartIndex < uiPivotIndex)
        QuickSort(container, uiStartIndex, uiPivotIndex - 1);

      QuickSort(container, uiPivotIndex + 1, uiEndIndex);
    }
  }
}

template <typename Comparer>
template <typename Container>
ezUInt32 ezSorting<Comparer>::Partition(Container& container, ezUInt32 uiLeft, ezUInt32 uiRight)
{
  const ezUInt32 uiPivotIndex = (uiLeft + uiRight) / 2;
  
  ezMath::Swap(container[uiPivotIndex], container[uiRight]); // move pivot to right

  ezUInt32 uiIndex = uiLeft;
  for (ezUInt32 i = uiLeft; i < uiRight; ++i)
  {
    if (Comparer::Less(container[i], container[uiRight]))
    {
      ezMath::Swap(container[i], container[uiIndex]);
      ++uiIndex;
    }
  }

  ezMath::Swap(container[uiIndex], container[uiRight]); // move pivot back in place

  return uiIndex;
}


template <typename Comparer>
template <typename T>
void ezSorting<Comparer>::QuickSort(ezArrayPtr<T>& arrayPtr, ezUInt32 uiStartIndex, ezUInt32 uiEndIndex)
{
  if (uiStartIndex < uiEndIndex)
  {
    if (uiEndIndex - uiStartIndex <= INSERTION_THRESHOLD)
    {
      InsertionSort(arrayPtr, uiStartIndex, uiEndIndex);
    }
    else
    {
      ezUInt32 uiPivotIndex = Partition(arrayPtr, uiStartIndex, uiEndIndex);

      if (uiStartIndex < uiPivotIndex)
        QuickSort(arrayPtr, uiStartIndex, uiPivotIndex - 1);

      QuickSort(arrayPtr, uiPivotIndex + 1, uiEndIndex);
    }
  }
}

template <typename Comparer>
template <typename T>
ezUInt32 ezSorting<Comparer>::Partition(ezArrayPtr<T>& arrayPtr, ezUInt32 uiLeft, ezUInt32 uiRight)
{
  const ezUInt32 uiPivotIndex = (uiLeft + uiRight) / 2;
  
  T pivotValue = arrayPtr[uiPivotIndex];
  ezMath::Swap(arrayPtr[uiPivotIndex], arrayPtr[uiRight]); // move pivot to right

  ezUInt32 uiIndex = uiLeft;
  for (ezUInt32 i = uiLeft; i < uiRight; ++i)
  {
    if (Comparer::Less(arrayPtr[i], pivotValue))
    {
      ezMath::Swap(arrayPtr[i], arrayPtr[uiIndex]);
      ++uiIndex;
    }
  }

  ezMath::Swap(arrayPtr[uiIndex], arrayPtr[uiRight]); // move pivot back in place

  return uiIndex;
}


template <typename Comparer>
template <typename Container>
void ezSorting<Comparer>::InsertionSort(Container& container, ezUInt32 uiStartIndex, ezUInt32 uiEndIndex)
{
  for (ezUInt32 i = uiStartIndex + 1; i <= uiEndIndex; ++i)
  { 
    ezUInt32 uiHoleIndex = i;
    while (uiHoleIndex > uiStartIndex && Comparer::Less(container[uiHoleIndex], container[uiHoleIndex - 1]))
    {
      ezMath::Swap(container[uiHoleIndex], container[uiHoleIndex - 1]);
      --uiHoleIndex;
    }
  }
}

template <typename Comparer>
template <typename T>
void ezSorting<Comparer>::InsertionSort(ezArrayPtr<T>& arrayPtr, ezUInt32 uiStartIndex, ezUInt32 uiEndIndex)
{
  for (ezUInt32 i = uiStartIndex + 1; i <= uiEndIndex; ++i)
  { 
    ezUInt32 uiHoleIndex = i;
    T valueToInsert = arrayPtr[uiHoleIndex];

    while (uiHoleIndex > uiStartIndex && Comparer::Less(valueToInsert, arrayPtr[uiHoleIndex - 1]))
    {
      --uiHoleIndex;
    }

    const ezUInt32 uiMoveCount = i - uiHoleIndex;
    if (uiMoveCount > 0)
    {
      ezMemoryUtils::Move(&arrayPtr[uiHoleIndex + 1], &arrayPtr[uiHoleIndex], uiMoveCount);
      arrayPtr[uiHoleIndex] = valueToInsert;
    }
  }
}
