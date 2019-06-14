#include <GameEnginePCH.h>

#include <GameEngine/Actors/Common/ActorDeviceRenderOutputGAL.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezActorDeviceRenderOutputGAL, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezActorDeviceRenderOutputGAL::ezActorDeviceRenderOutputGAL(ezWindowOutputTargetGAL* pOutputTarget)
{
  m_pOutputTarget = pOutputTarget;
}

ezActorDeviceRenderOutputGAL::~ezActorDeviceRenderOutputGAL() = default;

ezWindowOutputTargetGAL* ezActorDeviceRenderOutputGAL::GetWindowOutputTarget() const
{
  return m_pOutputTarget;
}

void ezActorDeviceRenderOutputGAL::Present()
{
  if (m_pOutputTarget)
  {
    m_pOutputTarget->Present(m_bVSync);
  }
}
