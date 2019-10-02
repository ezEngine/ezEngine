#pragma once

#include <TestFramework/Framework/Declarations.h>
#include <TestFramework/Framework/SimpleTest.h>
#include <TestFramework/Framework/TestBaseClass.h>
#include <TestFramework/Framework/TestResults.h>
#include <TestFramework/TestFrameworkDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/String.h>

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <utility>

// Disable C++/CX adds.
#pragma warning(disable : 4447)

class EZ_TEST_DLL ezTestFramework
{
public:
  ezTestFramework(const char* szTestName, const char* szAbsTestOutputDir, const char* szRelTestDataDir, int argc, const char** argv);
  virtual ~ezTestFramework();

  typedef void (*OutputHandler)(ezTestOutput::Enum Type, const char* szMsg);

  // Test management
  void CreateOutputFolder();
  const char* GetTestName() const;
  const char* GetAbsOutputPath() const;
  const char* GetRelTestDataPath() const;
  const char* GetAbsTestSettingsFilePath() const;
  void RegisterOutputHandler(OutputHandler Handler);
  void GatherAllTests();
  void LoadTestOrder();
  void AutoSaveTestOrder();
  void SaveTestOrder(const char* const filePath);
  void SetAllTestsEnabledStatus(bool bEnable);
  void SetAllFailedTestsEnabledStatus();
  // Each function on a test must not take longer than the given time or the test process will be terminated.
  void SetTestTimeout(ezUInt32 testTimeoutMS);
  void GetTestSettingsFromCommandLine(int argc, const char** argv);

  // Test execution
  void ResetTests();
  ezTestAppRun RunTestExecutionLoop();

  void StartTests();
  void ExecuteNextTest();
  void EndTests();
  void AbortTests();

  // Test queries
  ezUInt32 GetTestCount() const;
  ezUInt32 GetTestEnabledCount() const;
  ezUInt32 GetSubTestEnabledCount(ezUInt32 uiTestIndex) const;
  const std::string& IsTestAvailable(ezUInt32 uiTestIndex) const;
  bool IsTestEnabled(ezUInt32 uiTestIndex) const;
  bool IsSubTestEnabled(ezUInt32 uiTestIndex, ezUInt32 uiSubTestIndex) const;
  void SetTestEnabled(ezUInt32 uiTestIndex, bool bEnabled);
  void SetSubTestEnabled(ezUInt32 uiTestIndex, ezUInt32 uiSubTestIndex, bool bEnabled);

  ezInt32 GetCurrentTestIndex() { return m_iCurrentTestIndex; }
  ezInt32 GetCurrentSubTestIndex() { return m_iCurrentSubTestIndex; }
  ezTestEntry* GetTest(ezUInt32 uiTestIndex);
  bool GetTestsRunning() const { return m_bTestsRunning; }

  // Global settings
  TestSettings GetSettings() const;
  void SetSettings(const TestSettings& settings);

  // Test results
  ezTestFrameworkResult& GetTestResult();
  ezInt32 GetTotalErrorCount() const;
  ezInt32 GetTestsPassedCount() const;
  ezInt32 GetTestsFailedCount() const;
  double GetTotalTestDuration() const;

  // Image comparison
  void ScheduleImageComparison(ezUInt32 uiImageNumber, ezUInt32 uiMaxError);
  bool IsImageComparisonScheduled() const { return m_bImageComparisonScheduled; }
  void GenerateComparisonImageName(ezUInt32 uiImageNumber, ezStringBuilder& sImgName);
  void GetCurrentComparisonImageName(ezStringBuilder& sImgName);
  void SetImageReferenceFolderName(const char* szFolderName);
  void SetImageReferenceOverrideFolderName(const char* szFolderName);

  /// \brief Writes an Html file that contains test information and an image diff view for failed image comparisons.
  void WriteImageDiffHtml(const char* fileName, ezImage& referenceImgRgb, ezImage& referenceImgAlpha, ezImage& capturedImgRgb,
    ezImage& capturedImgAlpha, ezImage& diffImgRgb, ezImage& diffImgAlpha, ezUInt32 uiError, ezUInt32 uiThreshold, ezUInt8 uiMinDiffRgb,
    ezUInt8 uiMaxDiffRgb, ezUInt8 uiMinDiffAlpha, ezUInt8 uiMaxDiffAlpha);

  bool PerformImageComparison(ezStringBuilder sImgName, const ezImage& img, ezUInt32 uiMaxError, char* szErrorMsg);
  bool CompareImages(ezUInt32 uiImageNumber, ezUInt32 uiMaxError, char* szErrorMsg);

