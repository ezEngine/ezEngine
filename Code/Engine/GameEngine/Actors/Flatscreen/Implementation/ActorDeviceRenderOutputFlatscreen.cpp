#include <GameEnginePCH.h>

#include <GameEngine/Actors/Flatscreen/ActorDeviceRenderOutputFlatscreen.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezActorDeviceRenderOutputFlatscreen, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezActorDeviceRenderOutputFlatscreen::ezActorDeviceRenderOutputFlatscreen(ezWindowOutputTargetGAL* pOutputTarget)
{
  m_pOutputTarget = pOutputTarget;
}

ezActorDeviceRenderOutputFlatscreen::~ezActorDeviceRenderOutputFlatscreen() = default;

ezWindowOutputTargetGAL* ezActorDeviceRenderOutputFlatscreen::GetWindowOutputTarget() const
{
  return m_pOutputTarget;
}

void ezActorDeviceRenderOutputFlatscreen::Present()
{
  if (m_pOutputTarget)
  {
    m_pOutputTarget->Present(m_bVSync);
  }
}
