module ez.Script.Reflection.Reflection;
import ez.Foundation.Memory.AllocatorBase;
import std.conv;
import std.traits;

export:

class ReflectedDModule
{
  abstract Variable[] variables();
  abstract TlsVariable[] tlsVariables();
  abstract ClassTypeImpl*[] classes();
}

class ReflectedType
{
}

class BasicType : ReflectedType
{
  this(const(char)[] name)
  {
    this.name = name;
  }

  const(char)[] name;
}

class PointerType : ReflectedType
{
  this(ReflectedType next)
  {
    this.next = next;
  }

	ReflectedType next;
}

class ArrayType : ReflectedType
{
  this(ReflectedType next)
  {
    this.next = next;
  }

	ReflectedType next;
}

class ConstType : ReflectedType
{
  this(ReflectedType next)
  {
    this.next = next;
  }

  ReflectedType next;
}

class ImmutableType : ReflectedType
{
  this(ReflectedType next)
  {
    this.next = next;
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
	Member[] members;
}

class StructType : ReflectedType
{
  const(char)[] name;
	StructTypeImpl* oldType;
	StructTypeImpl* newType;

  this(const(char)[] name)
  {
    this.name = name;
  }
}

struct ClassTypeImpl
{
	const(char)[] name;
	ClassType baseClass;
	Member[] members;
}

class ClassType : ReflectedType
{
  const(char)[] name;
	ClassTypeImpl* oldType;
	ClassTypeImpl* newType;

  this(const(char)[] name)
  {
    this.name = name;
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

string GenerateMembers(T)(string cur)
{
  string result;
  size_t count;
  foreach(index, type; typeof(T.tupleof))
  {
    result ~= "curMember = &"~cur~".members[" ~ to!string(count) ~ "];\n";
    result ~= "curMember.name = \"" ~ T.tupleof[index].stringof ~ "\";\n";
    result ~= "curMember.offset = " ~ to!string(T.tupleof[index].offsetof) ~ ";\n";
    result ~= "curMember.type = allocator.GetReflectedType!(" ~ type.stringof ~ ");\n";
    count++;
  }
  return cur ~ ".members = allocator.NewArray!Member(" ~ to!string(count) ~ ");\n" ~ result;
}

string GenerateReflection(alias T)()
{
  string classes;
  size_t classNum = 0;
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
            classes ~= "curClass = &s_classes[" ~ to!string(classNum) ~ "];\n";
            classes ~= "curClass.name = allocator.CopyString(\"" ~ __traits(identifier, type) ~ "\");\n";
            classes ~= "curClass.baseClass = cast(ClassType)allocator.GetReflectedType!(" ~ __traits(identifier, BaseTypeTuple!type[0]) ~ ");\n";
            classes ~= GenerateMembers!type("curClass");
            classNum++;
          }
        }
        else static if(is(type == struct))
          pragma(msg, "struct " ~ type.stringof);
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
  classes = "s_classes = allocator.NewArray!ClassTypeImpl(" ~ to!string(classNum) ~ ");\n" ~ classes;
  return classes;
}

mixin template ReflectModule()
{
  import ez.Foundation.Memory.AllocatorBase;
  import ez.Script.DGlue.Allocator;

  class ReflectedModule
  {
    __gshared ClassTypeImpl[] s_classes;

    void BuildReflection(ezScriptReflectionAllocator allocator)
    {
      ClassTypeImpl* curClass;
      Member* curMember;
      //pragma(msg, GenerateReflection!(__traits(parent, ReflectedModule)));
      mixin(GenerateReflection!(__traits(parent, ReflectedModule)));
    }
  }
}