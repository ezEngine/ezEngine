#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Types/Status.h>
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

  const char* GetSubTestName(ezInt32 iIdentifier) const;

  /// Override this function to add additional information to the test configuration
  virtual void UpdateConfiguration(ezTestConfiguration& ref_config) const /*override*/;

  /// \brief Implement this to add support for image comparisons. See EZ_TEST_IMAGE_MSG.
  virtual ezResult GetImage(ezImage& ref_img, const ezSubTestEntry& subTest, ezUInt32 uiImageNumber) { return EZ_FAILURE; }

  /// \brief Implement this to add support for depth buffer image comparisons. See EZ_TEST_DEPTH_IMAGE_MSG.
  virtual ezResult GetDepthImage(ezImage& ref_img, const ezSubTestEntry& subTest, ezUInt32 uiImageNumber) { return EZ_FAILURE; }

  /// \brief Used to map the 'number' for an image comparison, to a string used for finding the comparison image.
  ///
  /// By default image comparison screenshots are called 'TestName_SubTestName_XYZ'
  /// This can be fully overridden to use any other file name.
  /// The location of the comparison images (ie the folder) cannot be specified at the moment.
  virtual void MapImageNumberToString(const char* szTestName, const ezSubTestEntry& subTest, ezUInt32 uiImageNumber, ezStringBuilder& out_sString) const;

protected:
  /// Called at startup to determine if the test can be run. Should return a detailed error message on failure.
  virtual std::string IsTestAvailable() const { return {}; };
  /// Called at startup to setup all tests. Should use 'AddSubTest' to register all the sub-tests to the test framework.
  virtual void SetupSubTests() = 0;
  /// Called to run the test that was registered with the given identifier.
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) = 0;

  // *** Override these functions to implement optional (de-)initialization ***

  /// Called to initialize the whole test.
  virtual ezResult InitializeTest() { return EZ_SUCCESS; }
  /// Called to deinitialize the whole test.
  virtual ezResult DeInitializeTest() { return EZ_SUCCESS; }
  /// Called before running a sub-test to do additional initialization specifically for that test.
  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) { return EZ_SUCCESS; }
  /// Called after running a sub-test to do additional deinitialization specifically for that test.
  virtual ezResult DeInitializeSubTest(ezInt32 iIdentifier) { return EZ_SUCCESS; }


  /// Adds a sub-test to the test suite. The index is used to identify it when running the sub-tests.
  void AddSubTest(const char* szName, ezInt32 iIdentifier);

private:
  struct TestEntry
  {
    const char* m_szName = "";
    ezInt32 m_iIdentifier = -1;
  };

  /// Removes all sub-tests.
  void ClearSubTests();

  // Called by ezTestFramework.
  ezResult DoTestInitialization();
  void DoTestDeInitialization();
  ezResult DoSubTestInitialization(ezInt32 iIdentifier);
  void DoSubTestDeInitialization(ezInt32 iIdentifier);
  ezTestAppRun DoSubTestRun(ezInt32 iIdentifier, double& fDuration, ezUInt32 uiInvocationCount);

  // Finds internal entry index for identifier
  ezInt32 FindEntryForIdentifier(ezInt32 iIdentifier) const;

  std::deque<TestEntry> m_Entries;
};

#define EZ_CREATE_TEST(TestClass)
