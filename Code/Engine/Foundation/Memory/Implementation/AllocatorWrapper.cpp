#include <Foundation/PCH.h>
#include <Foundation/Memory/AllocatorWrapper.h>

ezThreadLocalPointer<ezAllocatorBase> ezLocalAllocatorWrapper::m_pAllocator;


EZ_STATICLINK_FILE(Foundation, Foundation_Memory_Implementation_AllocatorWrapper);

