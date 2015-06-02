module ez.Script.Hook.Hook;

import core.sys.windows.dll;
import core.sys.windows.windows;
import core.sys.windows.dllfixup;
import core.sync.mutex;

import ez.Script.Reflection.Reflection;
import ez.Foundation.Containers.HashTable;



private:

alias void delegate(Object) DEvent;
extern (C) void rt_attachDisposeEvent(Object h, DEvent e);
extern (C) void rt_detachDisposeEvent(Object h, DEvent e);

alias TypeMap = ezHashTable!(void*, const(ClassTypeImpl)*);
alias AllocationMap = ezHashTable!(void*, const(ClassTypeImpl)*);

__gshared void[__traits(classInstanceSize, Mutex)] g_mutexMem;
__gshared Mutex g_mutex;
__gshared AllocationMap* g_allocations;
__gshared TypeMap* g_types;

extern(C)
{
  alias _d_newclass_t = Object function(const ClassInfo ci);
}

__gshared _d_newclass_t _d_newclass_org;

extern (C) Object _d_newclass(const ClassInfo ci)
{
  Object result = _d_newclass_org(ci);
  const(ClassTypeImpl)* reflectedType;
  if(g_types.TryGetValue(cast(void*)ci, reflectedType))
  {
    (*g_allocations)[cast(void*)result] = reflectedType;
    rt_attachDisposeEvent(result, &g_deleteHelper.DeleteClass);
  }
  return result;
}

struct DeleteHelper
{
  void DeleteClass(Object o)
  {
    g_allocations.Remove(cast(void*)o);
  }
}

DeleteHelper g_deleteHelper;

extern(C) void _ez_script_register_class_type(const(ClassInfo) c, ClassTypeImpl* info)
{
  (*g_types)[cast(void*)c] = info;
}

bool init()
{
  g_mutexMem = (cast(void[])typeid(Mutex).init)[];
  g_mutex = cast(Mutex)g_mutexMem.ptr;
  g_mutex.__ctor();

  debug
  {
    auto druntime = LoadLibraryA("druntime64ds.dll");
  }
  else
  {
    auto druntime = LoadLibraryA("druntime64s.dll");
  }
  if(druntime is null)
    return false;
  
  _d_newclass_org = cast(_d_newclass_t)GetProcAddress(druntime, "_d_newclass");
  if(_d_newclass_org is null)
    return false;

  return true;
}

void deinit()
{
  
  g_mutex.__dtor();
}

public:

extern(Windows)
bool DllMain(HINSTANCE hInstance, uint ulReason, void* reserved)
{
  import core.sys.windows.windows;
  import core.sys.windows.dllfixup;
  switch(ulReason)
  {
    default: assert(0);
    case DLL_PROCESS_ATTACH:
      _d_dll_fixup(hInstance);
      auto res = dll_process_attach( hInstance, false );
      if(res)
      {
        if(!init())
          return false;
      }
      return res;

    case DLL_PROCESS_DETACH:
      deinit();
      dll_process_detach( hInstance, false );
      return true;

    case DLL_THREAD_ATTACH:
        return true;

    case DLL_THREAD_DETACH:
        return true;
  }
}
