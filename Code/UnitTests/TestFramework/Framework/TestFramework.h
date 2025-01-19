#pragma once

#include <TestFramework/Framework/Declarations.h>
#include <TestFramework/Framework/SimpleTest.h>
#include <TestFramework/Framework/TestBaseClass.h>
#include <TestFramework/Framework/TestResults.h>
#include <TestFramework/TestFrameworkDLL.h>
#include <TestFrameworkEntryPoint_Platform.h>

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

class ezCommandLineUtils;

// Disable C++/CX adds.
#pragma warning(disable : 4447)

class EZ_TEST_DLL ezTestFramework
{
public:
  ezTestFramework(const char* szTestName, const char* szAbsTestOutputDir, const char* szRelTestDataDir, int iArgc, const char** pArgv);
  virtual ~ezTestFramework();

  using OutputHandler = void (*)(ezTestOutput::Enum, const char*);

  // Test management
  void CreateOutputFolder();
  void UpdateReferenceImages();
  const char* GetTestName() const;
  const char* GetAbsOutputPath() const;
  const char* GetRelTestDataPath() const;
  const char* GetAbsTestOrderFilePath() const;
  const char* GetAbsTestSettingsFilePath() const;
  void RegisterOutputHandler(OutputHandler handler);
  void GatherAllTests();
  void LoadTestOrder();
  void ApplyTestOrderFromCommandLine(const ezCommandLineUtils& cmd);
  void LoadTestSettings();
  void AutoSaveTestOrder();
  void SaveTestOrder(const char* const szFilePath);
  void SaveTestSettings(const char* const szFilePath);
  void SetAllTestsEnabledStatus(bool bEnable);
  void SetAllFailedTestsEnabledStatus();
  // Each function on a test must not take longer than the given time or the test process will be terminated.
  void SetTestTimeout(ezUInt32 uiTestTimeoutMS);
  ezUInt32 GetTestTimeout() const;
  void GetTestSettingsFromCommandLine(const ezCommandLineUtils& cmd);

  // Test execution
  void ResetTests();
  ezTestAppRun RunTestExecutionLoop();

  /// \brief Top-level function to run tests, can be overridden by platform specific implementations
  virtual ezTestAppRun RunTests() { return RunTestExecutionLoop(); }

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

  ezUInt32 GetCurrentTestIndex() const { return m_uiCurrentTestIndex; }
  ezUInt32 GetCurrentSubTestIndex() const { return m_uiCurrentSubTestIndex; }
  ezInt32 GetCurrentSubTestIdentifier() const;

  /// \brief Returns the index of the sub-test with the given identifier.
  ///
  /// Only looks at the currently running test, assuming that the identifier is unique among its sub-tests.
  ezUInt32 FindSubTestIndexForSubTestIdentifier(ezInt32 iSubTestIdentifier) const;

  ezTestEntry* GetTest(ezUInt32 uiTestIndex);
  const ezTestEntry* GetTest(ezUInt32 uiTestIndex) const;
  bool GetTestsRunning() const { return m_bTestsRunning; }

  const ezTestEntry* GetCurrentTest() const;
  const ezSubTestEntry* GetCurrentSubTest() const;

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
  void ScheduleDepthImageComparison(ezUInt32 uiImageNumber, ezUInt32 uiMaxError);
  bool IsImageComparisonScheduled() const { return m_bImageComparisonScheduled; }
  bool IsDepthImageComparisonScheduled() const { return m_bDepthImageComparisonScheduled; }
  void GenerateComparisonImageName(ezUInt32 uiImageNumber, ezStringBuilder& ref_sImgName);
  void GetCurrentComparisonImageName(ezStringBuilder& ref_sImgName);
  void SetImageReferenceFolderName(const char* szFolderName);
  void SetImageReferenceOverrideFolderName(const char* szFolderName);

