
#include <CoreUtils/PCH.h>
#include <CoreUtils/Basics.h>
#include <CoreUtils/DataProcessing/Stream/StreamProcessor.h>
#include <CoreUtils/DataProcessing/Stream/StreamGroup.h>

ezStreamProcessor::ezStreamProcessor()
  : m_pStreamGroup(nullptr)
{

}

ezStreamProcessor::~ezStreamProcessor()
{
  m_pStreamGroup = nullptr;
}
