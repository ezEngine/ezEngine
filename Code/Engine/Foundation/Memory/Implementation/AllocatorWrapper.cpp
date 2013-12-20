#include <Foundation/PCH.h>
#include <Foundation/Memory/AllocatorWrapper.h>

ezThreadLocalPointer<ezIAllocator> ezLocalAllocatorWrapper::m_pAllocator;


EZ_STATICLINK_REFPOINT(Foundation_Memory_Implementation_AllocatorWrapper);

