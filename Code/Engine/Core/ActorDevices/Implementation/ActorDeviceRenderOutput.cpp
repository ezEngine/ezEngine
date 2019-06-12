#include <CorePCH.h>

#include <Core/ActorDevices/ActorDeviceRenderOutput.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezActorDeviceRenderOutput, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezActorDeviceRenderOutput::ezActorDeviceRenderOutput() = default;
ezActorDeviceRenderOutput::~ezActorDeviceRenderOutput() = default;

void ezActorDeviceRenderOutput::SetVSync(bool enable)
{
  m_bVSync = enable;
}

bool ezActorDeviceRenderOutput::GetVSync() const
{
  return m_bVSync;
}
