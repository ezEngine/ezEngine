#include <GuiFoundation/PCH.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>

ezMenuActionMapView::ezMenuActionMapView(QWidget* parent, QWidget* pActionParent)
{
  m_pActionParent = pActionParent != nullptr ? pActionParent : parent;
  EZ_ASSERT_DEV(m_pActionParent != nullptr, "Either parent or pActionParent needs to be set in the action view!");
}

ezMenuActionMapView::~ezMenuActionMapView()
{
  ClearView();
}

void ezMenuActionMapView::SetActionContext(const ezActionContext& context)
{
  auto pMap = ezActionMapManager::GetActionMap(context.m_sMapping);

   EZ_ASSERT_DEV(pMap != nullptr, "The given mapping '%s' does not exist", context.m_sMapping.GetData());

  m_pActionMap = pMap;
  m_Context = context;

  CreateView();
}

void ezMenuActionMapView::ClearView()
{
  m_Proxies.Clear();
}

void ezMenuActionMapView::AddDocumentObjectToMenu(ezHashTable<ezUuid, QSharedPointer<ezQtProxy>>& Proxies, ezActionContext& Context, ezActionMap* pActionMap, QMenu* pCurrentRoot, ezDocumentObjectBase* pObject, QWidget* pActionParent)
{
  if (pObject == nullptr)
    return;

  for (auto pChild : pObject->GetChildren())
  {
    auto pDesc = pActionMap->GetDescriptor(pChild);
    QSharedPointer<ezQtProxy> pProxy = ezQtProxy::GetProxy(Context, pDesc->m_hAction);
    Proxies[pChild->GetGuid()] = pProxy;

    switch (pDesc->m_hAction.GetDescriptor()->m_Type)
    {
    case ezActionType::Action:
      {
        QAction* pQtAction = static_cast<ezQtActionProxy*>(pProxy.data())->GetQAction();
        pCurrentRoot->addAction(pQtAction);
        if (pDesc->m_hAction.GetDescriptor()->m_Scope == ezActionScope::Window)
          pQtAction->setParent(pActionParent);
      }
      break;

    case ezActionType::Category:
      {
        pCurrentRoot->addSeparator();

        AddDocumentObjectToMenu(Proxies, Context, pActionMap, pCurrentRoot, pChild, pActionParent);

        pCurrentRoot->addSeparator();
      }
      break;

    case ezActionType::Menu:
      {
        QMenu* pQtMenu = static_cast<ezQtMenuProxy*>(pProxy.data())->GetQMenu();
        pCurrentRoot->addMenu(pQtMenu);
        AddDocumentObjectToMenu(Proxies, Context, pActionMap, pQtMenu, pChild, pActionParent);
      }
      break;
    }
  }
}

void ezMenuActionMapView::CreateView()
{
  ClearView();

  auto pObject = m_pActionMap->GetRootObject();

  AddDocumentObjectToMenu(m_Proxies, m_Context, m_pActionMap, this, pObject, m_pActionParent);
}
