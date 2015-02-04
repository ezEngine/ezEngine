module ez.Foundation.Memory.AllocatorBase;

import ez.Foundation.Types.Types;
import std.traits;
import core.stdc.stdlib;

extern(C++) class ezAllocatorBase
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
  abstract void* Allocate(size_t uiSize, size_t uiAlign);
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
        //TODO: implement
        //AllocatorDelete(allocator, result);
      }
      result.__ctor(args);
    }
    else
    {
      static assert(args.length == 0 && !is(typeof(&T.__ctor)),
                    "Don't know how to initialize an object of type "
                    ~ T.stringof ~ " with arguments:\n" ~ ARGS.stringof ~ "\nAvailable ctors:\n" ~ ListAvailableCtors!T() );
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

auto NewArray(T)(ezAllocatorBase allocator, size_t size, InitializeMemoryWith init = InitializeMemoryWith.INIT)
{
  if(size == 0)
    return cast(T[])[];
  size_t memSize = T.sizeof * size;
  void* mem = allocator.Allocate(memSize, T.alignof).ptr;

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
