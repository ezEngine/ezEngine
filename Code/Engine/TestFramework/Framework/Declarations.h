#pragma once

#include <TestFramework/Basics.h>
#include <deque>
#include <string>

class ezTestFramework;
class ezTestBaseClass;

/// \brief Stores the identification of a sub-test.
struct ezSubTestEntry
{
  ezSubTestEntry() : m_iSubTestIdentifier(-1), m_szSubTestName("") { }

  ezInt32 m_iSubTestIdentifier;
  const char* m_szSubTestName;
  bool m_bEnableTest;
};

/// \brief Stores the identification of a test.
struct ezTestEntry
{
  ezTestEntry() : m_pTest(NULL), m_szTestName("") { }

  ezTestBaseClass* m_pTest;
  const char* m_szTestName;
  std::deque<ezSubTestEntry> m_SubTests;
  bool m_bEnableTest;
};



