#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Reflection/Reflection.h>
#include <GameEngine/XR/XRSwapChain.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezGALXRSwapChain, ezGALSwapChain, 1, ezRTTINoAllocator)
{
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

ezGALXRSwapChain::Functor ezGALXRSwapChain::s_Factory;

ezGALXRSwapChain::ezGALXRSwapChain(ezXRInterface* pXrInterface)
  : ezGALSwapChain(ezGetStaticRTTI<ezGALXRSwapChain>())
  , m_pXrInterface(pXrInterface)
{
}

ezResult ezGALXRSwapChain::UpdateSwapChain(ezGALDevice* pDevice, ezEnum<ezGALPresentMode> newPresentMode)
{
  return EZ_FAILURE;
}

void ezGALXRSwapChain::SetFactoryMethod(Functor factory)
{
  s_Factory = factory;
}

ezGALSwapChainHandle ezGALXRSwapChain::Create(ezXRInterface* pXrInterface)
{
  EZ_ASSERT_DEV(s_Factory.IsValid(), "No factory method assigned for ezGALXRSwapChain.");
  return s_Factory(pXrInterface);
}


EZ_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_XRSwapChain);

