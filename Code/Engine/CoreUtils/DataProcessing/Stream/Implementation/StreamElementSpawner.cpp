
#include <CoreUtils/PCH.h>
#include <CoreUtils/Basics.h>
#include <CoreUtils/DataProcessing/Stream/StreamElementSpawner.h>
#include <CoreUtils/DataProcessing/Stream/StreamGroup.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStreamElementSpawner, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezStreamElementSpawner::ezStreamElementSpawner()
  : m_pStreamGroup(nullptr)
{
}

ezStreamElementSpawner::~ezStreamElementSpawner()
{
}