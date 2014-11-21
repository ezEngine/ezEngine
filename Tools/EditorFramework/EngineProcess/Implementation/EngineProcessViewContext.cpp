#include <PCH.h>
#include <EditorFramework/EngineProcess/EngineProcessViewContext.h>

ezHashTable<ezUInt32, ezEngineProcessViewContext*> ezEngineProcessViewContext::s_ViewContexts;

ezEngineProcessViewContext* ezEngineProcessViewContext::GetViewContext(ezUInt32 uiViewID)
{
  ezEngineProcessViewContext* pResult = nullptr;
  s_ViewContexts.TryGetValue(uiViewID, pResult);
  return pResult;
}

void ezEngineProcessViewContext::AddViewContext(ezUInt32 uiViewID, ezEngineProcessViewContext* pView)
{
  EZ_ASSERT(!s_ViewContexts.Contains(uiViewID), "Cannot add a view with an index that already exists");
  s_ViewContexts[uiViewID] = pView;
}

void ezEngineProcessViewContext::DestroyViewContext(ezUInt32 uiViewID)
{
  ezEngineProcessViewContext* pResult = nullptr;
  if (s_ViewContexts.Remove(uiViewID, &pResult))
  {
    EZ_DEFAULT_DELETE(pResult);
  }
}