  /// \brief Writes an Html file that contains test information and an image diff view for failed image comparisons.
  void WriteImageDiffHtml(const char* szFileName, const ezImage& referenceImgRgb, const ezImage& referenceImgAlpha, const ezImage& capturedImgRgb, const ezImage& capturedImgAlpha, const ezImage& diffImgRgb, const ezImage& diffImgAlpha, ezUInt32 uiError, ezUInt32 uiThreshold, ezUInt8 uiMinDiffRgb,
    ezUInt8 uiMaxDiffRgb, ezUInt8 uiMinDiffAlpha, ezUInt8 uiMaxDiffAlpha);

  bool PerformImageComparison(ezStringBuilder sImgName, const ezImage& img, ezUInt32 uiMaxError, bool bIsLineImage, char* szErrorMsg);
  bool CompareImages(ezUInt32 uiImageNumber, ezUInt32 uiMaxError, char* szErrorMsg, bool bIsDepthImage = false, bool bIsLineImage = false);

  /// \brief A function to be called to add extra info to image diff output, that is not available from here.
  /// E.g. device specific info like driver version.
  using ImageDiffExtraInfoCallback = std::function<ezDynamicArray<std::pair<ezString, ezString>>()>;
  void SetImageDiffExtraInfoCallback(ImageDiffExtraInfoCallback provider);

  using ImageComparisonCallback = std::function<void(bool)>; /// \brief A function to be called after every image comparison with a bool
                                                             /// indicating if the images matched or not.
  void SetImageComparisonCallback(const ImageComparisonCallback& callback);

  static ezResult CaptureRegressionStat(ezStringView sTestName, ezStringView sName, ezStringView sUnit, float value, ezInt32 iTestId = -1);

protected:
  void Initialize();
  void DeInitialize();

  /// \brief Will be called for test failures to record the location of the failure and forward the error to OutputImpl.
  virtual void ErrorImpl(const char* szError, const char* szFile, ezInt32 iLine, const char* szFunction, const char* szMsg);
  /// \brief Receives ezLog messages (via LogWriter) as well as test-framework internal logging. Any ezTestOutput::Error will
  /// cause the test to fail.
  virtual void OutputImpl(ezTestOutput::Enum Type, const char* szMsg);
  virtual void TestResultImpl(ezUInt32 uiSubTestIndex, bool bSuccess, double fDuration);
  virtual void SetSubTestStatusImpl(ezUInt32 uiSubTestIndex, const char* szStatus);
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

  /// \brief Returns whether to assert on test failure.
  static bool GetAssertOnTestFail();

  static void Output(ezTestOutput::Enum type, const char* szMsg, ...);
  static void OutputArgs(ezTestOutput::Enum type, const char* szMsg, va_list szArgs);
  static void Error(const char* szError, const char* szFile, ezInt32 iLine, const char* szFunction, ezStringView sMsg, ...);
  static void Error(const char* szError, const char* szFile, ezInt32 iLine, const char* szFunction, ezStringView sMsg, va_list szArgs);
  static void TestResult(ezUInt32 uiSubTestIndex, bool bSuccess, double fDuration);
  static void SetSubTestStatus(ezUInt32 uiSubTestIndex, const char* szStatus);

  // static members
private:
  static ezTestFramework* s_pInstance;

private:
  std::string m_sTestName;                ///< The name of the tests being done
  std::string m_sAbsTestOutputDir;        ///< Absolute path to the output folder where results and temp data is stored
  std::string m_sRelTestDataDir;          ///< Relative path from the SDK to where the unit test data is located
  std::string m_sAbsTestOrderFilePath;    ///< Absolute path to the test order file
  std::string m_sAbsTestSettingsFilePath; ///< Absolute path to the test settings file
  ezInt32 m_iErrorCount = 0;
  ezInt32 m_iTestsFailed = 0;
  ezInt32 m_iTestsPassed = 0;
  TestSettings m_Settings;
  std::recursive_mutex m_OutputMutex;
  std::deque<OutputHandler> m_OutputHandlers;
  std::deque<ezTestEntry> m_TestEntries;
  ezTestFrameworkResult m_Result;
  ezAssertHandler m_PreviousAssertHandler = nullptr;
  ImageDiffExtraInfoCallback m_ImageDiffExtraInfoCallback;
  ImageComparisonCallback m_ImageComparisonCallback;

