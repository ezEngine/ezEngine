#include <GuiFoundation/PCH.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>
#include <QMenu>
#include <QToolButton>

ezToolBarActionMapView::ezToolBarActionMapView(QWidget* parent) : QToolBar(parent)
{
}

ezToolBarActionMapView::~ezToolBarActionMapView()
{
  ClearView();
}

ezResult ezToolBarActionMapView::SetActionContext(const ezActionContext& context)
{
  auto pMap = ezActionMapManager::GetActionMap(context.m_sMapping);

  if (pMap == nullptr)
    return EZ_FAILURE;

  m_pActionMap = pMap;
  m_Context = context;

  CreateView();

  return EZ_SUCCESS;
}

void ezToolBarActionMapView::ClearView()
{
}

void ezToolBarActionMapView::AddDocumentObjectToMenu(QMenu* pCurrentRoot, ezDocumentObjectBase* pObject)
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

void ezToolBarActionMapView::CreateView()
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
        QAction* pQtAction = static_cast<ezQtActionProxy*>(pProxy)->GetQAction();
        addAction(pQtAction);
        pQtAction->setParent(this);
      }
      break;

    case ezActionType::Category:
      {
        EZ_REPORT_FAILURE("Cannot map category in a toolbar view!");
      }
      break;

    case ezActionType::Menu:
      {
        QMenu* pQtMenu = static_cast<ezQtMenuProxy*>(pProxy)->GetQMenu();
        // TODO pButton leaks!
        QToolButton* pButton = new QToolButton(this);
        pButton->setMenu(pQtMenu);
        pButton->setPopupMode(QToolButton::ToolButtonPopupMode::MenuButtonPopup);

        // TODO addWidget return value of QAction leaks!
        addWidget(pButton);
        pQtMenu->setParent(pButton);

        //AddDocumentObjectToMenu(pQtMenu, pChild);
      }
      break;
    }
  }
}
