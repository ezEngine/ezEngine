module ez.Script.DefaultTypes;

import std.typetuple;
import ez.Script.Reflection;
import ez.Script.DGlue.Allocator;
import ez.Foundation.Memory.AllocatorBase;

alias BuiltinTypes = TypeTuple!(byte, short, int, long, ubyte, ushort, uint, ulong, float, double);

private
{
  string generateBuiltinTypeDeclarations(LIST...)()
  {
    string result;
    foreach(t; LIST)
    {
      result ~= "__gshared ReflectedType _builtinType_" ~ t.mangleof ~ ";\n";
    }
    return result;
  }

  string generateBuiltinTypeInitializations(LIST...)()
  {
    string result;
    foreach(t; LIST)
    {
      result ~= "_builtinType_" ~ t.mangleof;
    }
  }
}

//pragma(msg, generateBuiltinTypeDeclarations!BuiltinTypes);
mixin(generateBuiltinTypeDeclarations!BuiltinTypes);

shared static this()
{
  auto allocator = ezGetDefaultScriptReflectionAllocator();
  _builtinType_g = allocator.New!BasicType("int");
}