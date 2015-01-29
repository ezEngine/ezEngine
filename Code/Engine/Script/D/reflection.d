module reflection;

template ResolveType(T)
{
  alias ResolveType = T;
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
        else 
          pragma(msg, "unkown " ~ type.stringof);
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