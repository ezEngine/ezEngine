#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Utilities/EnumerableClass.h>
#include <TestFramework/Framework/Declarations.h>

struct ezTestConfiguration;
class ezImage;

class EZ_TEST_DLL ezTestBaseClass : public ezEnumerable<ezTestBaseClass>
{
  friend class ezTestFramework;

  EZ_DECLARE_ENUMERABLE_CLASS(ezTestBaseClass);

public:
  
  // *** Override these functions to implement the required test functionality ***

  /// Override this function to give the test a proper name.
  virtual const char* GetTestName() const /*override*/ = 0;

  /// Override this function to add additional information to the test configuration
  virtual void UpdateConfiguration(ezTestConfiguration& config) const /*override*/;

  virtual ezResult GetImage(ezImage& img) { return EZ_FAILURE; }

private:

  /// Called at startup to setup all tests. Should use 'AddSubTest' to register all the sub-tests to the test framework.
  virtual void SetupSubTests() /*override*/ = 0;
  /// Called to run the test that was registered with the given identifier.
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier) /*override*/ = 0;

  // *** Override these functions to implement optional (de-)initialization ***

  /// Called to initialize the whole test.
  virtual ezResult InitializeTest()                         /*override*/ { return EZ_SUCCESS; }
  /// Called to deinitialize the whole test.
  virtual ezResult DeInitializeTest()                       /*override*/ { return EZ_SUCCESS; }
  /// Called before running a sub-test to do additional initialization specifically for that test.
  virtual ezResult InitializeSubTest(ezInt32 iIdentifier)    /*override*/ { return EZ_SUCCESS; }
  /// Called after running a sub-test to do additional deinitialization specifically for that test.
  virtual ezResult DeInitializeSubTest(ezInt32 iIdentifier)  /*override*/ { return EZ_SUCCESS; }

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
  };

  /// Removes all sub-tests.
  void ClearSubTests();

  // Called by ezTestFramework.
  ezResult DoTestInitialization();
  void DoTestDeInitialization();
  ezResult DoSubTestInitialization(ezInt32 iIdentifier);
  void DoSubTestDeInitialization(ezInt32 iIdentifier);
  ezTestAppRun DoSubTestRun(ezInt32 iIdentifier, double& fDuration);


  std::deque<TestEntry> m_Entries;
};

#define EZ_CREATE_TEST(TestClass)

