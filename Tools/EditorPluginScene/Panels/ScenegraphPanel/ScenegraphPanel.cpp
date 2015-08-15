#include <PCH.h>
#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphPanel.moc.h>
#include <Core/World/GameObject.h>
ezScenegraphPanel::ezScenegraphPanel(QWidget* pParent, ezSceneDocument* pDocument)
  : ezDocumentPanel(pParent)
{
  setObjectName("ScenegraphPanel");
  setWindowTitle("Scenegraph");

  m_pDocument = pDocument;

  m_pTreeWidget = new ezRawDocumentTreeWidget(this, pDocument, ezGetStaticRTTI<ezGameObject>(), "Children");
  setWidget(m_pTreeWidget);

  m_pDocument->m_SceneEvents.AddEventHandler(ezMakeDelegate(&ezScenegraphPanel::DocumentSceneEventHandler, this));

}

ezScenegraphPanel::~ezScenegraphPanel()
{
  m_pDocument->m_SceneEvents.RemoveEventHandler(ezMakeDelegate(&ezScenegraphPanel::DocumentSceneEventHandler, this));
}

void ezScenegraphPanel::DocumentSceneEventHandler(const ezSceneDocument::SceneEvent& e)
{
  switch (e.m_Type)
  {
  case ezSceneDocument::SceneEvent::Type::ShowSelectionInScenegraph:
    {
      m_pTreeWidget->EnsureLastSelectedItemVisible();
    }
    break;
  }

}