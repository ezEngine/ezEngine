#include <PCH.h>

#include <Foundation/Basics.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcessingStreamProcessor, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezProcessingStreamProcessor::ezProcessingStreamProcessor()
    : m_pStreamGroup(nullptr)
{
}

ezProcessingStreamProcessor::~ezProcessingStreamProcessor()
{
  m_pStreamGroup = nullptr;
}



EZ_STATICLINK_FILE(Foundation, Foundation_DataProcessing_Stream_Implementation_ProcessingStreamProcessor);
