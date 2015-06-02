module ez.Foundation.Memory.AllocatorBase;

import ez.Foundation.Types.Types;
import std.traits;
import core.stdc.stdlib;

private extern (C) void rt_finalize2(void* p, bool det = true, bool resetMemory = true);

export extern(C++) class ezAllocatorBase
{
protected:
  abstract void __dtor(); // destructor member to match c++ vtable layout

public:
  static struct Stats
  {
    ezUInt64 m_uiNumAllocations;      ///< total number of allocations
    ezUInt64 m_uiNumDeallocations;    ///< total number of deallocations
    ezUInt64 m_uiAllocationSize;      ///< total allocation size in bytes
  };

  //ezAllocatorBase();
  //~ezAllocatorBase();

  /// \brief Interface, do not use this directly, always use the new/delete macros below
  abstract void* Allocate(size_t uiSize, size_t uiAlign, void* destructorFunc = null); // TODO implement destructorFunc
  abstract void Deallocate(void* ptr);
  abstract void* Reallocate(void* ptr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign);
  abstract size_t AllocatedSize(const void* ptr);

  abstract Stats GetStats() const;
};

enum InitializeMemoryWith
{
  Nothing,
  Null,
  Init
}

T New(T, ARGS...)(ezAllocatorBase allocator, ARGS args)
{
  import core.stdc.string : memset, memcpy;
  static if(is(T == class))
  {
    size_t memSize = __traits(classInstanceSize,T);
    static assert(!__traits(compiles, { T temp; bool test = temp.outer !is null; }), "inner classes are not implemented yet");
  } 
  else {
    size_t memSize = T.sizeof;
  }

  void[] mem = allocator.Allocate(memSize, T.alignof)[0..memSize];
  debug {
    assert(mem.ptr !is null,"Out of memory");
    auto address = cast(size_t)mem.ptr;
    auto alignment = T.alignof;
    assert(address % alignment == 0,"Missaligned memory");  
  }

  //initialize
  static if(is(T == class))
  {
    auto ti = typeid(Unqual!T);
    assert(memSize == ti.init.length,"classInstanceSize and typeid(T).init.length do not match");
    mem[] = (cast(void[])ti.init)[];
    auto result = (cast(T)mem.ptr);
    static if(is(typeof(result.__ctor(args))))
    {
      scope(failure)
      {
        Delete(allocator, result);
      }
      result.__ctor(args);
    }
    else
    {
      static assert(args.length == 0 && !is(typeof(&T.__ctor)),
                    "Don't know how to initialize an object of type "
                    ~ T.stringof ~ " with arguments:\n" ~ ARGS.stringof); // TODO: ~ "\nAvailable ctors:\n" ~ ListAvailableCtors!T() );
    }

    static if(__traits(hasMember, T, "SetAllocator"))
    {
      result.SetAllocator(allocator);
    }

    return result;
  }
  else
  {
    auto ti = typeid(Unqual!T);
    assert(memSize == ti.init().length, "T.sizeof and typeid(T).init.length do not match");
    if(ti.init().ptr is null)
      memset(mem.ptr, 0, mem.length);
    else
      mem[] = (cast(void[])ti.init())[];
    auto result = (cast(T*)mem);
    static if(ARGS.length > 0 && is(typeof(result.__ctor(args))))
    {
      result.__ctor(args);
    }
    else static if(ARGS.length > 0)
    {
      static assert(args.length == 0 && !is(typeof(&T.__ctor)),
                    "Don't know how to initialize an object of type "
                    ~ T.stringof ~ " with arguments " ~ args.stringof ~ "\nAvailable ctors:\n" ~ ListAvailableCtors!T() );
    }
    return result;
  }
}

auto NewArray(T)(ezAllocatorBase allocator, size_t size, InitializeMemoryWith init = InitializeMemoryWith.Init)
{
  import core.stdc.string : memset, memcpy;
  if(size == 0)
    return cast(T[])[];
  size_t memSize = T.sizeof * size;
  void* mem = allocator.Allocate(memSize, T.alignof);

  T[] data = (cast(T*)mem)[0..size];
  final switch(init)
  {
    case InitializeMemoryWith.Nothing:
      break;
    case InitializeMemoryWith.Init:
      static if(is(T == struct))
      {
        const(void[]) initMem = typeid(T).init();
        if(initMem.ptr is null)
        {
          memset(data.ptr, 0, data.length * T.sizeof);
        }
        else
        {
          foreach(ref e;data)
          {
            memcpy(&e, initMem.ptr, initMem.length);
          }
        }
      }
      else static if(!is(T == void))
      {
        foreach(ref e; data)
        {
          e = T.init;
        }
      }
      break;
    case InitializeMemoryWith.Null:
      memset(mem, 0, memSize);
      break;
  }
  return data;
}

void Delete(T)(ezAllocatorBase allocator, T obj)
{
  /*static if(is(T : RefCounted))
  {
    assert(obj.refcount == 0, "trying to delete reference counted object which is still referenced");
  }*/
  static if(is(T == class))
  {
    if(obj is null)
      return;
    rt_finalize2(cast(void*)obj, false, false);
    allocator.Deallocate(cast(void*)obj);
  }
  else static if(is(T == interface))
  {
    if(obj is null)
      return;
    Object realObj = cast(Object)obj;
    if(realObj is null)
      return;
    rt_finalize2(cast(void*)realObj, false, false);
    allocator.Deallocate(cast(void*)realObj);
  }
  else static if(is(T P == U*, U))
  {
    if(obj is null)
      return;
    callDtor(obj);
    allocator.Deallocate(cast(void*)obj);
  }
  else static if(is(T P == U[], U))
  {
    if(!obj)
      return;
    callDtor(obj);
    allocator.FreeMemory(cast(void*)obj.ptr);    
  }
  else
  {
    static assert(0, "Can not delete " ~ T.stringof);
  }
}

void callDtor(T)(T subject)
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
        typeinfo.xdtor(subject);
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