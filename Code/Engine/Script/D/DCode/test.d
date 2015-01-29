module test;
import ez.script.reflection;

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



void main(string[] args)
{
}