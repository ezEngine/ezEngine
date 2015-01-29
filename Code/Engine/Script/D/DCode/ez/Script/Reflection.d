module ez.script.reflection;

export:

struct Member
{
  const(char)[] name;
  TypeInfo type;
}

struct Variable
{
  const(char)[] name;
  void* address;
  TypeInfo type;
}

alias tlsVariableAccessor = void* function();

struct TlsVariable
{
  const(char)[] name;
  tlsVariableAccessor accessor;
  TypeInfo type;
}

struct ReflectedType
{
  TypeInfo type;
  Member[] members;
}

template ResolveType(T)
{
  alias ResolveType = T;
}

class ReflectedModule
{
  abstract Variable[] variables();
  abstract TlsVariable[] tlsVariables();
}

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