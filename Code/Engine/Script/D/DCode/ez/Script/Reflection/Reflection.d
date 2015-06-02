module ez.Script.Reflection.Reflection;
import ez.Foundation.Memory.AllocatorBase;
import ez.Script.DGlue.Allocator;
import ez.Script.Serialization.Serialization;
import std.conv;
import std.traits;
import std.stdio;
import core.stdc.string;

export:

class IReflectedModule
{
  abstract void BuildReflection(ezScriptReflectionAllocator allocator);
  abstract Variable[] variables();
  abstract TlsVariable[] tlsVariables();
  abstract ClassTypeImpl[] classes();
  abstract StructTypeImpl[] structs();
}

class ReflectedType
{
  enum Type
  {
    BasicType,
    PointerType,
    ArrayType,
    ConstType,
    ImmutableType,
    StructType,
    ClassType
  }
  
  Type metaType;

  final T castTo(T)()
  {
    mixin("assert(metaType == Type." ~ __traits(identifier, T) ~ ", \"invalid type conversion\");");
    return cast(T)cast(void*)this;
  }

  // unqualifies the type, e.g. removing const, immutable, shared until hitting a "real" type
  ReflectedType unqual()
  {
    return this;
  }
}

class BasicType : ReflectedType
{
  enum Type
  {
    Void,
    Byte,
    Short,
    Int,
    Long,
    Ubyte,
    Ushort,
    Uint,
    Ulong,
    Char,
    Wchar,
    Dchar,
    Bool,
    Float,
    Double,
    Object
  }

  this(const(char)[] name, Type type)
  {
    this.name = name;
    this.metaType = ReflectedType.Type.BasicType;
  }

  const(char)[] name;
  Type type;
}

class PointerType : ReflectedType
{
  this(ReflectedType next)
  {
    this.next = next;
    this.metaType = Type.PointerType;
  }

	ReflectedType next;
}

class ArrayType : ReflectedType
{
  this(ReflectedType next)
  {
    this.next = next;
    this.metaType = Type.ArrayType;
  }

	ReflectedType next;
}

class ConstType : ReflectedType
{
  this(ReflectedType next)
  {
    this.next = next;
    this.metaType = Type.ConstType;
  }

  override ReflectedType unqual()
  {
    return next.unqual;
  }

  ReflectedType next;
}

class ImmutableType : ReflectedType
{
  this(ReflectedType next)
  {
    this.next = next;
    this.metaType = Type.ImmutableType;
  }

  override ReflectedType unqual()
  {
    return next.unqual;
  }

  ReflectedType next;
}

struct Member
{
	const(char)[] name;
	size_t offset;
	ReflectedType type;
}

struct StructTypeImpl
{
	const(char)[] name;
  size_t size;
	Member[] members;
  void[] initializer;
}

class StructType : ReflectedType
{
  const(char)[] name;
	StructTypeImpl* curType;
	StructTypeImpl* newType;
  SerializationStep[] serialization;
  size_t size; // Size needed here for unreflected structs

  this(const(char)[] name)
  {
    this.name = name;
    this.metaType = Type.StructType;
  }
}

struct ClassTypeImpl
{
	const(char)[] name;
  size_t size;
	ClassType baseClass;
	Member[] members;
  void[] initializer;
}

class ClassType : ReflectedType
{
  const(char)[] name;
	ClassTypeImpl* curType;
	ClassTypeImpl* newType;

  this(const(char)[] name)
  {
    this.name = name;
    this.metaType = Type.ClassType;
  }
}

struct Variable
{
  const(char)[] name;
  void* address;
  ReflectedType type;
}

alias tlsVariableAccessor = void* function();

struct TlsVariable
{
  const(char)[] name;
  tlsVariableAccessor accessor;
  ReflectedType type;
}

template ResolveType(T)
{
  alias ResolveType = T;
}

struct TypeChainEnd {};

