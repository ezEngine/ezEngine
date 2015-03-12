module ez.Foundation.Basics;
public import ez.Foundation.Memory.AllocatorBase;
public import ez.Foundation.Types.Types;

void move(T)(ref T source, ref T target)
{
  import core.stdc.string : memcpy;
  import std.traits;
  import std.algorithm : endsWith;

  // Does it have a swap method?
  static if(is(typeof(T.swap)))
  {
    source.swap(target);
  }
  else
  {
    static if (hasAliasing!T) if (!__ctfe)
    {
      import std.exception : doesPointTo;
      assert(!doesPointTo(source, source), "Cannot move object with internal pointer.");
    }

    static if (is(T == struct))
    {
      if (&source == &target) return;
      // Most complicated case. Destroy whatever target had in it
      // and bitblast source over it
      static if (hasElaborateDestructor!T) typeid(T).destroy(&target);

      static if (hasElaborateAssign!T || !isAssignable!T)
        memcpy(&target, &source, T.sizeof);
      else
        target = source;

      // If the source defines a destructor or a postblit hook, we must obliterate the
      // object in order to avoid double freeing and undue aliasing
      static if (hasElaborateDestructor!T || hasElaborateCopyConstructor!T)
      {
        __gshared immutable T empty = T.init;
        static if (T.tupleof.length > 0 &&
                   T.tupleof[$-1].stringof.endsWith("this"))
        {
          // If T is nested struct, keep original context pointer
          memcpy(&source, &empty, T.sizeof - (void*).sizeof);
        }
        else
        {
          memcpy(&source, &empty, T.sizeof);
        }
      }
    }
    else
    {
      // Primitive data (including pointers and arrays) or class -
      // assignment works great
      target = source;
    }
  }
}

T move(T)(ref T source)
{
  import core.stdc.string : memcpy;
  import std.traits;
  import std.algorithm : endsWith;

  // Does it have a swap method?
  static if(is(typeof(T.swap)))
  {
    T result = T.init;
    source.swap(result);
    return result;
  }
  else
  {
    static if (hasAliasing!T) if (!__ctfe)
    {
      import std.exception : doesPointTo;
      assert(!doesPointTo(source, source), "Cannot move object with internal pointer.");
    }

    T result = void;
    static if (is(T == struct))
    {
      // Can avoid destructing result.
      static if (hasElaborateAssign!T || !isAssignable!T)
        memcpy(&result, &source, T.sizeof);
      else
        result = source;

      // If the source defines a destructor or a postblit hook, we must obliterate the
      // object in order to avoid double freeing and undue aliasing
      static if (hasElaborateDestructor!T || hasElaborateCopyConstructor!T)
      {
        __gshared immutable T empty = T.init;
        static if (T.tupleof.length > 0 &&
                   T.tupleof[$-1].stringof.endsWith("this"))
        {
          // If T is nested struct, keep original context pointer
          memcpy(&source, &empty, T.sizeof - (void*).sizeof);
        }
        else
        {
          memcpy(&source, &empty, T.sizeof);
        }
      }
    }
    else
    {
      // Primitive data (including pointers and arrays) or class -
      // assignment works great
      result = source;
    }
    return result;
  }
}

struct DefaultCtor {};
enum defaultCtor = DefaultCtor();

extern(C++):

class ezFoundation
{
public:
  static ezAllocatorBase GetDefaultAllocator();
}