#include <FoundationTestPCH.h>

#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>

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
      EZ_TEST_BOOL(m_ctorDel.IsEqualIfComparable(m_dtorDel));
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
    EZ_TEST_BOOL(d.IsEqualIfComparable(TestDelegate(&TestType::Method, &test)));
    EZ_TEST_BOOL(d.IsComparable());
    EZ_TEST_INT(d(4), 46);

    d = TestDelegate(&TestTypeDerived::Method, &test);
    EZ_TEST_BOOL(d.IsEqualIfComparable(TestDelegate(&TestTypeDerived::Method, &test)));
    EZ_TEST_BOOL(d.IsComparable());
    EZ_TEST_INT(d(4), 8);

  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Method With Many Params")
  {
    typedef ezDelegate<ezInt32(ezInt32, ezInt32, ezInt32, ezInt32, ezInt32, ezInt32)> TestDelegateMany;
    TestDelegateMany many;

    TestType test;
    test.m_iA = 1000000;

    many = TestDelegateMany(&TestType::MethodWithManyParams, &test);
    EZ_TEST_BOOL(many.IsEqualIfComparable(TestDelegateMany(&TestType::MethodWithManyParams, &test)));
    EZ_TEST_BOOL(d.IsComparable());
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
    EZ_TEST_BOOL(d.IsEqualIfComparable(TestDelegate(&TestType::ConstMethod, &constTest)));
    EZ_TEST_BOOL(d.IsComparable());
    EZ_TEST_INT(d(4), 43);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Virtual Method")
  {
    TestTypeDerived test;

    d = TestDelegate(&TestType::VirtualMethod, &test);
    EZ_TEST_BOOL(d.IsEqualIfComparable(TestDelegate(&TestType::VirtualMethod, &test)));
    EZ_TEST_BOOL(d.IsComparable());
    EZ_TEST_INT(d(4), 47);

    d = TestDelegate(&TestTypeDerived::VirtualMethod, &test);
    EZ_TEST_BOOL(d.IsEqualIfComparable(TestDelegate(&TestTypeDerived::VirtualMethod, &test)));
    EZ_TEST_BOOL(d.IsComparable());
    EZ_TEST_INT(d(4), 47);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Function")
  {
    d = &Function;
    EZ_TEST_BOOL(d.IsEqualIfComparable(&Function));
    EZ_TEST_BOOL(d.IsComparable());
    EZ_TEST_INT(d(4), 6);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Lambda - no capture")
  {
    d = [](ezInt32 i) { return i * 4; };
    EZ_TEST_BOOL(d.IsComparable());
    EZ_TEST_INT(d(2), 8);

    TestDelegate d2 = d;
    EZ_TEST_BOOL(d2.IsEqualIfComparable(d));
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Lambda - capture by value")
  {
    ezInt32 c = 20;
    d = [c](ezInt32)
    {
      return c;
    };
    EZ_TEST_BOOL(!d.IsComparable());
    EZ_TEST_INT(d(3), 20);
    c = 10;
    EZ_TEST_INT(d(3), 20);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Lambda - capture by value, mutable")
  {
    ezInt32 c = 20;
    d = [c](ezInt32) mutable { return c; };
    EZ_TEST_BOOL(!d.IsComparable());
    EZ_TEST_INT(d(3), 20);
    c = 10;
    EZ_TEST_INT(d(3), 20);

    d = [c](ezInt32 b) mutable -> decltype(b + c) {
      auto result = b + c;
      c = 1;
      return result;
    };
    EZ_TEST_BOOL(!d.IsComparable());
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
    EZ_TEST_BOOL(!d.IsComparable());
    EZ_TEST_INT(d(3), 3);
    EZ_TEST_INT(c, 5);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Lambda - capture by value of non-pod")
  {
    struct RefCountedInt : public ezRefCounted
    {
      RefCountedInt() = default;
      RefCountedInt(int i)
        : m_value(i)
      {
      }
      int m_value;
    };

    ezSharedPtr<RefCountedInt> shared = EZ_DEFAULT_NEW(RefCountedInt, 1);
    EZ_TEST_INT(shared->GetRefCount(), 1);
    {
      TestDelegate deleteMe = [shared](ezInt32 i) -> decltype(i) { return 0; };
      EZ_TEST_BOOL(!deleteMe.IsComparable());
      EZ_TEST_INT(shared->GetRefCount(), 2);
    }
    EZ_TEST_INT(shared->GetRefCount(), 1);

  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Lambda - capture lots of things")
  {
    ezInt64 a = 10;
    ezInt64 b = 20;
    ezInt64 c = 30;
    d = [a, b, c](ezInt32 i) -> ezInt32 { return static_cast<ezInt32>(a + b + c + i); };
    EZ_TEST_INT(d(6), 66);
    EZ_TEST_BOOL(!d.IsComparable());
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Lambda - capture lots of things - custom allocator")
  {
    ezInt64 a = 10;
    ezInt64 b = 20;
    ezInt64 c = 30;
    d = TestDelegate([a, b, c](ezInt32 i) -> ezInt32 { return static_cast<ezInt32>(a + b + c + i); }, ezFoundation::GetAlignedAllocator());
    EZ_TEST_INT(d(6), 66);
    EZ_TEST_BOOL(!d.IsComparable());

    d.Invalidate();
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Move semantics")
  {
    // Move pure function
    {
      d.Invalidate();
      TestDelegate d2 = &Function;
      d = std::move(d2);
      EZ_TEST_BOOL(d.IsValid());
      EZ_TEST_BOOL(!d2.IsValid());
      EZ_TEST_BOOL(d.IsComparable());
      EZ_TEST_INT(d(4), 6);
    }

    // Move delegate
    ezConstructionCounter::Reset();    
    d.Invalidate();
    {
      ezConstructionCounter value;
      value.m_iData = 666;
      EZ_TEST_INT(ezConstructionCounter::s_iConstructions, 1);
      EZ_TEST_INT(ezConstructionCounter::s_iDestructions, 0);
      TestDelegate d2 = [value](ezInt32 i) -> ezInt32 { return value.m_iData; };
      EZ_TEST_INT(ezConstructionCounter::s_iConstructions, 3); // Capture plus moving the lambda.
      EZ_TEST_INT(ezConstructionCounter::s_iDestructions, 1); // Move of lambda
      d = std::move(d2);
      // Moving a construction counter also counts as construction
      EZ_TEST_INT(ezConstructionCounter::s_iConstructions, 4);
      EZ_TEST_INT(ezConstructionCounter::s_iDestructions, 1);
      EZ_TEST_BOOL(d.IsValid());
      EZ_TEST_BOOL(!d2.IsValid());
      EZ_TEST_BOOL(!d.IsComparable());
      EZ_TEST_INT(d(0), 666);
    }
    EZ_TEST_INT(ezConstructionCounter::s_iDestructions, 2); // value out of scope
    EZ_TEST_INT(ezConstructionCounter::s_iConstructions, 4);
    d.Invalidate();
    EZ_TEST_INT(ezConstructionCounter::s_iDestructions, 3); // lambda destroyed.
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Lambda - Copy")
  {
    d.Invalidate();
    ezConstructionCounter::Reset();
    {
      ezConstructionCounter value;
      value.m_iData = 666;
      EZ_TEST_INT(ezConstructionCounter::s_iConstructions, 1);
      EZ_TEST_INT(ezConstructionCounter::s_iDestructions, 0);
      TestDelegate d2 = TestDelegate([value](ezInt32 i) -> ezInt32 { return value.m_iData; }, ezFoundation::GetAlignedAllocator());
      EZ_TEST_INT(ezConstructionCounter::s_iConstructions, 3); // Capture plus moving the lambda.
      EZ_TEST_INT(ezConstructionCounter::s_iDestructions, 1);  // Move of lambda
      d = d2;
      EZ_TEST_INT(ezConstructionCounter::s_iConstructions, 4); // Lambda Copy
      EZ_TEST_INT(ezConstructionCounter::s_iDestructions, 1);
      EZ_TEST_BOOL(d.IsValid());
      EZ_TEST_BOOL(d2.IsValid());
      EZ_TEST_BOOL(!d.IsComparable());
      EZ_TEST_BOOL(!d2.IsComparable());
      EZ_TEST_INT(d(0), 666);
      EZ_TEST_INT(d2(0), 666);
    }
    EZ_TEST_INT(ezConstructionCounter::s_iDestructions, 3); // value and lambda out of scope
    EZ_TEST_INT(ezConstructionCounter::s_iConstructions, 4);
    d.Invalidate();
    EZ_TEST_INT(ezConstructionCounter::s_iDestructions, 4); // lambda destroyed.
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Lambda - capture non-copyable type")
  {
    ezUniquePtr<ezConstructionCounter> data(EZ_DEFAULT_NEW(ezConstructionCounter));
    data->m_iData = 666;
    TestDelegate d2 = [data = std::move(data)](ezInt32 i) -> ezInt32 { return data->m_iData; };
    EZ_TEST_INT(d2(0), 666);
    d = std::move(d2);
    EZ_TEST_BOOL(d.IsValid());
    EZ_TEST_BOOL(!d2.IsValid());
    EZ_TEST_INT(d(0), 666);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "ezMakeDelegate")
  {
    auto d1 = ezMakeDelegate(&Function);
    EZ_TEST_BOOL(d1.IsEqualIfComparable(ezMakeDelegate(&Function)));

    TestType instance;
    auto d2 = ezMakeDelegate(&TestType::Method, &instance);
    EZ_TEST_BOOL(d2.IsEqualIfComparable(ezMakeDelegate(&TestType::Method, &instance)));
    auto d3 = ezMakeDelegate(&TestType::ConstMethod, &instance);
    EZ_TEST_BOOL(d3.IsEqualIfComparable(ezMakeDelegate(&TestType::ConstMethod, &instance)));
    auto d4 = ezMakeDelegate(&TestType::VirtualMethod, &instance);
    EZ_TEST_BOOL(d4.IsEqualIfComparable(ezMakeDelegate(&TestType::VirtualMethod, &instance)));

    EZ_IGNORE_UNUSED(d1);
    EZ_IGNORE_UNUSED(d2);
    EZ_IGNORE_UNUSED(d3);
    EZ_IGNORE_UNUSED(d4);
  }
}
