
#include <CoreUtils/PCH.h>
#include <CoreUtils/Basics.h>
#include <CoreUtils/DataProcessing/Stream/ProcessingStreamSpawner.h>
#include <CoreUtils/DataProcessing/Stream/ProcessingStreamGroup.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezProcessingStreamSpawner, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezProcessingStreamSpawner::ezProcessingStreamSpawner()
  : m_pStreamGroup(nullptr)
{
}

ezProcessingStreamSpawner::~ezProcessingStreamSpawner()
{
}