  std::mutex m_TimeoutLock;
  ezUInt32 m_uiTimeoutMS = 5 * 60 * 1000; // 5 min default timeout
  bool m_bUseTimeout = false;
  bool m_bArm = false;
  std::condition_variable m_TimeoutCV;
  std::thread m_TimeoutThread;

  ezUInt32 m_uiExecutingTest = 0;
  ezUInt32 m_uiExecutingSubTest = 0;
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

  bool m_bDepthImageComparisonScheduled = false;
  ezUInt32 m_uiMaxDepthImageComparisonError = 0;
  ezUInt32 m_uiComparisonDepthImageNumber = 0;

  std::string m_sImageReferenceFolderName = "Images_Reference";
  std::string m_sImageReferenceOverrideFolderName;

protected:
  ezUInt32 m_uiCurrentTestIndex = ezInvalidIndex;
  ezUInt32 m_uiCurrentSubTestIndex = ezInvalidIndex;
  bool m_bTestsRunning = false;
};

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

EZ_TEST_DLL bool ezTestBool(
  bool bCondition, const char* szErrorText, const char* szFile, ezInt32 iLine, const char* szFunction, const char* szMsg, ...);

/// \brief Tests for a boolean condition, does not output an extra message.
#define EZ_TEST_BOOL(condition) EZ_TEST_BOOL_MSG(condition, "")

