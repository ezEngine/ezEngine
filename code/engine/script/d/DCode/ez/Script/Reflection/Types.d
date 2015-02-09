module ez.Script.Reflection.Types;

import std.typetuple;
import std.algorithm : splitter;
import core.sync.mutex;
import ez.Script.Reflection.Reflection;
import ez.Script.DGlue.Allocator;
import ez.Foundation.Strings.StringBuilder;
import ez.Foundation.Memory.AllocatorBase;
import ez.Foundation.Basics;

alias BuiltinTypes = TypeTuple!(void, char, wchar, dchar, byte, short, int, long, ubyte, ushort, uint, ulong, float, double,
                                void[], char[], wchar[], dchar[], byte[], short[], int[], long[], ubyte[], ushort[], uint[], ulong[], float[], double[],
                                const(char), immutable(char), const(char)[], string);

private __gshared ReflectedType[string] g_types;
private __gshared Mutex g_typeLock;

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
  g_typeLock = new Mutex();
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
  g_typeLock = null;
}

private ReflectedType GetReflectedTypeImpl(const(char)[] mangledType)
{
  g_typeLock.lock();
  scope(exit) g_typeLock.unlock();
  auto pType = mangledType in g_types;
  if(pType !is null)
    return *pType;
  return null;
}

ReflectedType GetReflectedType(T)(ezAllocatorBase allocator)
{
  static if(staticIndexOf!(T, BuiltinTypes) >= 0)
  {
    return mixin("_builtinType_" ~ T.mangleof ~ ";");
  }
  else
  {
    auto result = GetReflectedTypeImpl(T.mangleof);
    if(result is null)
    {
      auto mangling = T.mangleof;
      auto nameCopy = ezGetDefaultScriptReflectionAllocator().NewArray!char(mangling.length);
      nameCopy[] = mangling[];
      g_types[cast(string)nameCopy] = allocator.Make!T();
    }
  }
}

/*private*/ ReflectedType GetReflectedAggregateType(string name)
{
  ezStringBuilder buffer;
  ezConstructStringBuilder(buffer);
  scope(exit) ezDestroyStringBuilder(buffer);
  foreach(part; name.splitter('.'))
  {
    buffer.AppendFormat("%d%.*s", part.length, part.length, part.ptr);
  }
  auto mangled = buffer.GetString();
  auto result = GetReflectedTypeImpl(mangled);
  assert(result !is null, "trying to get non existing type");
  return result;
}

ReflectedType GetReflectedType(TypeInfo_Class info)
{
  return GetReflectedAggregateType(info.name);
}

ReflectedType GetReflectedType(TypeInfo_Struct info)
{
  return GetReflectedAggregateType(info.name);
}


