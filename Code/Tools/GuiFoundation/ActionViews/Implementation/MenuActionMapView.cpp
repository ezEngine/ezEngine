#include <GuiFoundation/PCH.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>

ezMenuActionMapView::ezMenuActionMapView(QWidget* parent)
{
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
  for (auto it = m_Proxies.GetIterator(); it.IsValid(); ++it)
  {
    ezQtProxy* pProxy = it.Value();
    pProxy->deleteLater();
  }
  m_Proxies.Clear();
}

void ezMenuActionMapView::AddDocumentObjectToMenu(ezHashTable<ezUuid, ezQtProxy*>& Proxies, ezActionContext& Context, ezActionMap* pActionMap, QMenu* pCurrentRoot, ezDocumentObjectBase* pObject)
{
  if (pObject == nullptr)
    return;

  for (auto pChild : pObject->GetChildren())
  {
    auto pDesc = pActionMap->GetDescriptor(pChild);
    auto pAction = pDesc->m_hAction.GetDescriptor()->CreateAction(Context);

    ezQtProxy* pProxy = ezRttiMappedObjectFactory<ezQtProxy>::CreateObject(pAction->GetDynamicRTTI());
    EZ_ASSERT_DEBUG(pProxy != nullptr, "No proxy assigned to action '%s'", pDesc->m_hAction.GetDescriptor()->m_sActionName.GetData());

    Proxies[pChild->GetGuid()] = pProxy;
    pProxy->setParent(pCurrentRoot);
    pProxy->SetAction(pAction, true);

    switch (pDesc->m_hAction.GetDescriptor()->m_Type)
    {
    case ezActionType::Action:
      {
        QAction* pQtAction = static_cast<ezQtActionProxy*>(pProxy)->GetQAction();
        pCurrentRoot->addAction(pQtAction);
        //pQtAction->setParent(pCurrentRoot);
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
        QMenu* pQtMenu = static_cast<ezQtMenuProxy*>(pProxy)->GetQMenu();
        pCurrentRoot->addMenu(pQtMenu);
        //pQtMenu->setParent(pCurrentRoot);
        AddDocumentObjectToMenu(Proxies, Context, pActionMap, pQtMenu, pChild);
      }
      break;
    }
  }
}

void ezMenuActionMapView::CreateView()
{
  ClearView();

  auto pObject = m_pActionMap->GetRootObject();

  AddDocumentObjectToMenu(m_Proxies, m_Context, m_pActionMap, this, pObject);
}
