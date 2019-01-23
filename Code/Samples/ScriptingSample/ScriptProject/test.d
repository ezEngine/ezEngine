module test;
import ez.Script.Reflection.Reflection;
import ez.Script.Reflection.Types;

mixin ReflectModule;

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