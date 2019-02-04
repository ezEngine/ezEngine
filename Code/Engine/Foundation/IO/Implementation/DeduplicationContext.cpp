#include <PCH.h>

#include <Foundation/IO/DeduplicationReadContext.h>
#include <Foundation/IO/DeduplicationWriteContext.h>

EZ_IMPLEMENT_SERIALIZATION_CONTEXT(ezDeduplicationReadContext);

ezDeduplicationReadContext::ezDeduplicationReadContext() = default;
ezDeduplicationReadContext::~ezDeduplicationReadContext() = default;

//////////////////////////////////////////////////////////////////////////

EZ_IMPLEMENT_SERIALIZATION_CONTEXT(ezDeduplicationWriteContext);

ezDeduplicationWriteContext::ezDeduplicationWriteContext() = default;
ezDeduplicationWriteContext::~ezDeduplicationWriteContext() = default;



EZ_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_DeduplicationContext);

