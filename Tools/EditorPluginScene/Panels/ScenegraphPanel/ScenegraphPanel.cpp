#include <PCH.h>
#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphPanel.moc.h>
#include <Core/World/GameObject.h>
#include <QMenu>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/EditActions.h>
#include <Actions/SelectionActions.h>

ezScenegraphPanel::ezScenegraphPanel(QWidget* pParent, ezSceneDocument* pDocument)
  : ezDocumentPanel(pParent)
{
  setObjectName("ScenegraphPanel");
  setWindowTitle("Scenegraph");

  m_pDocument = pDocument;

  m_pTreeWidget = new ezRawDocumentTreeWidget(this, pDocument, ezGetStaticRTTI<ezGameObject>(), "Children");
  setWidget(m_pTreeWidget);

  m_pDocument->m_SceneEvents.AddEventHandler(ezMakeDelegate(&ezScenegraphPanel::DocumentSceneEventHandler, this));

  m_pTreeWidget->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

  EZ_VERIFY(connect(m_pTreeWidget, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(OnItemDoubleClicked(const QModelIndex&))) != nullptr, "signal/slot connection failed");
  EZ_VERIFY(connect(m_pTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(OnRequestContextMenu(QPoint))) != nullptr, "signal/slot connection failed");

}

ezScenegraphPanel::~ezScenegraphPanel()
{
  m_pDocument->m_SceneEvents.RemoveEventHandler(ezMakeDelegate(&ezScenegraphPanel::DocumentSceneEventHandler, this));
}

void ezScenegraphPanel::RegisterActions()
{
  ezActionMapManager::RegisterActionMap("ScenegraphContextMenu");

  ezSelectionActions::MapContextMenuActions("ScenegraphContextMenu", "");
  ezEditActions::MapContextMenuActions("ScenegraphContextMenu", "");
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

void ezScenegraphPanel::OnItemDoubleClicked(const QModelIndex&)
{
  m_pDocument->TriggerFocusOnSelection(true);
}

void ezScenegraphPanel::OnRequestContextMenu(QPoint pos)
{
  ezMenuActionMapView menu(nullptr);

  ezActionContext context;
  context.m_sMapping = "ScenegraphContextMenu";
  context.m_pDocument = m_pDocument;
  context.m_pWindow = this;
  menu.SetActionContext(context);

  menu.exec(m_pTreeWidget->mapToGlobal(pos));

}





