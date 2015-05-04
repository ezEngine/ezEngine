module DTest.Tests.Delegate;
import DTest.TestFramework;
import ez.Foundation.Types.Delegate;

mixin TestGroup!("Delegate");

alias DelType = ezDelegate!(Signature!(int function(float, double)));

extern(C++) int CallDelegate(ezDelegate!(Signature!(int function(DelType, float, double))), float, double);

int TargetFunction(DelType del, float arg1, double arg2)
{
  return del(arg1, arg2);
}

struct WithContext
{
  int offset;

  int TargetFunction(DelType del, float arg1, double arg2)
  {
    return del(arg1, arg2) + offset;
  }
}

@Test("Delegate")
void DelegateTest()
{
  if(EZ_TEST_BLOCK(ezTestBlock.Enabled, "Cross D/C++ Calls"))
  {
    EZ_TEST_INT(3, CallDelegate(ezMakeDelegate(&TargetFunction), 1.0f, 2.0));

    auto context = WithContext(5);
    EZ_TEST_INT(8, CallDelegate(ezMakeDelegate(&context.TargetFunction), 1.0f, 2.0));
  }

  if(EZ_TEST_BLOCK(ezTestBlock.Enabled, "Delegate comparison"))
  {
    auto context = WithContext(10);
    EZ_TEST_BOOL(ezMakeDelegate(&TargetFunction) == ezMakeDelegate(&TargetFunction));

    EZ_TEST_BOOL(ezMakeDelegate(&context.TargetFunction) == ezMakeDelegate(&context.TargetFunction));
  }
}