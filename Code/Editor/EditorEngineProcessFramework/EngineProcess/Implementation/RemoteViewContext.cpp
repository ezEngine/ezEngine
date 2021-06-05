#include <EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/EngineProcess/RemoteViewContext.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

ezUInt32 ezRemoteEngineProcessViewContext::s_uiActiveViewID = 0;
ezRemoteEngineProcessViewContext* ezRemoteEngineProcessViewContext::s_pActiveRemoteViewContext = nullptr;

ezRemoteEngineProcessViewContext::ezRemoteEngineProcessViewContext(ezEngineProcessDocumentContext* pContext)
  : ezEngineProcessViewContext(pContext)
{
}

ezRemoteEngineProcessViewContext::~ezRemoteEngineProcessViewContext()
{
  if (s_pActiveRemoteViewContext == this)
  {
    s_pActiveRemoteViewContext = nullptr;

    ezView* pView = nullptr;
    if (ezRenderWorld::TryGetView(m_hView, pView))
    {
      pView->SetWorld(nullptr);
    }
  }

  // make sure the base class destructor doesn't destroy the view
  m_hView.Invalidate();
}

void ezRemoteEngineProcessViewContext::HandleViewMessage(const ezEditorEngineViewMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezActivateRemoteViewMsgToEngine>())
  {
    if (m_hView.IsInvalidated())
    {
      m_hView = ezEditorEngineProcessApp::GetSingleton()->CreateRemoteWindowAndView(&m_Camera);
    }

    s_pActiveRemoteViewContext = this;

    ezView* pView = nullptr;
    if (ezRenderWorld::TryGetView(m_hView, pView))
    {
      ezEngineProcessDocumentContext* pDocumentContext = GetDocumentContext();
      pView->SetWorld(pDocumentContext->GetWorld());
      pView->SetCamera(&m_Camera);

      s_uiActiveViewID = pMsg->m_uiViewID;
    }
  }

  // ignore all messages for views that are currently not activated
  if (pMsg->m_uiViewID != s_uiActiveViewID)
    return;

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezViewRedrawMsgToEngine>())
  {
    const ezViewRedrawMsgToEngine* pMsg2 = static_cast<const ezViewRedrawMsgToEngine*>(pMsg);
    SetCamera(pMsg2);

    // skip the on-message redraw, in remote mode it will just render as fast as it can
    // Redraw(false);
  }
}

ezViewHandle ezRemoteEngineProcessViewContext::CreateView()
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return ezViewHandle();
}
