#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>

ezQtMenuActionMapView::ezQtMenuActionMapView(QWidget* pParent)
{
  setToolTipsVisible(true);
}

ezQtMenuActionMapView::~ezQtMenuActionMapView()
{
  ClearView();
}

void ezQtMenuActionMapView::SetActionContext(const ezActionContext& context)
{
  auto pMap = ezActionMapManager::GetActionMap(context.m_sMapping);

  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping '{0}' does not exist", context.m_sMapping);

  m_pActionMap = pMap;
  m_Context = context;

  CreateView();
}

void ezQtMenuActionMapView::ClearView()
{
  m_Proxies.Clear();
}

void ezQtMenuActionMapView::AddDocumentObjectToMenu(ezHashTable<ezUuid, QSharedPointer<ezQtProxy>>& ref_proxies, ezActionContext& ref_context,
  ezActionMap* pActionMap, QMenu* pCurrentRoot, const ezActionMap::TreeNode* pObject)
{
  if (pObject == nullptr)
    return;

  for (auto pChild : pObject->GetChildren())
  {
    auto pDesc = pActionMap->GetDescriptor(pChild);
    QSharedPointer<ezQtProxy> pProxy = ezQtProxy::GetProxy(ref_context, pDesc->m_hAction);
    ref_proxies[pChild->GetGuid()] = pProxy;

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

        AddDocumentObjectToMenu(ref_proxies, ref_context, pActionMap, pCurrentRoot, pChild);

        pCurrentRoot->addSeparator();
      }
      break;

      case ezActionType::Menu:
      {
        QMenu* pQtMenu = static_cast<ezQtMenuProxy*>(pProxy.data())->GetQMenu();
        pCurrentRoot->addMenu(pQtMenu);
        AddDocumentObjectToMenu(ref_proxies, ref_context, pActionMap, pQtMenu, pChild);
      }
      break;

      case ezActionType::ActionAndMenu:
      {
        QAction* pQtAction = static_cast<ezQtDynamicActionAndMenuProxy*>(pProxy.data())->GetQAction();
        QMenu* pQtMenu = static_cast<ezQtDynamicActionAndMenuProxy*>(pProxy.data())->GetQMenu();
        pCurrentRoot->addAction(pQtAction);
        pCurrentRoot->addMenu(pQtMenu);
        AddDocumentObjectToMenu(ref_proxies, ref_context, pActionMap, pQtMenu, pChild);
      }
      break;
    }
  }
}

void ezQtMenuActionMapView::CreateView()
{
  ClearView();

  auto pObject = m_pActionMap->BuildActionTree();

  AddDocumentObjectToMenu(m_Proxies, m_Context, m_pActionMap, this, pObject);
}
