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
}

ezResult ezMenuActionMapView::SetActionContext(const ezActionContext& context)
{
  auto pMap = ezActionMapManager::GetActionMap(context.m_sMapping);

  if (pMap == nullptr)
    return EZ_FAILURE;

  m_pActionMap = pMap;
  m_Context = context;

  CreateView();

  return EZ_SUCCESS;
}

void ezMenuActionMapView::ClearView()
{
}

void ezMenuActionMapView::AddDocumentObjectToMenu(QMenu* pCurrentRoot, ezDocumentObjectBase* pObject)
{
  if (pObject == nullptr)
    return;

  for (auto pChild : pObject->GetChildren())
  {
    auto pDesc = m_pActionMap->GetDescriptor(pChild);
    auto pAction = pDesc->m_hAction.GetDescriptor()->CreateAction(m_Context);

    ezQtProxy* pProxy = ezRttiMappedObjectFactory<ezQtProxy>::CreateObject(pAction->GetDynamicRTTI());
    m_Proxies[pChild->GetGuid()] = pProxy;

    pProxy->SetAction(pAction);

    switch (pDesc->m_hAction.GetDescriptor()->m_Type)
    {
    case ezActionType::Action:
      {
        QAction* pQtAction = static_cast<ezQtActionProxy*>(pProxy)->GetQAction();
        pCurrentRoot->addAction(pQtAction);
      }
      break;

    case ezActionType::Category:
      EZ_ASSERT_NOT_IMPLEMENTED;
      break;

    case ezActionType::Menu:
      {
        QMenu* pQtMenu = static_cast<ezQtMenuProxy*>(pProxy)->GetQMenu();
        pCurrentRoot->addMenu(pQtMenu);

        AddDocumentObjectToMenu(pQtMenu, pChild);
      }
      break;
    }
  }
}

void ezMenuActionMapView::CreateView()
{
  ClearView();

  auto pObject = m_pActionMap->GetRootObject();

  AddDocumentObjectToMenu(this, pObject);
}
