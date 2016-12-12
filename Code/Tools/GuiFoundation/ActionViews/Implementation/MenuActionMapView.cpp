#include <GuiFoundation/PCH.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>

ezQtMenuActionMapView::ezQtMenuActionMapView(QWidget* parent)
{
}

ezQtMenuActionMapView::~ezQtMenuActionMapView()
{
  ClearView();
}

void ezQtMenuActionMapView::SetActionContext(const ezActionContext& context)
{
  auto pMap = ezActionMapManager::GetActionMap(context.m_sMapping);

   EZ_ASSERT_DEV(pMap != nullptr, "The given mapping '{0}' does not exist", context.m_sMapping.GetData());

  m_pActionMap = pMap;
  m_Context = context;

  CreateView();
}

void ezQtMenuActionMapView::ClearView()
{
  m_Proxies.Clear();
}

void ezQtMenuActionMapView::AddDocumentObjectToMenu(ezHashTable<ezUuid, QSharedPointer<ezQtProxy>>& Proxies, ezActionContext& Context, ezActionMap* pActionMap, QMenu* pCurrentRoot, const ezActionMap::TreeNode* pObject)
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
      }
      break;

    case ezActionType::Category:
      {
        pCurrentRoot->addSeparator();

        AddDocumentObjectToMenu(Proxies, Context, pActionMap, pCurrentRoot, pChild);

        pCurrentRoot->addSeparator();
      }
      break;

    case ezActionType::Menu:
      {
        QMenu* pQtMenu = static_cast<ezQtMenuProxy*>(pProxy.data())->GetQMenu();
        pCurrentRoot->addMenu(pQtMenu);
        AddDocumentObjectToMenu(Proxies, Context, pActionMap, pQtMenu, pChild);
      }
      break;

    case ezActionType::ActionAndMenu:
      {
        QAction* pQtAction = static_cast<ezQtDynamicActionAndMenuProxy*>(pProxy.data())->GetQAction();
        QMenu* pQtMenu = static_cast<ezQtDynamicActionAndMenuProxy*>(pProxy.data())->GetQMenu();
        pCurrentRoot->addAction(pQtAction);
        pCurrentRoot->addMenu(pQtMenu);
        AddDocumentObjectToMenu(Proxies, Context, pActionMap, pQtMenu, pChild);
      }
      break;
    }
  }
}

void ezQtMenuActionMapView::CreateView()
{
  ClearView();

  auto pObject = m_pActionMap->GetRootObject();

  AddDocumentObjectToMenu(m_Proxies, m_Context, m_pActionMap, this, pObject);
}
