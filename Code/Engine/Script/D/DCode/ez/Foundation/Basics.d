module ez.Foundation.Basics;
import ez.Foundation.Memory.AllocatorBase;

extern(C++):

class ezFoundation
{
public:
  static ezAllocatorBase GetDefaultAllocator();
}