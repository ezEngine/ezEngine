#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Utilities/EnumerableClass.h>
#include <RendererTest/TestClass/TestClass.h>
#include <TestFramework/Framework/Declarations.h>
#include <TestFramework/Framework/SimpleTest.h>

class ezSimpleRendererTestGroup : public ezGraphicsTest
{
public:
  using SimpleRendererTestFunc = void (*)();

  ezSimpleRendererTestGroup(const char* szName)
    : m_szTestName(szName)
  {
  }

  void AddSimpleRendererTest(const char* szName, SimpleRendererTestFunc testFunc);
  virtual const char* GetTestName() const override { return m_szTestName; }

private:
  virtual void SetupSubTests() override;
  virtual ezTestAppRun RunSubTest(ezInt32 iIdentifier, ezUInt32 uiInvocationCount) override;

private:
  struct SimpleRendererTestEntry
  {
    const char* m_szName;
    SimpleRendererTestFunc m_Func;
  };

  const char* m_szTestName;
  std::deque<SimpleRendererTestEntry> m_SimpleRendererTests;
};

class ezRegisterSimpleRendererTestHelper : public ezRegisterTestHelper
{
public:
  ezRegisterSimpleRendererTestHelper(ezSimpleRendererTestGroup* pTestGroup, const char* szTestName, ezSimpleRendererTestGroup::SimpleRendererTestFunc func)
  {
    m_pTestGroup = pTestGroup;
    m_szTestName = szTestName;
    m_Func = func;
  }

  virtual void RegisterTest() override { m_pTestGroup->AddSimpleRendererTest(m_szTestName, m_Func); }

private:
  ezSimpleRendererTestGroup* m_pTestGroup;
  const char* m_szTestName;
  ezSimpleRendererTestGroup::SimpleRendererTestFunc m_Func;
};

#define EZ_CREATE_SIMPLE_RENDERER_TEST_GROUP(GroupName) ezSimpleRendererTestGroup EZ_PP_CONCAT(g_SimpleRendererTestGroup__, GroupName)(EZ_PP_STRINGIFY(GroupName));

#define EZ_CREATE_SIMPLE_RENDERER_TEST(GroupName, TestName)                                                                                    \
  extern ezSimpleRendererTestGroup EZ_PP_CONCAT(g_SimpleRendererTestGroup__, GroupName);                                                       \
  static void ezSimpleRendererTestFunction__##GroupName##_##TestName();                                                                        \
  ezRegisterSimpleRendererTestHelper ezRegisterSimpleRendererTest__##GroupName##TestName(                                                      \
    &EZ_PP_CONCAT(g_SimpleRendererTestGroup__, GroupName), EZ_PP_STRINGIFY(TestName), ezSimpleRendererTestFunction__##GroupName##_##TestName); \
  static void ezSimpleRendererTestFunction__##GroupName##_##TestName()
