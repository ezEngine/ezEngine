#include <GameEnginePCH.h>

#include <GameEngine/Actors/Common/ActorDeviceRenderOutputGAL.h>
#include <GameEngine/Actors/Flatscreen/ActorFlatscreen.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/Device.h>
#include <System/Window/Window.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezActorFlatscreen, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezActorFlatscreen::ezActorFlatscreen(const char* szActorName, const void* pCreatedBy, ezUniquePtr<ezWindow>&& pWindow)
  : ezActor(szActorName, pCreatedBy)
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

  m_OutputTarget = EZ_DEFAULT_NEW(ezWindowOutputTargetGAL);
  m_OutputTarget->CreateSwapchain(desc);

  AddDevice(EZ_DEFAULT_NEW(ezActorDeviceRenderOutputGAL, m_OutputTarget.Borrow()));
}

void ezActorFlatscreen::Deactivate()
{
  m_OutputTarget = nullptr;
  m_pWindow = nullptr;

  SUPER::Deactivate();
}

void ezActorFlatscreen::Update()
{
  m_pWindow->ProcessWindowMessages();
}

