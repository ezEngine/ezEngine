#include <EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/Actors/EditorWnd/ActorEditorWnd.h>
#include <EditorEngineProcessFramework/Actors/EditorWnd/ActorManagerEditorWnd.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezActorManagerEditorWnd, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezActorManagerEditorWnd::ezActorManagerEditorWnd() = default;
ezActorManagerEditorWnd::~ezActorManagerEditorWnd() = default;

ezActorEditorWnd* ezActorManagerEditorWnd::CreateEditorWndActor(
  const char* szActorName, const char* szGroupName, ezWindowHandle hWnd, ezUInt16 uiWidth, ezUInt16 uiHeight)
{
  ezUniquePtr<ezEditorProcessViewWindow> pWindow = EZ_DEFAULT_NEW(ezEditorProcessViewWindow);
  pWindow->m_hWnd = hWnd;
  pWindow->m_uiWidth = uiWidth;
  pWindow->m_uiHeight = uiHeight;

  ezUniquePtr<ezActorEditorWnd> pActorEditorWnd = EZ_DEFAULT_NEW(ezActorEditorWnd, szActorName, szGroupName, std::move(pWindow));
  ezActorEditorWnd* pActorToReturn = pActorEditorWnd.Borrow();

  AddActor(std::move(pActorEditorWnd));

  return pActorToReturn;
}

void ezActorManagerEditorWnd::DestroyEditorWndActor(ezActorEditorWnd* pActor)
{
  DestroyActor(pActor);
}