template nextType(T)
{
	static if(is(T foo : U*, U))
	{
		alias type = U;
		alias info = PointerType;
	}
	else static if(is(T foo : U[], U))
	{
		alias type = U;
		alias info = ArrayType;
	}
  else static if(is(T U == const U))
  {
    alias type = U;
    alias info = ConstType;
  }
  else static if(is(T U == immutable U))
  {
    alias type = U;
    alias info = ImmutableType;
  }
	else static if(is(T == struct))
	{
		alias type = TypeChainEnd;
		alias info = StructType;
	}
	else static if(is(T == class))
	{
		alias type = TypeChainEnd;
		alias info = ClassType;
	}
	else
	{
		alias type = TypeChainEnd;
		alias info = BasicType;
	}
}

static assert(is(nextType!int.type == TypeChainEnd));
static assert(is(nextType!int.info == BasicType));
static assert(is(nextType!(int*).type == int));
static assert(is(nextType!(int*).info == PointerType));
static assert(is(nextType!(byte[]).type == byte));
static assert(is(nextType!(byte[]).info == ArrayType));
static assert(is(nextType!(TlsVariable*).type == TlsVariable));
static assert(is(nextType!(TlsVariable*).info == PointerType));
static assert(is(nextType!(TlsVariable).type == TypeChainEnd));
static assert(is(nextType!(TlsVariable).info == StructType));
static assert(is(nextType!(ReflectedType[]).type == ReflectedType));
static assert(is(nextType!(ReflectedType[]).info == ArrayType));
static assert(is(nextType!(ReflectedType).type == TypeChainEnd));
static assert(is(nextType!(ReflectedType).info == ClassType));
static assert(is(nextType!(const(int)).type == int));
static assert(is(nextType!(const(int)).info == ConstType));
static assert(is(nextType!(immutable(int)).type == int));
static assert(is(nextType!(immutable(int)).info == ImmutableType));

const(char)[] CopyString(ezAllocatorBase allocator, const(char)[] str)
{
  auto copy = allocator.NewArray!char(str.length);
  copy[] = str[];
  return copy;
}

void[] CopyInitData(ezAllocatorBase allocator, TypeInfo info)
{
  void[] data = allocator.NewArray!void(info.init().length, InitializeMemoryWith.Nothing);
  if(info.init().ptr is null)
  {
    memset(data.ptr, 0, data.length);
  }
  else
  {
    data[] = info.init()[];
  }
  return data;
}

private extern(C) void _ez_script_register_type(const(ClassInfo) c, ReflectedType* info);

string GenerateMembers(T)(string cur)
{
  string result;
  size_t count;
  foreach(index, type; typeof(T.tupleof))
  {
    result ~= "curMember = &"~cur~".members[" ~ to!string(count) ~ "];\n";
    result ~= "curMember.name = \"" ~ T.tupleof[index].stringof ~ "\";\n";
    result ~= "curMember.offset = " ~ to!string(T.tupleof[index].offsetof) ~ ";\n";
    result ~= "curMember.type = GetReflectedType!(" ~ type.stringof ~ ");\n";
    count++;
  }
  return cur ~ ".members = allocator.NewArray!Member(" ~ to!string(count) ~ ");\n" ~ result;
}

