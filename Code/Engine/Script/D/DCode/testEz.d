import ez.Foundation.Basics;
import ez.Foundation.Memory.AllocatorBase;

void main(string[] args)
{
  auto defaultAllocator = ezFoundation.GetDefaultAllocator();
  auto mem = defaultAllocator.Allocate(16, 4);
  assert(mem !is null);
  defaultAllocator.Deallocate(mem);
}