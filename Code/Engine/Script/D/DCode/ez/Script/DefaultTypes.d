module ez.Script.DefaultTypes;

import std.typetuple;
import ez.Script.Reflection;
import ez.Script.DGlue.Allocator;
import ez.Foundation.Memory.AllocatorBase;

alias BuiltinTypes = TypeTuple!(void, char, wchar, dchar, byte, short, int, long, ubyte, ushort, uint, ulong, float, double,
                                void[], char[], wchar[], dchar[], byte[], short[], int[], long[], ubyte[], ushort[], uint[], ulong[], float[], double[],
                                const(char), immutable(char), const(char)[], string);

private
{
  string generateBuiltinTypeDeclarations(LIST...)()
  {
    string result;
    foreach(t; LIST)
    {
      result ~= "__gshared "~ nextType!t.info.stringof ~ " _builtinType_" ~ t.mangleof ~ ";\n";
    }
    return result;
  }

  string generateBuiltinTypeInitializations(LIST...)()
  {
    string result;
    foreach(t; LIST)
    {
      result ~= "_builtinType_" ~ t.mangleof ~ " = allocator.Make!("~t.stringof~")();\n";
    }
    return result;
  }

  auto Make(T)(ezScriptReflectionAllocator allocator)
  {
    alias tc = nextType!T;
    static if(is(tc.type == TypeChainEnd))
      return allocator.New!(tc.info)(T.stringof);
    else
      return allocator.New!(tc.info)(allocator.MakeNext!(tc.type));
  }

  auto MakeNext(T)(ezScriptReflectionAllocator allocator)
  {
    static assert(staticIndexOf!(T, BuiltinTypes) >= 0, T.stringof ~ " is not a builtin type");
    mixin("return _builtinType_" ~ T.mangleof ~ ";");
  }
}

//pragma(msg, generateBuiltinTypeDeclarations!BuiltinTypes);
mixin(generateBuiltinTypeDeclarations!BuiltinTypes);

struct test {};

shared static this()
{
  auto allocator = ezGetDefaultScriptReflectionAllocator();
  //pragma(msg, generateBuiltinTypeInitializations!BuiltinTypes);
  mixin(generateBuiltinTypeInitializations!BuiltinTypes);
  pragma(msg, test.mangleof);
  import std.stdio;
  writefln("%s", typeid(test).name);
}

shared static ~this()
{
  auto allocator = ezGetDefaultScriptReflectionAllocator();
  allocator.Reset();
}