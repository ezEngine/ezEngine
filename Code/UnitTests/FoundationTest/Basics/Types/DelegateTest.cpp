#include <FoundationTestPCH.h>

#include <Foundation/Types/Delegate.h>

namespace
{
  struct TestType
  {
    TestType() {}

    ezInt32 MethodWithManyParams(ezInt32 a, ezInt32 b, ezInt32 c, ezInt32 d, ezInt32 e, ezInt32 f) { return m_iA + a + b + c + d + e + f; }

    ezInt32 Method(ezInt32 b) { return b + m_iA; }

    ezInt32 ConstMethod(ezInt32 b) const { return b + m_iA + 4; }

    virtual ezInt32 VirtualMethod(ezInt32 b) { return b; }

    mutable ezInt32 m_iA;
  };

  struct TestTypeDerived : public TestType
  {
    ezInt32 Method(ezInt32 b) { return b + 4; }

    virtual ezInt32 VirtualMethod(ezInt32 b) override { return b + 43; }
  };

  struct BaseA
  {
    virtual ~BaseA() {}
    virtual void bar() {}

    int m_i1;
  };

  struct BaseB
  {
    virtual ~BaseB() {}
    virtual void foo() {}
    int m_i2;
  };

  struct ComplexClass : public BaseA, public BaseB
  {
    ComplexClass() { m_ctorDel = ezMakeDelegate(&ComplexClass::nonVirtualFunc, this); }

    virtual ~ComplexClass()
    {
      m_dtorDel = ezMakeDelegate(&ComplexClass::nonVirtualFunc, this);
      EZ_TEST_BOOL(m_ctorDel == m_dtorDel);
    }
    virtual void bar() override {}
    virtual void foo() override {}



    void nonVirtualFunc()
    {
      m_i1 = 1;
      m_i2 = 2;
      m_i3 = 3;
    }

    int m_i3;

    ezDelegate<void()> m_ctorDel;
    ezDelegate<void()> m_dtorDel;
  };

  static ezInt32 Function(ezInt32 b) { return b + 2; }
} // namespace

EZ_CREATE_SIMPLE_TEST(Basics, Delegate)
{
  typedef ezDelegate<ezInt32(ezInt32)> TestDelegate;
  TestDelegate d;

#if EZ_ENABLED(EZ_PLATFORM_64BIT)
  EZ_TEST_BOOL(sizeof(d) == 32);
#endif

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Method")
  {
    TestTypeDerived test;
    test.m_iA = 42;

    d = TestDelegate(&TestType::Method, &test);
    EZ_TEST_BOOL(d == TestDelegate(&TestType::Method, &test));
    EZ_TEST_INT(d(4), 46);

    d = TestDelegate(&TestTypeDerived::Method, &test);
    EZ_TEST_BOOL(d == TestDelegate(&TestTypeDerived::Method, &test));
    EZ_TEST_INT(d(4), 8);

  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Method With Many Params")
  {
    typedef ezDelegate<ezInt32(ezInt32, ezInt32, ezInt32, ezInt32, ezInt32, ezInt32)> TestDelegateMany;
    TestDelegateMany many;

    TestType test;
    test.m_iA = 1000000;

    many = TestDelegateMany(&TestType::MethodWithManyParams, &test);
    EZ_TEST_BOOL(many == TestDelegateMany(&TestType::MethodWithManyParams, &test));
    EZ_TEST_INT(many(1,10,100,1000,10000,100000), 1111111);

  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Complex Class")
  {
    ComplexClass* c = new ComplexClass();
    delete c;
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Const Method")
  {
    const TestType constTest;
    constTest.m_iA = 35;

    d = TestDelegate(&TestType::ConstMethod, &constTest);
    EZ_TEST_BOOL(d == TestDelegate(&TestType::ConstMethod, &constTest));
    EZ_TEST_INT(d(4), 43);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Virtual Method")
  {
    TestTypeDerived test;

    d = TestDelegate(&TestType::VirtualMethod, &test);
    EZ_TEST_BOOL(d == TestDelegate(&TestType::VirtualMethod, &test));
    EZ_TEST_INT(d(4), 47);

    d = TestDelegate(&TestTypeDerived::VirtualMethod, &test);
    EZ_TEST_BOOL(d == TestDelegate(&TestTypeDerived::VirtualMethod, &test));
    EZ_TEST_INT(d(4), 47);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Function")
  {
    d = &Function;
    EZ_TEST_BOOL(d == &Function);
    EZ_TEST_INT(d(4), 6);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Lambda - no capture")
  {
    d = [](ezInt32 i) { return i * 4; };
    EZ_TEST_INT(d(2), 8);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Lambda - capture by value")
  {
    ezInt32 c = 20;
    d = [c](ezInt32) { return c; };
    EZ_TEST_INT(d(3), 20);
    c = 10;
    EZ_TEST_INT(d(3), 20);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Lambda - capture by value, mutable")
  {
    ezInt32 c = 20;
    d = [c](ezInt32) mutable { return c; };
    EZ_TEST_INT(d(3), 20);
    c = 10;
    EZ_TEST_INT(d(3), 20);

    d = [c](ezInt32 b) mutable -> decltype(b + c) {
      auto result = b + c;
      c = 1;
      return result;
    };
    EZ_TEST_INT(d(3), 13);
    EZ_TEST_INT(d(3), 4);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Lambda - capture by reference")
  {
    ezInt32 c = 20;
    d = [&c](ezInt32 i) -> decltype(i) {
      c = 5;
      return i;
    };
    EZ_TEST_INT(d(3), 3);
    EZ_TEST_INT(c, 5);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezMakeDelegate")
  {
    auto d1 = ezMakeDelegate(&Function);
    EZ_TEST_BOOL(d1 == ezMakeDelegate(&Function));

    TestType instance;
    auto d2 = ezMakeDelegate(&TestType::Method, &instance);
    EZ_TEST_BOOL(d2 == ezMakeDelegate(&TestType::Method, &instance));
    auto d3 = ezMakeDelegate(&TestType::ConstMethod, &instance);
    EZ_TEST_BOOL(d3 == ezMakeDelegate(&TestType::ConstMethod, &instance));
    auto d4 = ezMakeDelegate(&TestType::VirtualMethod, &instance);
    EZ_TEST_BOOL(d4 == ezMakeDelegate(&TestType::VirtualMethod, &instance));

    EZ_IGNORE_UNUSED(d1);
    EZ_IGNORE_UNUSED(d2);
    EZ_IGNORE_UNUSED(d3);
    EZ_IGNORE_UNUSED(d4);
  }
}
