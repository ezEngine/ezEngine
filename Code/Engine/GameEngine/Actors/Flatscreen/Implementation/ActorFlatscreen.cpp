#include <GameEnginePCH.h>

#include <GameEngine/Actors/Flatscreen/ActorDeviceRenderOutputFlatscreen.h>
#include <GameEngine/Actors/Flatscreen/ActorFlatscreen.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/Device.h>
#include <System/Window/Window.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezActorFlatscreen, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezActorFlatscreen::ezActorFlatscreen(const char* szActorName, ezUniquePtr<ezWindow>&& pWindow)
  : ezActor(szActorName)
{
  m_pWindow = std::move(pWindow);
}

ezWindow* ezActorFlatscreen::GetWindow()
{
  return m_pWindow.Borrow();
}

void ezActorFlatscreen::Activate()
{
  SUPER::Activate();

  ezGALSwapChainCreationDescription desc;
  desc.m_pWindow = m_pWindow.Borrow();
  desc.m_BackBufferFormat = ezGALResourceFormat::RGBAUByteNormalizedsRGB;
  desc.m_bAllowScreenshots = true;
  auto hSwapChain = ezGALDevice::GetDefaultDevice()->CreateSwapChain(desc);

  m_OutputTarget = EZ_DEFAULT_NEW(ezWindowOutputTargetGAL);
  m_OutputTarget->m_hSwapChain = hSwapChain;

  //m_pRenderOutput = EZ_DEFAULT_NEW(ezActorDeviceRenderOutputFlatscreen, m_OutputTarget.Borrow());
  AddDevice(EZ_DEFAULT_NEW(ezActorDeviceRenderOutputFlatscreen, m_OutputTarget.Borrow()));
}

void ezActorFlatscreen::Deactivate()
{
  // do not try to destroy the primary swapchain, that is handled by the device
  if (ezGALDevice::GetDefaultDevice()->GetPrimarySwapChain() != m_OutputTarget->m_hSwapChain)
  {
    ezGALDevice::GetDefaultDevice()->DestroySwapChain(m_OutputTarget->m_hSwapChain);
  }

  //m_pRenderOutput = nullptr;
  m_OutputTarget = nullptr;
  m_pWindow = nullptr;

  SUPER::Deactivate();
}

