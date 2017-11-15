#include <PCH.h>
#include <EditorFramework/EditTools/EditTool.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezGameObjectEditTool, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezGameObjectEditTool::ezGameObjectEditTool()
{
}

void ezGameObjectEditTool::ConfigureTool(ezGameObjectDocument* pDocument, ezQtGameObjectDocumentWindow* pWindow, ezGameObjectGizmoInterface* pInterface)
{
  m_pDocument = pDocument;
  m_pWindow = pWindow;
  m_pInterface = pInterface;

  OnConfigured();
}

void ezGameObjectEditTool::SetActive(bool active)
{
  if (m_bIsActive == active)
    return;

  m_bIsActive = active;
}

