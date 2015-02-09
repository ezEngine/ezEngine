module ez.Script.DGlue.Allocator;
import ez.Foundation.Memory.AllocatorBase;

extern(C++)
{
  class ezScriptReflectionAllocator : ezAllocatorBase
  {
  public:
    final void Reset();
  };

  ezScriptReflectionAllocator ezGetDefaultScriptReflectionAllocator();
  private void ezInitDefaultScriptReflectionAllocator();
  private void ezDeinitDefaultScriptReflectionAllocator();
}

shared static this()
{
  ezInitDefaultScriptReflectionAllocator();
}

shared static ~this()
{
  ezDeinitDefaultScriptReflectionAllocator();
}