#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Utilities/EnumerableClass.h>
#include <TestFramework/Framework/Declarations.h>
#include <TestFramework/Framework/TestBaseClass.h>

class EZ_TEST_DLL ezSimpleTestGroup : public ezTestBaseClass
{
public:
  typedef void(*SimpleTestFunc)();

  ezSimpleTestGroup(const char* szName) : m_szTestName(szName) { }

  void AddSimpleTest(const char* szName, SimpleTestFunc TestFunc);

  virtual const char* GetTestName() const override { return m_szTestName; }

private:
  virtual void SetupSubTests() override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier) override;
  virtual ezResult InitializeSubTest(ezInt32 iIdentifier) override;
  virtual ezResult DeInitializeSubTest(ezInt32 iIdentifier) override;

private:
  struct SimpleTestEntry
  {
    const char* m_szName;
    SimpleTestFunc m_Func;
    ezProfilingId m_ProfilingId;
  };

  const char* m_szTestName;
  std::deque<SimpleTestEntry> m_SimpleTests;
};

class EZ_TEST_DLL ezRegisterSimpleTestHelper : public ezEnumerable<ezRegisterSimpleTestHelper>
{
  EZ_DECLARE_ENUMERABLE_CLASS(ezRegisterSimpleTestHelper);

public:

  ezRegisterSimpleTestHelper(ezSimpleTestGroup* pTestGroup, const char* szTestName, ezSimpleTestGroup::SimpleTestFunc Func)
  {
    m_pTestGroup = pTestGroup;
    m_szTestName = szTestName;
    m_Func = Func;
  }

  void RegisterTest()
  {
    m_pTestGroup->AddSimpleTest(m_szTestName, m_Func);
  }

private:
  ezSimpleTestGroup* m_pTestGroup;
  const char* m_szTestName;
  ezSimpleTestGroup::SimpleTestFunc m_Func;
};

#define EZ_CREATE_SIMPLE_TEST_GROUP(GroupName) \
  ezSimpleTestGroup EZ_CONCAT(g_SimpleTestGroup__,GroupName)(EZ_STRINGIZE(GroupName));

#define EZ_CREATE_SIMPLE_TEST(GroupName, TestName) \
  extern ezSimpleTestGroup EZ_CONCAT(g_SimpleTestGroup__,GroupName); \
  static void ezSimpleTestFunction__##GroupName##_##TestName(); \
  ezRegisterSimpleTestHelper ezRegisterSimpleTest__##GroupName##TestName(&EZ_CONCAT(g_SimpleTestGroup__,GroupName), EZ_STRINGIZE(TestName), ezSimpleTestFunction__##GroupName##_##TestName); \
  static void ezSimpleTestFunction__##GroupName##_##TestName()