  /// \brief A function to be called to add extra info to image diff output, that is not available from here.
  /// E.g. device specific info like driver version.
  typedef std::function<ezDynamicArray<std::pair<ezString, ezString>>()> ImageDiffExtraInfoCallback;
  void SetImageDiffExtraInfoCallback(ImageDiffExtraInfoCallback provider);

  typedef std::function<void(bool)> ImageComparisonCallback; /// \brief A function to be called after every image comparison with a bool
                                                             /// indicating if the images matched or not.
  void SetImageComparisonCallback(const ImageComparisonCallback& callback);

  static ezResult CaptureRegressionStat(ezStringView testName, ezStringView name, ezStringView unit, float value, ezInt32 testId = -1);

protected:
  void Initialize();
  void DeInitialize();

  /// \brief Will be called for test failures to record the location of the failure and forward the error to OutputImpl.
  virtual void ErrorImpl(const char* szError, const char* szFile, ezInt32 iLine, const char* szFunction, const char* szMsg);
  /// \brief Receives ezLog messages (via LogWriter) as well as test-framework internal logging. Any ezTestOutput::Error will
  /// cause the test to fail.
  virtual void OutputImpl(ezTestOutput::Enum Type, const char* szMsg);
  virtual void TestResultImpl(ezInt32 iSubTestIndex, bool bSuccess, double fDuration);
  void FlushAsserts();
  void TimeoutThread();
  void UpdateTestTimeout();

  // ignore this for now
public:
  static const char* s_szTestBlockName;
  static int s_iAssertCounter;
  static bool s_bCallstackOnAssert;
  static ezLog::TimestampMode s_LogTimestampMode;

  // static functions
public:
  static EZ_ALWAYS_INLINE ezTestFramework* GetInstance() { return s_pInstance; }

  /// \brief Returns whether to asset on test fail, will return false if no debugger is attached to prevent crashing.
  static bool GetAssertOnTestFail();

  static void Output(ezTestOutput::Enum Type, const char* szMsg, ...);
  static void OutputArgs(ezTestOutput::Enum Type, const char* szMsg, va_list args);
  static void Error(const char* szError, const char* szFile, ezInt32 iLine, const char* szFunction, const char* szMsg, ...);
  static void Error(const char* szError, const char* szFile, ezInt32 iLine, const char* szFunction, const char* szMsg, va_list args);
  static void TestResult(ezInt32 iSubTestIndex, bool bSuccess, double fDuration);

  // static members
private:
  static ezTestFramework* s_pInstance;

private:
  std::string m_sTestName;                ///< The name of the tests being done
  std::string m_sAbsTestOutputDir;        ///< Absolute path to the output folder where results and temp data is stored
  std::string m_sRelTestDataDir;          ///< Relative path from the SDK to where the unit test data is located
  std::string m_sAbsTestSettingsFilePath; ///< Absolute path to the test settings file
  ezInt32 m_iErrorCount = 0;
  ezInt32 m_iTestsFailed = 0;
  ezInt32 m_iTestsPassed = 0;
  TestSettings m_Settings;
  std::recursive_mutex m_outputMutex;
  std::deque<OutputHandler> m_OutputHandlers;
  std::deque<ezTestEntry> m_TestEntries;
  ezTestFrameworkResult m_Result;
  ezAssertHandler m_PreviousAssertHandler = nullptr;
  ImageDiffExtraInfoCallback m_ImageDiffExtraInfoCallback;
  ImageComparisonCallback m_ImageComparisonCallback;

  std::mutex m_timeoutLock;
  ezUInt32 m_timeoutMS = 5 * 60 * 1000; // 5 min default timeout
  bool m_useTimeout = false;
  bool m_reArm = false;
  std::condition_variable m_timeoutCV;
  std::thread m_timeoutThread;

  ezInt32 m_iExecutingTest = 0;
  ezInt32 m_iExecutingSubTest = 0;
  bool m_bSubTestInitialized = false;
  bool m_bAbortTests = false;
  ezUInt8 m_uiPassesLeft = 0;
  double m_fTotalTestDuration = 0.0;
  double m_fTotalSubTestDuration = 0.0;
  ezInt32 m_iErrorCountBeforeTest = 0;
  ezUInt32 m_uiSubTestInvocationCount = 0;

  bool m_bIsInitialized = false;

