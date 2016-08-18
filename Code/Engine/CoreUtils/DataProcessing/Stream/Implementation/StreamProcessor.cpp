
#include <CoreUtils/PCH.h>
#include <CoreUtils/Basics.h>
#include <CoreUtils/DataProcessing/Stream/StreamProcessor.h>
#include <CoreUtils/DataProcessing/Stream/StreamGroup.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStreamProcessor, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezStreamProcessor::ezStreamProcessor()
  : m_pStreamGroup(nullptr)
{

}

ezStreamProcessor::~ezStreamProcessor()
{
  m_pStreamGroup = nullptr;
}
