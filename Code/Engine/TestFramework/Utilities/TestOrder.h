#pragma once

#include <TestFramework/Framework/TestFramework.h>

/// Sorts all tests and subtests alphabetically
void SortTestsAlphabetically(std::deque<ezTestEntry>& inout_Tests);

/// Writes the given test order to a simple config file
void SaveTestOrder(const char* szFile, const std::deque<ezTestEntry>& AllTests, TestSettings& testSettings);

/// Loads the order of tests from a file and sorts the existing tests in the array accordingly. The file and the array do not need to contain the same set of tests, the array is only re-arranged, nothing is added or removed from it.
void LoadTestOrder(const char* szFile, std::deque<ezTestEntry>& AllTests, TestSettings& testSettings);