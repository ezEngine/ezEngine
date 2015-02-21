module test;
import ez.Script.Reflection.Reflection;
import ez.Script.Reflection.Types;
import ez.Foundation.Types.Delegate;

//mixin ReflectModule;

class SomeClass
{
  int i;
  float f;
}

struct SomeStruct
{
  int i;
  float f;
}

enum SomeEnum
{
  Value1
}

alias SomeStructAlias = SomeStruct;

__gshared int g_var1;
shared(int) g_var2;
int g_tlsVar1;



void foo()
{
}

/*extern(C++)
{
  alias del1_t = Delegate1!(Signature!(void function()));
  alias del2_t = Delegate2!(Signature!(int function(float, double)), Signature!(int function(float, double)));
  extern(C++) void DelegateTest1(del1_t);
  extern(C++) void DelegateTest2(del2_t);
}*/

extern(C++) void CallTest1();

extern(C++) void Test1(ezDelegate!(Signature!(void function())) func)
{
  func();
}

int main(string[] args)
{
  ezDelegate!(Signature!(void function())) test;
  Test1(test);
  //ezDelegate!(Signature!(int function(int, float))) test2;
  CallTest1();
  //GetReflectedAggregateType("ez.Foundation.Memory.AllocatorBase");
  //DelegateTest1(del1_t());
  //DelegateTest2(del2_t());
  return 0;
}