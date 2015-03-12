module ez.Foundation.Memory.AllocatorWrapper;
import ez.Foundation.Basics;

struct ezDefaultAllocatorWrapper
{
  export static ezAllocatorBase GetAllocator()
  {
    return ezFoundation.GetDefaultAllocator();
  }
}
