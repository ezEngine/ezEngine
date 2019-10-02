#include <FoundationTestPCH.h>

#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Threading/TaskSystem.h>

namespace
{
  static constexpr ezUInt32 s_uiNumberOfWorkers = 4;
  static constexpr ezUInt32 s_uiTaskItemSliceSize = 25;
  static constexpr ezUInt32 s_uiTotalNumberOfTaskItems = s_uiNumberOfWorkers * s_uiTaskItemSliceSize;
} // namespace

EZ_CREATE_SIMPLE_TEST(Threading, ParallelFor)
{
  // set up controlled task system environment
  ezTaskSystem::SetWorkerThreadCount(::s_uiNumberOfWorkers, ::s_uiNumberOfWorkers);

  // shared variables
  ezMutex dataAccessMutex;

  ezUInt32 uiRangesEncounteredCheck = 0;
  ezUInt32 uiNumbersSum = 0;

  ezUInt32 uiNumbersCheckSum = 0;
  ezStaticArray<ezUInt32, ::s_uiTotalNumberOfTaskItems> numbers;

  ezTaskSystem::ParallelForParams parallelForParams;
  parallelForParams.uiBinSize = ::s_uiTaskItemSliceSize;
  parallelForParams.uiMaxTasksPerThread = 1;

  auto ResetSharedVariables = [&uiRangesEncounteredCheck, &uiNumbersSum, &uiNumbersCheckSum, &numbers]() {
    uiRangesEncounteredCheck = 0;
    uiNumbersSum = 0;

    uiNumbersCheckSum = 0;

    numbers.EnsureCount(::s_uiTotalNumberOfTaskItems);
    for (ezUInt32 i = 0; i < ::s_uiTotalNumberOfTaskItems; ++i)
    {
      numbers[i] = i + 1;
      uiNumbersCheckSum += numbers[i];
    }
  };

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Parallel For (Indexed)")
  {
    // reset
    ResetSharedVariables();

    // test
    // sum up the slice of number assigned to us via index ranges and
    // check if the ranges described by them are as expected
    ezTaskSystem::ParallelForIndexed(0, ::s_uiTotalNumberOfTaskItems,
      [&dataAccessMutex, &uiRangesEncounteredCheck, &uiNumbersSum, &numbers](ezUInt32 uiStartIndex, ezUInt32 uiEndIndex) {
        EZ_LOCK(dataAccessMutex);

        // size check
        EZ_TEST_INT(uiEndIndex - uiStartIndex, ::s_uiTaskItemSliceSize);

        // note down which range this is
        uiRangesEncounteredCheck |= 1 << (uiStartIndex / ::s_uiTaskItemSliceSize);

        // sum up numbers in our slice
        for (ezUInt32 uiIndex = uiStartIndex; uiIndex < uiEndIndex; ++uiIndex)
        {
          uiNumbersSum += numbers[uiIndex];
        }
      },
      "ParallelForIndexed Test", parallelForParams);

    // check results
    EZ_TEST_INT(uiRangesEncounteredCheck, 0b1111);
    EZ_TEST_INT(uiNumbersSum, uiNumbersCheckSum);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Parallel For (Array)")
  {
    // reset
    ResetSharedVariables();

    // test-specific data
    ezStaticArray<ezUInt32*, ::s_uiNumberOfWorkers> startAddresses;
    for (ezUInt32 i = 0; i < ::s_uiNumberOfWorkers; ++i)
    {
      startAddresses.PushBack(numbers.GetArrayPtr().GetPtr() + (i * ::s_uiTaskItemSliceSize));
    }

    // test
    // sum up the slice of numbers assigned to us via array pointers and
    // check if the ranges described by them are as expected
    ezTaskSystem::ParallelFor<ezUInt32>(numbers.GetArrayPtr(),
      [&dataAccessMutex, &uiRangesEncounteredCheck, &uiNumbersSum, &startAddresses](ezArrayPtr<ezUInt32> taskItemSlice) {
        EZ_LOCK(dataAccessMutex);

        // size check
        EZ_TEST_INT(taskItemSlice.GetCount(), ::s_uiTaskItemSliceSize);

        // note down which range this is
        for (ezUInt32 index = 0; index < startAddresses.GetCount(); ++index)
        {
          if (startAddresses[index] == taskItemSlice.GetPtr())
          {
            uiRangesEncounteredCheck |= 1 << index;
          }
        }

        // sum up numbers in our slice
        for (const ezUInt32& number : taskItemSlice)
        {
          uiNumbersSum += number;
        }
      },
      "ParallelFor Array Test", parallelForParams);

    // check results
    EZ_TEST_INT(15, 0b1111);
    EZ_TEST_INT(uiNumbersSum, uiNumbersCheckSum);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Parallel For (Array, Single)")
  {
    // reset
    ResetSharedVariables();

    // test
    // sum up the slice of numbers by summing up the individual numbers that get handed to us
    ezTaskSystem::ParallelForSingle(numbers.GetArrayPtr(),
      [&dataAccessMutex, &uiNumbersSum](ezUInt32 uiNumber) {
        EZ_LOCK(dataAccessMutex);
        uiNumbersSum += uiNumber;
      },
      "ParallelFor Array Single Test", parallelForParams);

    // check the resulting sum
    EZ_TEST_INT(uiNumbersSum, uiNumbersCheckSum);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Parallel For (Array, Single, Index)")
  {
    // reset
    ResetSharedVariables();

    // test
    // sum up the slice of numbers that got assigned to us via an index range
    ezTaskSystem::ParallelForSingleIndex(numbers.GetArrayPtr(),
      [&dataAccessMutex, &uiNumbersSum](ezUInt32 uiIndex, ezUInt32 uiNumber) {
        EZ_LOCK(dataAccessMutex);
        uiNumbersSum += uiNumber + (uiIndex + 1);
      },
      "ParallelFor Array Single Index Test", parallelForParams);

    // check the resulting sum
    EZ_TEST_INT(uiNumbersSum, 2 * uiNumbersCheckSum);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Parallel For (Array, Single) Write")
  {
    // reset
    ResetSharedVariables();

    // test
    // modify the original array of numbers
    ezTaskSystem::ParallelForSingle(numbers.GetArrayPtr(),
      [&dataAccessMutex](ezUInt32& uiNumber) {
        EZ_LOCK(dataAccessMutex);
        uiNumber = uiNumber * 3;
      },
      "ParallelFor Array Single Write Test (Write)", parallelForParams);

    // sum up the new values to test if writing worked
    ezTaskSystem::ParallelForSingle(numbers.GetArrayPtr(),
      [&dataAccessMutex, &uiNumbersSum](const ezUInt32& uiNumber) {
        EZ_LOCK(dataAccessMutex);
        uiNumbersSum += uiNumber;
      },
      "ParallelFor Array Single Write Test (Sum)", parallelForParams);

    // check the resulting sum
    EZ_TEST_INT(uiNumbersSum, 3 * uiNumbersCheckSum);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Parallel For (Array, Single, Index) Write")
  {
    // reset
    ResetSharedVariables();

    // test
    // modify the original array of numbers
    ezTaskSystem::ParallelForSingleIndex(numbers.GetArrayPtr(),
      [&dataAccessMutex](ezUInt32, ezUInt32& uiNumber) {
        EZ_LOCK(dataAccessMutex);
        uiNumber = uiNumber * 4;
      },
      "ParallelFor Array Single Write Test (Write)", parallelForParams);

    // sum up the new values to test if writing worked
    ezTaskSystem::ParallelForSingle(numbers.GetArrayPtr(),
      [&dataAccessMutex, &uiNumbersSum](const ezUInt32& uiNumber) {
        EZ_LOCK(dataAccessMutex);
        uiNumbersSum += uiNumber;
      },
      "ParallelFor Array Single Write Test (Sum)", parallelForParams);

    // check the resulting sum
    EZ_TEST_INT(uiNumbersSum, 4 * uiNumbersCheckSum);
  }
}
