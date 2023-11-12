#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/EditTools/EditTool.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGameObjectEditTool, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezGameObjectEditTool::ezGameObjectEditTool() = default;

void ezGameObjectEditTool::ConfigureTool(
  ezGameObjectDocument* pDocument, ezQtGameObjectDocumentWindow* pWindow, ezGameObjectGizmoInterface* pInterface)
{
  m_pDocument = pDocument;
  m_pWindow = pWindow;
  m_pInterface = pInterface;

  OnConfigured();
}

void ezGameObjectEditTool::SetActive(bool bActive)
{
  if (m_bIsActive == bActive)
    return;

  m_bIsActive = bActive;
  OnActiveChanged(m_bIsActive);

  if (!m_bIsActive)
  {
    m_pWindow->SetPermanentStatusBarMsg("");
  }
}