string GenerateReflection(alias T)()
{
  string classes;
  size_t classNum = 0;
  string structs;
  size_t structNum = 0;
  foreach(m; __traits(allMembers, T))
  {
    static if(!__traits(compiles, typeof(__traits(getMember, T, m))))
    {
      static if(__traits(compiles, ResolveType!(__traits(getMember, T, m))))
      {
        alias type = ResolveType!(__traits(getMember, T, m));
        static if(is(type == class))
        {
          static if(type.stringof != "ReflectedModule")
          {
            pragma(msg, "class " ~ type.stringof);
            classes ~= "curClass = &m_classes[" ~ to!string(classNum) ~ "];\n";
            classes ~= "curClass.name = allocator.CopyString(\"" ~ __traits(identifier, type) ~ "\");\n";
            classes ~= "curClass.size = " ~ to!string(__traits(classInstanceSize, type)) ~ ";\n";
            classes ~= "curClass.baseClass = cast(ClassType)GetReflectedType!(" ~ __traits(identifier, BaseTypeTuple!type[0]) ~ ");\n";
            classes ~= "curClass.initializer = allocator.CopyInitData(typeid(" ~ __traits(identifier, type) ~ "));\n";
            classes ~= GenerateMembers!type("curClass");
            classes ~= "_ez_script_register_class_type(typeid(" ~ __traits(identifier, type) ~ "), curClass);";
            classNum++;
          }
        }
        else static if(is(type == struct))
        {
          pragma(msg, "struct " ~ type.stringof);
          structs ~= "curStruct = &m_structs[" ~ to!string(structNum) ~ "];\n";
          structs ~= "curStruct.name = allocator.CopyString(\"" ~ __traits(identifier, type) ~ "\");\n";
          structs ~= "curStruct.size = " ~ to!string(type.sizeof) ~ ";\n";
          structs ~= GenerateMembers!type("curStruct");
          structs ~= "curStruct.initializer = allocator.CopyInitData(typeid(" ~ fullyQualifiedName!type ~ "));\n";
          structNum++;
        }
        else static if(is(type == enum))
          pragma(msg, "enum " ~ type.stringof);
        else 
          pragma(msg, "unkown " ~ type.stringof);
      }
    }
    else 
    {
      alias type = typeof(__traits(getMember, T, m));
      static if(is(type == function))
      {
        pragma(msg, "function " ~ m);
      }
      else
      {
        static if(__traits(compiles, {__gshared auto ptr = &__traits(getMember, T, m);}))
        {
          pragma(msg, "variable " ~ m);
        }
        else
        {
          pragma(msg, "tls variable " ~ m);
        }
      }
    }
  }
  classes = "m_classes = allocator.NewArray!ClassTypeImpl(" ~ to!string(classNum) ~ ");\n" ~ classes;
  structs = "m_structs = allocator.NewArray!StructTypeImpl(" ~ to!string(structNum) ~ ");\n" ~ structs;
  return classes;
}

template moduleNameUnderscore(alias T)
{
  import std.algorithm : startsWith;

  static assert(!T.stringof.startsWith("package "), "cannot get the module name for a package");

  static if (T.stringof.startsWith("module "))
  {
    static if (__traits(compiles, packageName!T))
      enum packagePrefix = packageName!T ~ '_';
    else
      enum packagePrefix = "";

    enum moduleNameUnderscore = packagePrefix ~ T.stringof[7..$];
  }
  else
    alias moduleNameUnderscore!(__traits(parent, T)) moduleNameUnderscore;
}

string GenerateReflectionGetter(alias T)()
{
  string name = moduleNameUnderscore!T;
  return "export extern(C) IReflectedModule GetReflectedModule_" ~ name ~ "(ezAllocatorBase allocator) { return allocator.New!ReflectedModule(); }";
}

IReflectedModule GetReflectedModule(string name)
{
  foreach (m; ModuleInfo)
  {
    if (m && m.name == name)
      writefln("module %s", m.name);
      foreach (c; m.localClasses)
      {
        writefln("\tclass %s", c.name);
        if (c.name == "ReflectedModule")
        {
          return cast(IReflectedModule)c.create();
        }
      }
  }
  return null;
}

mixin template ReflectModule()
{
  import ez.Foundation.Memory.AllocatorBase;
  import ez.Script.DGlue.Allocator;

  class ReflectedModule : IReflectedModule
  {
  private:
    ClassTypeImpl[] m_classes;
    StructTypeImpl[] m_structs;
    Variable[] m_variables;
    TlsVariable[] m_tlsVariables;

  public:
    this(){}

    override void BuildReflection(ezScriptReflectionAllocator allocator)
    {
      ClassTypeImpl* curClass;
      StructTypeImpl* curStruct;
      Member* curMember;
      pragma(msg, GenerateReflection!(__traits(parent, ReflectedModule)));
      mixin(GenerateReflection!(__traits(parent, ReflectedModule)));
    }

    override Variable[] variables() { return m_variables; }
    override TlsVariable[] tlsVariables() { return m_tlsVariables; }
    override ClassTypeImpl[] classes() { return m_classes; }
    override StructTypeImpl[] structs() { return m_structs; }
  }

  mixin(GenerateReflectionGetter!ReflectedModule);
}