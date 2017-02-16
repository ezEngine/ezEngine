
#include <PCH.h>
#include <Foundation/Basics.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcessingStreamProcessor, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezProcessingStreamProcessor::ezProcessingStreamProcessor()
  : m_pStreamGroup(nullptr)
{

}

ezProcessingStreamProcessor::~ezProcessingStreamProcessor()
{
  m_pStreamGroup = nullptr;
}



EZ_STATICLINK_FILE(Foundation, Foundation_DataProcessing_Stream_Implementation_ProcessingStreamProcessor);

