
#include <CoreUtils/PCH.h>
#include <CoreUtils/Basics.h>
#include <CoreUtils/DataProcessing/Stream/StreamProcessor.h>
#include <CoreUtils/DataProcessing/Stream/StreamGroup.h>

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