/// \brief Tests for a boolean condition, outputs a custom message on failure.
#define EZ_TEST_BOOL_MSG(condition, msg, ...) \
  ezTestBool(condition, "Test failed: " EZ_PP_STRINGIFY(condition), EZ_SOURCE_FILE, EZ_SOURCE_LINE, EZ_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

EZ_TEST_DLL bool ezTestResult(
  ezResult condition, const char* szErrorText, const char* szFile, ezInt32 iLine, const char* szFunction, const char* szMsg, ...);

/// \brief Tests for a boolean condition, does not output an extra message.
#define EZ_TEST_RESULT(condition) EZ_TEST_RESULT_MSG(condition, "")

/// \brief Tests for a boolean condition, outputs a custom message on failure.
#define EZ_TEST_RESULT_MSG(condition, msg, ...) \
  ezTestResult(condition, "Test failed: " EZ_PP_STRINGIFY(condition), EZ_SOURCE_FILE, EZ_SOURCE_LINE, EZ_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

EZ_TEST_DLL bool ezTestResult(
  ezResult condition, const char* szErrorText, const char* szFile, ezInt32 iLine, const char* szFunction, const char* szMsg, ...);

/// \brief Tests for a boolean condition, does not output an extra message.
#define EZ_TEST_RESULT(condition) EZ_TEST_RESULT_MSG(condition, "")

/// \brief Tests for a boolean condition, outputs a custom message on failure.
#define EZ_TEST_RESULT_MSG(condition, msg, ...) \
  ezTestResult(condition, "Test failed: " EZ_PP_STRINGIFY(condition), EZ_SOURCE_FILE, EZ_SOURCE_LINE, EZ_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

/// \brief Tests for a ezStatus condition, outputs ezStatus message on failure
#define EZ_TEST_STATUS(condition)                    \
  auto EZ_PP_CONCAT(l_, EZ_SOURCE_LINE) = condition; \
  ezTestResult(EZ_PP_CONCAT(l_, EZ_SOURCE_LINE).GetResult(), "Test failed: " EZ_PP_STRINGIFY(condition), EZ_SOURCE_FILE, EZ_SOURCE_LINE, EZ_SOURCE_FUNCTION, EZ_PP_CONCAT(l_, EZ_SOURCE_LINE).GetMessageString())

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

EZ_TEST_DLL bool ezTestDouble(double f1, double f2, double fEps, const char* szF1, const char* szF2, const char* szFile, ezInt32 iLine,
  const char* szFunction, const char* szMsg, ...);

/// \brief Tests two floats for equality, within a given epsilon. On failure both actual and expected values are output.
#define EZ_TEST_FLOAT(f1, f2, epsilon) EZ_TEST_FLOAT_MSG(f1, f2, epsilon, "")

/// \brief Tests two floats for equality, within a given epsilon. On failure both actual and expected values are output, also a custom
/// message is printed.
#define EZ_TEST_FLOAT_MSG(f1, f2, epsilon, msg, ...)                                                                                                     \
  ezTestDouble(ToFloat(f1), ToFloat(f2), ToFloat(epsilon), EZ_PP_STRINGIFY(f1), EZ_PP_STRINGIFY(f2), EZ_SOURCE_FILE, EZ_SOURCE_LINE, EZ_SOURCE_FUNCTION, \
    msg, ##__VA_ARGS__)


//////////////////////////////////////////////////////////////////////////

/// \brief Tests two doubles for equality, within a given epsilon. On failure both actual and expected values are output.
#define EZ_TEST_DOUBLE(f1, f2, epsilon) EZ_TEST_DOUBLE_MSG(f1, f2, epsilon, "")

/// \brief Tests two doubles for equality, within a given epsilon. On failure both actual and expected values are output, also a custom
/// message is printed.
#define EZ_TEST_DOUBLE_MSG(f1, f2, epsilon, msg, ...)                                                                                                    \
  ezTestDouble(ToFloat(f1), ToFloat(f2), ToFloat(epsilon), EZ_PP_STRINGIFY(f1), EZ_PP_STRINGIFY(f2), EZ_SOURCE_FILE, EZ_SOURCE_LINE, EZ_SOURCE_FUNCTION, \
    msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

EZ_TEST_DLL bool ezTestInt(
  ezInt64 i1, ezInt64 i2, const char* szI1, const char* szI2, const char* szFile, ezInt32 iLine, const char* szFunction, const char* szMsg, ...);

/// \brief Tests two ints for equality. On failure both actual and expected values are output.
#define EZ_TEST_INT(i1, i2) EZ_TEST_INT_MSG(i1, i2, "")

/// \brief Tests two ints for equality. On failure both actual and expected values are output, also a custom message is printed.
#define EZ_TEST_INT_MSG(i1, i2, msg, ...) \
  ezTestInt(i1, i2, EZ_PP_STRINGIFY(i1), EZ_PP_STRINGIFY(i2), EZ_SOURCE_FILE, EZ_SOURCE_LINE, EZ_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

EZ_TEST_DLL bool ezTestString(ezStringView s1, ezStringView s2, const char* szString1, const char* szString2, const char* szFile, ezInt32 iLine, const char* szFunction, const char* szMsg, ...);

/// \brief Tests two strings for equality. On failure both actual and expected values are output.
#define EZ_TEST_STRING(i1, i2) EZ_TEST_STRING_MSG(i1, i2, "")

/// \brief Tests two strings for equality. On failure both actual and expected values are output, also a custom message is printed.
#define EZ_TEST_STRING_MSG(s1, s2, msg, ...)                                                                                                           \
  ezTestString(static_cast<ezStringView>(s1), static_cast<ezStringView>(s2), EZ_PP_STRINGIFY(s1), EZ_PP_STRINGIFY(s2), EZ_SOURCE_FILE, EZ_SOURCE_LINE, \
    EZ_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

EZ_TEST_DLL bool ezTestWString(std::wstring s1, std::wstring s2, const char* szString1, const char* szString2, const char* szFile, ezInt32 iLine,
  const char* szFunction, const char* szMsg, ...);

/// \brief Tests two strings for equality. On failure both actual and expected values are output.
#define EZ_TEST_WSTRING(i1, i2) EZ_TEST_WSTRING_MSG(i1, i2, "")

/// \brief Tests two strings for equality. On failure both actual and expected values are output, also a custom message is printed.
#define EZ_TEST_WSTRING_MSG(s1, s2, msg, ...)                                                                                               \
  ezTestWString(static_cast<const wchar_t*>(s1), static_cast<const wchar_t*>(s2), EZ_PP_STRINGIFY(s1), EZ_PP_STRINGIFY(s2), EZ_SOURCE_FILE, \
    EZ_SOURCE_LINE, EZ_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

/// \brief Tests two strings for equality. On failure both actual and expected values are output. Does not embed the original expression to
/// work around issues with the current code page and unicode literals.
#define EZ_TEST_STRING_UNICODE(i1, i2) EZ_TEST_STRING_UNICODE_MSG(i1, i2, "")

/// \brief Tests two strings for equality. On failure both actual and expected values are output, also a custom message is printed. Does not
/// embed the original expression to work around issues with the current code page and unicode literals.
#define EZ_TEST_STRING_UNICODE_MSG(s1, s2, msg, ...) \
  ezTestString(                                      \
    static_cast<const char*>(s1), static_cast<const char*>(s2), "", "", EZ_SOURCE_FILE, EZ_SOURCE_LINE, EZ_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

EZ_TEST_DLL bool ezTestVector(
  ezVec4d v1, ezVec4d v2, double fEps, const char* szCondition, const char* szFile, ezInt32 iLine, const char* szFunction, const char* szMsg, ...);

/// \brief Tests two ezVec2's for equality, using some epsilon. On failure both actual and expected values are output.
#define EZ_TEST_VEC2(i1, i2, epsilon) EZ_TEST_VEC2_MSG(i1, i2, epsilon, "")

/// \brief Tests two ezVec2's for equality. On failure both actual and expected values are output, also a custom message is printed.
#define EZ_TEST_VEC2_MSG(r1, r2, epsilon, msg, ...)                                                                                \
  ezTestVector(ezVec4d(ToFloat((r1).x), ToFloat((r1).y), 0, 0), ezVec4d(ToFloat((r2).x), ToFloat((r2).y), 0, 0), ToFloat(epsilon), \
    EZ_PP_STRINGIFY(r1) " == " EZ_PP_STRINGIFY(r2), EZ_SOURCE_FILE, EZ_SOURCE_LINE, EZ_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

/// \brief Tests two ezVec3's for equality, using some epsilon. On failure both actual and expected values are output.
#define EZ_TEST_VEC3(i1, i2, epsilon) EZ_TEST_VEC3_MSG(i1, i2, epsilon, "")

/// \brief Tests two ezVec3's for equality. On failure both actual and expected values are output, also a custom message is printed.
#define EZ_TEST_VEC3_MSG(r1, r2, epsilon, msg, ...)                                                                                          \
  ezTestVector(ezVec4d(ToFloat((r1).x), ToFloat((r1).y), ToFloat((r1).z), 0), ezVec4d(ToFloat((r2).x), ToFloat((r2).y), ToFloat((r2).z), 0), \
    ToFloat(epsilon), EZ_PP_STRINGIFY(r1) " == " EZ_PP_STRINGIFY(r2), EZ_SOURCE_FILE, EZ_SOURCE_LINE, EZ_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

/// \brief Tests two ezVec4's for equality, using some epsilon. On failure both actual and expected values are output.
#define EZ_TEST_VEC4(i1, i2, epsilon) EZ_TEST_VEC4_MSG(i1, i2, epsilon, "")

/// \brief Tests two ezVec4's for equality. On failure both actual and expected values are output, also a custom message is printed.
#define EZ_TEST_VEC4_MSG(r1, r2, epsilon, msg, ...)                                                                                                \
  ezTestVector(ezVec4d(ToFloat((r1).x), ToFloat((r1).y), ToFloat((r1).z), ToFloat((r1).w)),                                                        \
    ezVec4d(ToFloat((r2).x), ToFloat((r2).y), ToFloat((r2).z), ToFloat((r2).w)), ToFloat(epsilon), EZ_PP_STRINGIFY(r1) " == " EZ_PP_STRINGIFY(r2), \
    EZ_SOURCE_FILE, EZ_SOURCE_LINE, EZ_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

EZ_TEST_DLL bool ezTestFiles(
  const char* szFile1, const char* szFile2, const char* szFile, ezInt32 iLine, const char* szFunction, const char* szMsg, ...);

#define EZ_TEST_FILES(szFile1, szFile2, msg, ...) \
  ezTestFiles(szFile1, szFile2, EZ_SOURCE_FILE, EZ_SOURCE_LINE, EZ_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

EZ_TEST_DLL bool ezTestTextFiles(
  const char* szFile1, const char* szFile2, const char* szFile, ezInt32 iLine, const char* szFunction, const char* szMsg, ...);

#define EZ_TEST_TEXT_FILES(szFile1, szFile2, msg, ...) \
  ezTestTextFiles(szFile1, szFile2, EZ_SOURCE_FILE, EZ_SOURCE_LINE, EZ_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

EZ_TEST_DLL bool ezTestImage(
  ezUInt32 uiImageNumber, ezUInt32 uiMaxError, bool bIsDepthImage, bool bIsLineImage, const char* szFile, ezInt32 iLine, const char* szFunction, const char* szMsg, ...);

/// \brief Same as EZ_TEST_IMAGE_MSG but uses an empty error message.
#define EZ_TEST_IMAGE(ImageNumber, MaxError) EZ_TEST_IMAGE_MSG(ImageNumber, MaxError, "")

/// \brief Same as EZ_TEST_DEPTH_IMAGE_MSG but uses an empty error message.
#define EZ_TEST_DEPTH_IMAGE(ImageNumber, MaxError) EZ_TEST_DEPTH_IMAGE_MSG(ImageNumber, MaxError, "")

/// \brief Same as EZ_TEST_LINE_IMAGE_MSG but uses an empty error message.
#define EZ_TEST_LINE_IMAGE(ImageNumber, MaxError) EZ_TEST_LINE_IMAGE_MSG(ImageNumber, MaxError, "")

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
/// Use the * DEPTH * variant if a depth buffer comparison should be requested.
///
/// \note Some tests need to know at the start, whether an image comparison will be done at the end, so they
/// can capture the image first. For such use cases, use EZ_SCHEDULE_IMAGE_TEST at the start of a sub-test instead.
#define EZ_TEST_IMAGE_MSG(ImageNumber, MaxError, msg, ...) \
  ezTestImage(ImageNumber, MaxError, false, false, EZ_SOURCE_FILE, EZ_SOURCE_LINE, EZ_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

#define EZ_TEST_DEPTH_IMAGE_MSG(ImageNumber, MaxError, msg, ...) \
  ezTestImage(ImageNumber, MaxError, true, false, EZ_SOURCE_FILE, EZ_SOURCE_LINE, EZ_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

/// \brief Same as EZ_TEST_IMAGE_MSG, but allows for pixels to shift in a 1-pixel radius to account for different line rasterization of GPU vendors.
#define EZ_TEST_LINE_IMAGE_MSG(ImageNumber, MaxError, msg, ...) \
  ezTestImage(ImageNumber, MaxError, false, true, EZ_SOURCE_FILE, EZ_SOURCE_LINE, EZ_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

/// \brief Schedules an EZ_TEST_IMAGE to be executed after the current sub-test execution finishes.
///
/// Call this at the beginning of a sub-test, to automatically execute an image comparison when it is finished.
/// Calling ezTestFramework::IsImageComparisonScheduled() will now return true.
///
/// To support image comparisons, tests derived from ezTestBaseClass need to provide the current image through ezTestBaseClass::GetImage().
/// To support 'scheduled' image comparisons, the class should poll ezTestFramework::IsImageComparisonScheduled() every step and capture the
/// image when needed.
///
/// Use the * DEPTH * variant if a depth buffer comparison is intended.
///
/// \note Scheduling image comparisons is an optimization to only capture data when necessary, instead of capturing it every single frame.
#define EZ_SCHEDULE_IMAGE_TEST(ImageNumber, MaxError) ezTestFramework::GetInstance()->ScheduleImageComparison(ImageNumber, MaxError);

#define EZ_SCHEDULE_DEPTH_IMAGE_TEST(ImageNumber, MaxError) ezTestFramework::GetInstance()->ScheduleDepthImageComparison(ImageNumber, MaxError);
