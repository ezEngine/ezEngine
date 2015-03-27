#include <GuiFoundation/PCH.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
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
  for (auto it = m_Proxies.GetIterator(); it.IsValid(); ++it)
  {
    ezQtProxy* pProxy = it.Value();
    pProxy->deleteLater();
  }
  m_Proxies.Clear();
}

void ezToolBarActionMapView::CreateView()
{
  ClearView();

  auto pObject = m_pActionMap->GetRootObject();

  CreateView(pObject);

  if (!actions().isEmpty() && actions().back()->isSeparator())
  {
    QAction* pAction = actions().back();
    removeAction(pAction);
    pAction->deleteLater();
  }
}

void ezToolBarActionMapView::CreateView(ezDocumentObjectBase* pObject)
{
  for (auto pChild : pObject->GetChildren())
  {
    auto pDesc = m_pActionMap->GetDescriptor(pChild);
    auto pAction = pDesc->m_hAction.GetDescriptor()->CreateAction(m_Context);

    ezQtProxy* pProxy = ezRttiMappedObjectFactory<ezQtProxy>::CreateObject(pAction->GetDynamicRTTI());

    if (pProxy != nullptr)
    {
      m_Proxies[pChild->GetGuid()] = pProxy;
      pProxy->setParent(this);
      pProxy->SetAction(pAction, false);
    }

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
        if (!actions().isEmpty() && !actions().back()->isSeparator())
          addSeparator();

        CreateView(pChild);

        if (!actions().isEmpty() && !actions().back()->isSeparator())
          addSeparator();
      }
      break;

    case ezActionType::Menu:
      {
        QMenu* pQtMenu = static_cast<ezQtMenuProxy*>(pProxy)->GetQMenu();
        // TODO pButton leaks!
        QToolButton* pButton = new QToolButton(this);
        pButton->setMenu(pQtMenu);
        pButton->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
        pButton->setText(pQtMenu->title());

        // TODO addWidget return value of QAction leaks!
        QAction* pToolButtonAction = addWidget(pButton);
        pToolButtonAction->setParent(pQtMenu);
        //pButton->setParent(pQtMenu);

        ezMenuActionMapView::AddDocumentObjectToMenu(m_Proxies, m_Context, m_pActionMap, pQtMenu, pChild);
      }
      break;
    }
  }
}
