#include <EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/Actors/EditorWnd/ActorDeviceRenderOutputEditorWnd.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezActorDeviceRenderOutputEditorWnd, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezActorDeviceRenderOutputEditorWnd::ezActorDeviceRenderOutputEditorWnd(ezWindowOutputTargetGAL* pOutputTarget)
{
  m_pOutputTarget = pOutputTarget;
}

ezActorDeviceRenderOutputEditorWnd::~ezActorDeviceRenderOutputEditorWnd() = default;

ezWindowOutputTargetGAL* ezActorDeviceRenderOutputEditorWnd::GetWindowOutputTarget() const
{
  return m_pOutputTarget;
}

void ezActorDeviceRenderOutputEditorWnd::Present()
{
  if (m_pOutputTarget)
  {
    m_pOutputTarget->Present(m_bVSync);
  }
}
