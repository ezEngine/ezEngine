#include <Foundation/PCH.h>
#include <Foundation/Memory/MemoryUtils.h>

void ezMemoryUtils::ReserveLower4GBAddressSpace()
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS) && EZ_ENABLED(EZ_PLATFORM_64BIT)
  // The following code was taken from http://randomascii.wordpress.com/2012/02/14/64-bit-made-easy/
  // and adapted to our coding guidelines.

  static bool s_bReserved = false;
  if (s_bReserved)
    return;

  s_bReserved = true;

  // Start by reserving large blocks of address space, and then
  // gradually reduce the size in order to capture all of the
  // fragments. Technically we should continue down to 64 KB but
  // stopping at 1 MB is sufficient to keep most allocators out.

  const size_t LOW_MEM_LINE = 0x100000000LL;
  const size_t ONE_MB = 1024 * 1024;

  size_t uiTotalReservation = 0;
  size_t uiNumVAllocs = 0;
  size_t uiNumHeapAllocs = 0;
  
  for (size_t uiSize = 256 * ONE_MB; uiSize >= ONE_MB; uiSize /= 2)
  {
    while (true)
    {
      void* p = VirtualAlloc(0, uiSize, MEM_RESERVE, PAGE_NOACCESS);
      if (!p)
        break;

      if ((size_t)p >= LOW_MEM_LINE)
      {
        // We don't need this memory, so release it completely.
        VirtualFree(p, 0, MEM_RELEASE);
        break;
      }

      uiTotalReservation += uiSize;
      ++uiNumVAllocs;
    }
  }

  // Now repeat the same process but making heap allocations, to use up
  // the already reserved heap blocks that are below the 4 GB line.
  HANDLE heap = GetProcessHeap();
  for (size_t uiBlockSize = 64 * 1024; uiBlockSize >= 16; uiBlockSize /= 2)
  {
    while (true)
    {
      void* p = HeapAlloc(heap, 0, uiBlockSize);
      if (!p)
        break;

      if ((size_t)p >= LOW_MEM_LINE)
      {
        // We don't need this memory, so release it completely.
        HeapFree(heap, 0, p);
        break;
      }

      uiTotalReservation += uiBlockSize;
      ++uiNumHeapAllocs;
    }
  }

  // Perversely enough the CRT doesn't use the process heap. Suck up
  // the memory the CRT heap has already reserved.
  for (size_t uiBlockSize = 64 * 1024; uiBlockSize >= 16; uiBlockSize /= 2)
  {
    while (true)
    {
      void* p = malloc(uiBlockSize);
      if (!p)
        break;

      if ((size_t)p >= LOW_MEM_LINE)
      {
        // We don't need this memory, so release it completely.
        free(p);
        break;
      }

      uiTotalReservation += uiBlockSize;
      ++uiNumHeapAllocs;
    }
  }

  // Print diagnostics showing how many allocations we had to make in
  // order to reserve all of low memory, typically less than 200.
  char buffer[1000];
  sprintf_s(buffer, "Reserved %1.3f MB (%u vallocs, %u heap allocs) of low-memory.\n",
    uiTotalReservation / (1024 * 1024.0), (ezUInt32)uiNumVAllocs, (ezUInt32)uiNumHeapAllocs);
  OutputDebugStringA(buffer);
#endif
}


EZ_STATICLINK_FILE(Foundation, Foundation_Memory_Implementation_MemoryUtils);