  // image comparisons
  bool m_bImageComparisonScheduled = false;
  ezUInt32 m_uiMaxImageComparisonError = 0;
  ezUInt32 m_uiComparisonImageNumber = 0;
  std::string m_sImageReferenceFolderName = "Images_Reference";
  std::string m_sImageReferenceOverrideFolderName;

protected:
  ezInt32 m_iCurrentTestIndex = -1;
  ezInt32 m_iCurrentSubTestIndex = -1;
  bool m_bTestsRunning = false;
};

#ifdef EZ_NV_OPTIMUS
#  undef EZ_NV_OPTIMUS
#endif

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Win/MinWindows.h>
#  define EZ_NV_OPTIMUS                                                          \
    extern "C"                                                                   \
    {                                                                            \
      _declspec(dllexport) ezMinWindows::DWORD NvOptimusEnablement = 0x00000001; \
    }
#else
#  define EZ_NV_OPTIMUS
#endif

#if EZ_ENABLED(EZ_PLATFORM_ANDROID)
#  include <Foundation/Basics/Platform/Android/AndroidUtils.h>
#  include <android/log.h>
#  include <android/native_activity.h>
#  include <android_native_app_glue.h>


#  define EZ_TESTFRAMEWORK_ENTRY_POINT_BEGIN(szTestName, szNiceTestName)                                                   \
    int ezAndroidMain(int argc, char** argv);                                                                              \
    extern "C" void android_main(struct android_app* app)                                                                  \
    {                                                                                                                      \
      ezAndroidUtils::SetAndroidApp(app);                                                                                  \
      /* TODO: do something with the return value of ezAndroidMain?  */                                                    \
      /* TODO: can we get somehow get the command line arguments to the android app? Is there even something like that? */ \
      int iReturnCode = ezAndroidMain(0, nullptr);                                                                         \
      __android_log_print(ANDROID_LOG_ERROR, "ezEngine", "Test framework exited with return code: '%d'", iReturnCode);     \
    }                                                                                                                      \
                                                                                                                           \
    int ezAndroidMain(int argc, char** argv)                                                                               \
    {                                                                                                                      \
      ezTestSetup::InitTestFramework(szTestName, szNiceTestName, 0, nullptr);                                              \
      /* Execute custom init code here by using the BEGIN/END macros directly */

#else
/// \brief Macro to define the application entry point for all test applications
#  define EZ_TESTFRAMEWORK_ENTRY_POINT_BEGIN(szTestName, szNiceTestName)                    \
    /* Enables that on machines with multiple GPUs the NVIDIA GPU is preferred */           \
    EZ_NV_OPTIMUS                                                                           \
    int main(int argc, char** argv)                                                         \
    {                                                                                       \
      ezTestSetup::InitTestFramework(szTestName, szNiceTestName, argc, (const char**)argv); \
      /* Execute custom init code here by using the BEGIN/END macros directly */

#endif

#if EZ_ENABLED(EZ_PLATFORM_ANDROID)
#  define EZ_TESTFRAMEWORK_ENTRY_POINT_END()                                       \
    /* TODO: This is too big for a macro now */                                    \
    auto app = ezAndroidUtils::GetAndroidApp();                                    \
    bool bRun = true;                                                              \
    while (true)                                                                   \
    {                                                                              \
      struct android_poll_source* source = nullptr;                                \
      int ident = 0;                                                               \
      int events = 0;                                                              \
      while ((ident = ALooper_pollAll(0, nullptr, &events, (void**)&source)) >= 0) \
      {                                                                            \
        if (source != nullptr)                                                     \
          source->process(app, source);                                            \
      }                                                                            \
      if (bRun && ezTestSetup::RunTests() != ezTestAppRun::Continue)               \
      {                                                                            \
        bRun = false;                                                              \
        ANativeActivity_finish(app->activity);                                     \
      }                                                                            \
      if (app->destroyRequested)                                                   \
      {                                                                            \
        const ezInt32 iFailedTests = ezTestSetup::GetFailedTestCount();            \
        ezTestSetup::DeInitTestFramework();                                        \
        return iFailedTests;                                                       \
      }                                                                            \
    }                                                                              \
    } 

#else
#  define EZ_TESTFRAMEWORK_ENTRY_POINT_END()                        \
    while (ezTestSetup::RunTests() == ezTestAppRun::Continue)       \
    {                                                               \
    }                                                               \
    const ezInt32 iFailedTests = ezTestSetup::GetFailedTestCount(); \
    ezTestSetup::DeInitTestFramework();                             \
    return iFailedTests;                                            \
    }

