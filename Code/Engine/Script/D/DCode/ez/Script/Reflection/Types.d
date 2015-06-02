module ez.Script.Reflection.Types;

import std.typetuple;
import std.algorithm : splitter;
import core.sync.mutex;
import ez.Script.Reflection.Reflection;
import ez.Script.DGlue.Allocator;
import ez.Foundation.Strings.StringBuilder;
import ez.Foundation.Memory.AllocatorBase;
import ez.Foundation.Basics;

alias BasicTypes = TypeTuple!(void, char, wchar, dchar, byte, short, int, long, ubyte, ushort, uint, ulong, float, double);

alias BuiltinTypes = TypeTuple!(void, char, wchar, dchar, byte, short, int, long, ubyte, ushort, uint, ulong, float, double,
                                void[], char[], wchar[], dchar[], byte[], short[], int[], long[], ubyte[], ushort[], uint[], ulong[], float[], double[],
                                const(char), immutable(char), const(char)[], string, Object);

private __gshared ReflectedType[string] g_types;
private __gshared Mutex g_typeLock;

private
{
  string generateBuiltinTypeDeclarations(LIST...)()
  {
    string result;
    foreach(t; LIST)
    {
      result ~= "export __gshared "~ nextType!t.info.stringof ~ " _builtinType_" ~ t.mangleof ~ ";\n";
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

  BasicType.Type MapBasicType(T)()
  {
    static if(is(T == void))
      return BasicType.Type.Void;
    else static if(is(T == char))
      return BasicType.Type.Char;
    else static if(is(T == wchar))
      return BasicType.Type.Wchar;
    else static if(is(T == dchar))
      return BasicType.Type.Dchar;
    else static if(is(T == byte))
      return BasicType.Type.Byte;
    else static if(is(T == short))
      return BasicType.Type.Short;
    else static if(is(T == int))
      return BasicType.Type.Int;
    else static if(is(T == long))
      return BasicType.Type.Long;
    else static if(is(T == ubyte))
      return BasicType.Type.Ubyte;
    else static if(is(T == ushort))
      return BasicType.Type.Ushort;
    else static if(is(T == uint))
      return BasicType.Type.Uint;
    else static if(is(T == ulong))
      return BasicType.Type.Ulong;
    else static if(is(T == float))
      return BasicType.Type.Float;
    else static if(is(T == double))
      return BasicType.Type.Double;
    else
      static assert(0, T.stringof ~ " is not a basic type");
  }

  auto Make(T)(ezScriptReflectionAllocator allocator)
  {
    alias tc = nextType!T;
    static if(is(tc.type == TypeChainEnd))
    {
      static if(staticIndexOf!(T, BasicTypes) >= 0)
        return allocator.New!(tc.info)(T.stringof, MapBasicType!T);
      else static if(staticIndexOf!(T, BuiltinTypes) >= 0)
        return allocator.New!(tc.info)(T.stringof);
      else
      {
        auto identifier = __traits(identifier, T);
        auto name = allocator.NewArray!char(identifier.length);
        name[] = identifier[];
        return allocator.New!(tc.info)(name);
      }
    }
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
  import std.stdio;
  //writefln("%s", "bla");
}

shared static ~this()
{
  g_typeLock = null;
}

extern(C++)
{
  private void ezInitDefaultScriptReflectionAllocator();
  private void ezDeinitDefaultScriptReflectionAllocator();
}

void InitializeScriptReflection()
{
  ezInitDefaultScriptReflectionAllocator();
  auto allocator = ezGetDefaultScriptReflectionAllocator();
  //pragma(msg, generateBuiltinTypeInitializations!BuiltinTypes);
  mixin(generateBuiltinTypeInitializations!BuiltinTypes);
}

void ShutdownScriptReflection()
{
  auto allocator = ezGetDefaultScriptReflectionAllocator();
  allocator.Reset();
  ezDeinitDefaultScriptReflectionAllocator();
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

ReflectedType GetReflectedType(T)()
{
  static if(staticIndexOf!(T, BuiltinTypes) >= 0)
  {
    mixin("return _builtinType_" ~ T.mangleof ~ ";");
  }
  else
  {
    auto result = GetReflectedTypeImpl(T.mangleof);
    if(result is null)
    {
      auto allocator = ezGetDefaultScriptReflectionAllocator();
      auto mangling = T.mangleof;
      auto nameCopy = allocator.NewArray!char(mangling.length);
      nameCopy[] = mangling[];
      auto newType = allocator.Make!T();
      static if(is(T == struct))
        newType.size = T.sizeof;
      result = newType;
      g_types[cast(string)nameCopy] = result;
    }
    return result;
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


