module ez.Script.Reflection;

export:

extern (C++) interface ezReflectedDModule
{
  Variable[] variables();
  TlsVariable[] tlsVariables();
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
	StructTypeImpl* oldType;
	StructTypeImpl* newType;
}

struct ClassTypeImpl
{
	const(char)[] name;
	ClassTypeImpl* baseClass;
	Member[] members;
}

class ClassType : ReflectedType
{
	ClassTypeImpl* oldType;
	ClassTypeImpl* newType;
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
  TypeInfo type;
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

void PrintMembers(alias T)()
{
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
            pragma(msg, "class " ~ type.stringof);
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
}

mixin template ReflectModule()
{
  class ReflectedModule
  {
    this()
    { 
      PrintMembers!(__traits(parent, ReflectedModule))();
    }
  }
}