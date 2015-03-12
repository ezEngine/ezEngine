module ez.Foundation.Memory.MemoryUtils;
import std.traits;
import core.stdc.string : memcpy;
import std.string : endsWith;

struct ezMemoryUtils
{
  static void Destruct(T)(T obj)
  {
    static if(is(T U == V[], V))
    {
      static if(is(V == struct))
      {
        auto typeinfo = typeid(V);
        if(typeinfo.xdtor !is null)
        {
          foreach(ref el; subject)
          {
            typeinfo.xdtor(&el);
          }
        }
        //TODO: structs are currently only destroyable over a typeinfo object, fix
        /*static if(is(typeof(subject[0].__fieldDtor)))
        {
        foreach(ref el; subject)
        el.__fieldDtor();
        }
        else static if(is(typeof(subject[0].__dtor)))
        {
        foreach(ref el; subject)
        el.__dtor();
        }*/
      }
    }
    else static if(is(T P == U*, U))
    {
      static if(is(U == struct))
      {
        auto typeinfo = typeid(U);
        if(typeinfo.xdtor !is null)
        {
          typeinfo.xdtor(obj);
        }
        //TODO: structs are currently only destroyable over a type info object, fix
        /*static if(is(typeof(subject.__fieldDtor)))
        {
        subject.__fieldDtor();
        }
        else static if(is(typeof(subject.__dtor)))
        {
        subject.__dtor();
        }*/
      }
    }
    else
    {
      static if(is(T == struct))
      {
        static assert(0, "can not destruct copy");
      }
    }
  }

  static void ZeroFill(T)(T* pDestination, size_t uiCount = 1)
  {
    import core.stdc.string : memset;
    memset(pDestination, 0, uiCount * T.sizeof);
  }

  static void MoveConstruct(T)(T* target, ref T source)
  {
    if(is(T == struct))
    {
      // Does it have a swap method?
      static if(is(typeof(T.swap)))
      {
        __gshared immutable T empty;
        memcpy(target, &empty, T.sizeof);
        target.swap(source);
      }
      else
      {
        // Can avoid destructing result.
        static if (hasElaborateAssign!T || !isAssignable!T)
          memcpy(&target, &source, T.sizeof);
        else
          *target = source;

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
    }
    else
    {
      *target = source;
    }
  }
}