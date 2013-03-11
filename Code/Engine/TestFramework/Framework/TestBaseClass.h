#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Utilities/EnumerableClass.h>
#include <TestFramework/Framework/Declarations.h>

class EZ_TEST_DLL ezTestBaseClass : public ezEnumerable<ezTestBaseClass>
{
  friend class ezTestFramework;

  EZ_DECLARE_ENUMERABLE_CLASS(ezTestBaseClass);

public:
  
  // *** Override these functions to implement the required test functionality ***

  /// Override this function to give the test a proper name.
  virtual const char* GetTestName() const /*EZ_OVERRIDE*/ = 0;

private:

  /// Called at startup to setup all tests. Should use 'AddSubTest' to register all the sub-tests to the test framework.
  virtual void SetupSubTests() /*EZ_OVERRIDE*/ = 0;
  /// Called to run the test that was registered with the given identifier.
  virtual void RunSubTest(ezInt32 iIdentifier) /*EZ_OVERRIDE*/ = 0;

  // *** Override these functions to implement optional (de-)initialization ***

  /// Called to initialize the whole test.
  virtual ezResult InitializeTest()                         /*EZ_OVERRIDE*/ { return EZ_SUCCESS; }
  /// Called to deinitialize the whole test.
  virtual ezResult DeInitializeTest()                       /*EZ_OVERRIDE*/ { return EZ_SUCCESS; }
  /// Called before running a sub-test to do additional initialization specifically for that test.
  virtual ezResult InitializeSubTest(ezInt32 iIdentifier)    /*EZ_OVERRIDE*/ { return EZ_SUCCESS; }
  /// Called after running a sub-test to do additional deinitialization specifically for that test.
  virtual ezResult DeInitializeSubTest(ezInt32 iIdentifier)  /*EZ_OVERRIDE*/ { return EZ_SUCCESS; }

protected:
  /// Adds a sub-test to the test suite. The index is used to identify it when running the sub-tests.
  void AddSubTest (const char* szName, ezInt32 iIdentifier);

private:

  struct TestEntry
  {
    TestEntry()
    {
      m_szName = "";
      m_iIdentifier = -1;
    }

    const char* m_szName;
    ezInt32 m_iIdentifier;
    double m_fDuration;
  };

  /// Removes all sub-tests.
  void ClearSubTests();

  void SetSubTestDuration(ezInt32 iIndex, double fDuration);

  /// Called by ezTestFramework. Runs all sub-tests.
  ezResult ExecuteTest(std::deque<ezSubTestEntry>& inout_SubTestsToExecute, double& out_fTotalDuration);

  ezResult DoTestInitialization();
  void DoTestDeInitialization();
  ezResult DoSubTestInitialization(ezInt32 iIdentifier);
  void DoSubTestDeInitialization(ezInt32 iIdentifier);
  double DoSubTestRun(ezInt32 iIdentifier, const char* szSubTestName);


  std::deque<TestEntry> m_Entries;
};

#define EZ_CREATE_TEST(TestClass)