#endif


#define EZ_TESTFRAMEWORK_ENTRY_POINT(szTestName, szNiceTestName)             \
  EZ_TESTFRAMEWORK_ENTRY_POINT_BEGIN(szTestName, szNiceTestName)             \
  /* Execute custom init code here by using the BEGIN/END macros directly */ \
  EZ_TESTFRAMEWORK_ENTRY_POINT_END()

/// \brief Enum for usage in EZ_TEST_BLOCK to enable or disable the block.
struct ezTestBlock
{
  /// \brief Enum for usage in EZ_TEST_BLOCK to enable or disable the block.
  enum Enum
  {
    Enabled,           ///< The test block is enabled.
    Disabled,          ///< The test block will be skipped. The test framework will print a warning message, that some block is deactivated.
    DisabledNoWarning, ///< The test block will be skipped, but no warning printed. Used to deactivate 'on demand/optional' tests.
  };
};

#define safeprintf ezStringUtils::snprintf

/// \brief Starts a small test block inside a larger test.
///
/// First parameter allows to quickly disable a block depending on a condition (e.g. platform).
/// Second parameter just gives it a name for better error reporting.
/// Also skipped tests are highlighted in the output, such that people can quickly see when a test is currently deactivated.
#define EZ_TEST_BLOCK(enable, name)                                                  \
  ezTestFramework::s_szTestBlockName = name;                                         \
  if (enable == ezTestBlock::Disabled)                                               \
  {                                                                                  \
    ezTestFramework::s_szTestBlockName = "";                                         \
    ezTestFramework::Output(ezTestOutput::Warning, "Skipped Test Block '%s'", name); \
  }                                                                                  \
  else if (enable == ezTestBlock::DisabledNoWarning)                                 \
  {                                                                                  \
    ezTestFramework::s_szTestBlockName = "";                                         \
  }                                                                                  \
  else


/// \brief Will trigger a debug break, if the test framework is configured to do so on test failure
#define EZ_TEST_DEBUG_BREAK                   \
  if (ezTestFramework::GetAssertOnTestFail()) \
  EZ_DEBUG_BREAK

