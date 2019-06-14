#include <EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/Actors/EditorWnd/ActorEditorWnd.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>
#include <GameEngine/Actors/Common/ActorDeviceRenderOutputGAL.h>
#include <GameEngine/GameApplication/WindowOutputTarget.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/Device.h>
#include <System/Window/Window.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezActorEditorWnd, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezActorEditorWnd::ezActorEditorWnd(const char* szActorName, const void* pCreatedBy, ezUniquePtr<ezEditorProcessViewWindow>&& pWindow)
  : ezActor(szActorName, pCreatedBy)
{
  m_pWindow = std::move(pWindow);
}

ezEditorProcessViewWindow* ezActorEditorWnd::GetWindow()
{
  return m_pWindow.Borrow();
}

void ezActorEditorWnd::Activate()
{
  SUPER::Activate();

  ezGALSwapChainCreationDescription desc;
  desc.m_pWindow = m_pWindow.Borrow();
  desc.m_BackBufferFormat = ezGALResourceFormat::RGBAUByteNormalizedsRGB;
  desc.m_bAllowScreenshots = true;
  auto hSwapChain = ezGALDevice::GetDefaultDevice()->CreateSwapChain(desc);

  m_OutputTarget = EZ_DEFAULT_NEW(ezWindowOutputTargetGAL);
  m_OutputTarget->m_hSwapChain = hSwapChain;

  AddDevice(EZ_DEFAULT_NEW(ezActorDeviceRenderOutputGAL, m_OutputTarget.Borrow()));
}

void ezActorEditorWnd::Deactivate()
{
  m_OutputTarget = nullptr;
  m_pWindow = nullptr;

  SUPER::Deactivate();
}

void ezActorEditorWnd::Update()
{
  m_pWindow->ProcessWindowMessages();
}
