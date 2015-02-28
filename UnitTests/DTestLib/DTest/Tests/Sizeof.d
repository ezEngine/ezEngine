module DTest.Tests.Sizeof;
import DTest.TestFramework;

import ez.Foundation.Types.Delegate;

mixin TestGroup!("Sizeof");

extern(C++) size_t SizeOfDelegate();

@Test("Sizeof")
void SizeofTest()
{
  if(EZ_TEST_BLOCK(ezTestBlock.Enabled, "Delegate"))
  {
    EZ_TEST_INT(ezDelegate!(Signature!(void function())).sizeof, SizeOfDelegate());
  }
}
