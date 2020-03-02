#pragma once

#include <TestFramework/Framework/TestFramework.h>

/// Sorts all tests and subtests alphabetically
void SortTestsAlphabetically(std::deque<ezTestEntry>& inout_Tests);

/// Writes the given test order to a simple config file
void SaveTestOrder(const char* szFile, const std::deque<ezTestEntry>& AllTests);

/// Loads the order of tests from a file and sorts the existing tests in the array accordingly. The file and the array do not need to
/// contain the same set of tests, the array is only re-arranged, nothing is added or removed from it.
void LoadTestOrder(const char* szFile, std::deque<ezTestEntry>& AllTests);

/// Writes the given test settings to a simple config file
void SaveTestSettings(const char* szFile, TestSettings& testSettings);

/// Loads the test settings from a file
void LoadTestSettings(const char* szFile, TestSettings& testSettings);