#define EZ_TEST_FAILURE(erroroutput, msg, ...)                                                                   \
  {                                                                                                              \
    ezTestFramework::Error(erroroutput, EZ_SOURCE_FILE, EZ_SOURCE_LINE, EZ_SOURCE_FUNCTION, msg, ##__VA_ARGS__); \
    EZ_TEST_DEBUG_BREAK                                                                                          \
  }

//////////////////////////////////////////////////////////////////////////

EZ_TEST_DLL ezResult ezTestBool(
  bool bCondition, const char* szErrorText, const char* szFile, ezInt32 iLine, const char* szFunction, const char* szMsg, ...);

/// \brief Tests for a boolean condition, does not output an extra message.
#define EZ_TEST_BOOL(condition) EZ_TEST_BOOL_MSG(condition, "")

/// \brief Tests for a boolean condition, outputs a custom message on failure.
#define EZ_TEST_BOOL_MSG(condition, msg, ...) \
  ezTestBool(condition, "Test failed: " EZ_STRINGIZE(condition), EZ_SOURCE_FILE, EZ_SOURCE_LINE, EZ_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

inline double ToFloat(int f)
{
  return static_cast<double>(f);
}

inline double ToFloat(float f)
{
  return static_cast<double>(f);
}

inline double ToFloat(double f)
{
  return static_cast<double>(f);
}

EZ_TEST_DLL ezResult ezTestDouble(double f1, double f2, double fEps, const char* szF1, const char* szF2, const char* szFile, ezInt32 iLine,
  const char* szFunction, const char* szMsg, ...);

/// \brief Tests two floats for equality, within a given epsilon. On failure both actual and expected values are output.
#define EZ_TEST_FLOAT(f1, f2, epsilon) EZ_TEST_FLOAT_MSG(f1, f2, epsilon, "")

/// \brief Tests two floats for equality, within a given epsilon. On failure both actual and expected values are output, also a custom
/// message is printed.
#define EZ_TEST_FLOAT_MSG(f1, f2, epsilon, msg, ...)                                                                           \
  ezTestDouble(ToFloat(f1), ToFloat(f2), ToFloat(epsilon), EZ_STRINGIZE(f1), EZ_STRINGIZE(f2), EZ_SOURCE_FILE, EZ_SOURCE_LINE, \
    EZ_SOURCE_FUNCTION, msg, ##__VA_ARGS__)


//////////////////////////////////////////////////////////////////////////

/// \brief Tests two doubles for equality, within a given epsilon. On failure both actual and expected values are output.
#define EZ_TEST_DOUBLE(f1, f2, epsilon) EZ_TEST_DOUBLE_MSG(f1, f2, epsilon, "")

/// \brief Tests two doubles for equality, within a given epsilon. On failure both actual and expected values are output, also a custom
/// message is printed.
#define EZ_TEST_DOUBLE_MSG(f1, f2, epsilon, msg, ...)                                                                          \
  ezTestDouble(ToFloat(f1), ToFloat(f2), ToFloat(epsilon), EZ_STRINGIZE(f1), EZ_STRINGIZE(f2), EZ_SOURCE_FILE, EZ_SOURCE_LINE, \
    EZ_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

EZ_TEST_DLL ezResult ezTestInt(ezInt64 i1, ezInt64 i2, const char* szI1, const char* szI2, const char* szFile, ezInt32 iLine,
  const char* szFunction, const char* szMsg, ...);

/// \brief Tests two ints for equality. On failure both actual and expected values are output.
#define EZ_TEST_INT(i1, i2) EZ_TEST_INT_MSG(i1, i2, "")

/// \brief Tests two ints for equality. On failure both actual and expected values are output, also a custom message is printed.
#define EZ_TEST_INT_MSG(i1, i2, msg, ...) \
  ezTestInt(i1, i2, EZ_STRINGIZE(i1), EZ_STRINGIZE(i2), EZ_SOURCE_FILE, EZ_SOURCE_LINE, EZ_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

EZ_TEST_DLL ezResult ezTestString(std::string s1, std::string s2, const char* szString1, const char* szString2, const char* szFile,
  ezInt32 iLine, const char* szFunction, const char* szMsg, ...);

/// \brief Tests two strings for equality. On failure both actual and expected values are output.
#define EZ_TEST_STRING(i1, i2) EZ_TEST_STRING_MSG(i1, i2, "")

/// \brief Tests two strings for equality. On failure both actual and expected values are output, also a custom message is printed.
#define EZ_TEST_STRING_MSG(s1, s2, msg, ...)                                                                                   \
  ezTestString(static_cast<const char*>(s1), static_cast<const char*>(s2), EZ_STRINGIZE(s1), EZ_STRINGIZE(s2), EZ_SOURCE_FILE, \
    EZ_SOURCE_LINE, EZ_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

/// \brief Tests two strings for equality. On failure both actual and expected values are output. Does not embed the original expression to
/// work around issues with the current code page and unicode literals.
#define EZ_TEST_STRING_UNICODE(i1, i2) EZ_TEST_STRING_UNICODE_MSG(i1, i2, "")

/// \brief Tests two strings for equality. On failure both actual and expected values are output, also a custom message is printed. Does not
/// embed the original expression to work around issues with the current code page and unicode literals.
#define EZ_TEST_STRING_UNICODE_MSG(s1, s2, msg, ...)                                                                                   \
  ezTestString(static_cast<const char*>(s1), static_cast<const char*>(s2), "", "", EZ_SOURCE_FILE, EZ_SOURCE_LINE, EZ_SOURCE_FUNCTION, \
    msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

EZ_TEST_DLL ezResult ezTestVector(ezVec4d v1, ezVec4d v2, double fEps, const char* szCondition, const char* szFile, ezInt32 iLine,
  const char* szFunction, const char* szMsg, ...);

/// \brief Tests two ezVec2's for equality, using some epsilon. On failure both actual and expected values are output.
#define EZ_TEST_VEC2(i1, i2, epsilon) EZ_TEST_VEC2_MSG(i1, i2, epsilon, "")

/// \brief Tests two ezVec2's for equality. On failure both actual and expected values are output, also a custom message is printed.
#define EZ_TEST_VEC2_MSG(r1, r2, epsilon, msg, ...)                                                                                \
  ezTestVector(ezVec4d(ToFloat((r1).x), ToFloat((r1).y), 0, 0), ezVec4d(ToFloat((r2).x), ToFloat((r2).y), 0, 0), ToFloat(epsilon), \
    EZ_STRINGIZE(r1) " == " EZ_STRINGIZE(r2), EZ_SOURCE_FILE, EZ_SOURCE_LINE, EZ_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

/// \brief Tests two ezVec3's for equality, using some epsilon. On failure both actual and expected values are output.
#define EZ_TEST_VEC3(i1, i2, epsilon) EZ_TEST_VEC3_MSG(i1, i2, epsilon, "")

/// \brief Tests two ezVec3's for equality. On failure both actual and expected values are output, also a custom message is printed.
#define EZ_TEST_VEC3_MSG(r1, r2, epsilon, msg, ...)                                                                            \
  ezTestVector(ezVec4d(ToFloat((r1).x), ToFloat((r1).y), ToFloat((r1).z), 0),                                                  \
    ezVec4d(ToFloat((r2).x), ToFloat((r2).y), ToFloat((r2).z), 0), ToFloat(epsilon), EZ_STRINGIZE(r1) " == " EZ_STRINGIZE(r2), \
    EZ_SOURCE_FILE, EZ_SOURCE_LINE, EZ_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

/// \brief Tests two ezVec4's for equality, using some epsilon. On failure both actual and expected values are output.
#define EZ_TEST_VEC4(i1, i2, epsilon) EZ_TEST_VEC4_MSG(i1, i2, epsilon, "")

/// \brief Tests two ezVec4's for equality. On failure both actual and expected values are output, also a custom message is printed.
#define EZ_TEST_VEC4_MSG(r1, r2, epsilon, msg, ...)                                                \
  ezTestVector(ezVec4d(ToFloat((r1).x), ToFloat((r1).y), ToFloat((r1).z), ToFloat((r1).w)),        \
    ezVec4d(ToFloat((r2).x), ToFloat((r2).y), ToFloat((r2).z), ToFloat((r2).w)), ToFloat(epsilon), \
    EZ_STRINGIZE(r1) " == " EZ_STRINGIZE(r2), EZ_SOURCE_FILE, EZ_SOURCE_LINE, EZ_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

EZ_TEST_DLL ezResult ezTestFiles(
  const char* szFile1, const char* szFile2, const char* szFile, ezInt32 iLine, const char* szFunction, const char* szMsg, ...);

#define EZ_TEST_FILES(szFile1, szFile2, msg, ...) \
  ezTestFiles(szFile1, szFile2, EZ_SOURCE_FILE, EZ_SOURCE_LINE, EZ_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

EZ_TEST_DLL ezResult ezTestImage(
  ezUInt32 uiImageNumber, ezUInt32 uiMaxError, const char* szFile, ezInt32 iLine, const char* szFunction, const char* szMsg, ...);

/// \brief Same as EZ_TEST_IMAGE_MSG but uses an empty error message.
#define EZ_TEST_IMAGE(ImageNumber, MaxError) EZ_TEST_IMAGE_MSG(ImageNumber, MaxError, "")

/// \brief Executes an image comparison right now.
///
/// The reference image is read from disk.
/// The path to the reference image is constructed from the test and sub-test name and the 'ImageNumber'.
/// One can, for instance, use the 'invocation count' that is passed to ezTestBaseClass::RunSubTest() as the ImageNumber,
/// but any other integer is fine as well.
///
/// The current image to compare is taken from ezTestBaseClass::GetImage().
/// Rendering tests typically override this function to return the result of the currently rendered frame.
///
/// 'MaxError' specifies the maximum mean-square error that is still considered acceptable
/// between the reference image and the current image.
///
/// \note Some tests need to know at the start, whether an image comparison will be done at the end, so they
/// can capture the image first. For such use cases, use EZ_SCHEDULE_IMAGE_TEST at the start of a sub-test instead.
#define EZ_TEST_IMAGE_MSG(ImageNumber, MaxError, msg, ...) \
  ezTestImage(ImageNumber, MaxError, EZ_SOURCE_FILE, EZ_SOURCE_LINE, EZ_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

/// \brief Schedules an EZ_TEST_IMAGE to be executed after the current sub-test execution finishes.
///
/// Call this at the beginning of a sub-test, to automatically execute an image comparison when it is finished.
/// Calling ezTestFramework::IsImageComparisonScheduled() will now return true.
///
/// To support image comparisons, tests derived from ezTestBaseClass need to provide the current image through ezTestBaseClass::GetImage().
/// To support 'scheduled' image comparisons, the class should poll ezTestFramework::IsImageComparisonScheduled() every step and capture the
/// image when needed.
///
/// \note Scheduling image comparisons is an optimization to only capture data when necessary, instead of capturing it every single frame.
#define EZ_SCHEDULE_IMAGE_TEST(ImageNumber, MaxError) ezTestFramework::GetInstance()->ScheduleImageComparison(ImageNumber, MaxError);
