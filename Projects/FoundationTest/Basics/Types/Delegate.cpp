#include <PCH.h>
#include <Foundation/Basics/Types/Delegate.h>

namespace
{
  struct TestType
  {
    ezInt32 Method(ezInt32 b)
    {
      return b + m_iA;
    }

    ezInt32 ConstMethod(ezInt32 b) const
    {
      return b + m_iA;
    }

    virtual ezInt32 VirtualMethod(ezInt32 b)
    {
      return b;
    }

    mutable ezInt32 m_iA;
  };

  struct TestTypeDerived : public TestType
  {
    ezInt32 Method(ezInt32 b)
    {
      return b + 4;
    }

    virtual ezInt32 VirtualMethod(ezInt32 b) EZ_OVERRIDE
    {
      return b + 43;
    }
  };

  static ezInt32 Function(ezInt32 b)
  {
    return b + 2;
  }
}

EZ_CREATE_SIMPLE_TEST(Basics, Delegate)
{
  typedef ezDelegate<ezInt32 (ezInt32)> TestDelegate;
  TestDelegate d;

#if EZ_ENABLED(EZ_PLATFORM_64BIT)
  EZ_TEST(sizeof(d) == 32);
#endif

  EZ_TEST_BLOCK(true, "Method")
  {
    TestTypeDerived test;
    test.m_iA = 42;

    d = TestDelegate(&TestType::Method, &test);
    EZ_TEST_INT(d(4), 46);

    d = TestDelegate(&TestTypeDerived::Method, &test);
    EZ_TEST_INT(d(4), 8);
  }

  EZ_TEST_BLOCK(true, "Const Method")
  {
    const TestType constTest;
    constTest.m_iA = 35;

    d = TestDelegate(&TestType::ConstMethod, &constTest);
    EZ_TEST_INT(d(4), 39);
  }

  EZ_TEST_BLOCK(true, "Virtual Method")
  {
    TestTypeDerived test;

    d = TestDelegate(&TestType::VirtualMethod, &test);
    EZ_TEST_INT(d(4), 47);

    d = TestDelegate(&TestTypeDerived::VirtualMethod, &test);
    EZ_TEST_INT(d(4), 47);
  }

  EZ_TEST_BLOCK(true, "Function")
  {
    d = &Function;
    EZ_TEST_INT(d(4), 6);
  }
}

