#include <GuiFoundation/PCH.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>

ezMenuBarActionMapView::ezMenuBarActionMapView(QWidget* parent) : QMenuBar(parent)
{
}

ezMenuBarActionMapView::~ezMenuBarActionMapView()
{
}

ezResult ezMenuBarActionMapView::SetActionContext(const ezActionContext& context)
{
  auto pMap = ezActionMapManager::GetActionMap(context.m_sMapping);

  if (pMap == nullptr)
    return EZ_FAILURE;

  m_pActionMap = pMap;
  m_Context = context;

  CreateView();

  return EZ_SUCCESS;
}

void ezMenuBarActionMapView::ClearView()
{
}

void ezMenuBarActionMapView::AddDocumentObjectToMenu(QMenu* pCurrentRoot, ezDocumentObjectBase* pObject)
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

void ezMenuBarActionMapView::CreateView()
{
  ClearView();

  auto pObject = m_pActionMap->GetRootObject();

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
        EZ_REPORT_FAILURE("Cannot map actions in a menubar view!");
      }
      break;

    case ezActionType::Category:
      {
        EZ_REPORT_FAILURE("Cannot map category in a menubar view!");
      }
      break;

    case ezActionType::Menu:
      {
        QMenu* pQtMenu = static_cast<ezQtMenuProxy*>(pProxy)->GetQMenu();
        addMenu(pQtMenu);

        AddDocumentObjectToMenu(pQtMenu, pChild);
      }
      break;
    }
  }
}
