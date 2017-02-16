#include <PCH.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <QMenu>
#include <QToolButton>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <Foundation/Strings/TranslationLookup.h>

ezQtToolBarActionMapView::ezQtToolBarActionMapView(QString title, QWidget* parent) : QToolBar(title, parent)
{
  setIconSize(QSize(16, 16));
  setFloatable(false);
}

ezQtToolBarActionMapView::~ezQtToolBarActionMapView()
{
  ClearView();
}

void ezQtToolBarActionMapView::SetActionContext(const ezActionContext& context)
{
  auto pMap = ezActionMapManager::GetActionMap(context.m_sMapping);

  EZ_ASSERT_DEV(pMap != nullptr, "The given mapping '{0}' does not exist", context.m_sMapping.GetData());

  m_pActionMap = pMap;
  m_Context = context;

  CreateView();
}

void ezQtToolBarActionMapView::ClearView()
{
  m_Proxies.Clear();
}

void ezQtToolBarActionMapView::CreateView()
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

void ezQtToolBarActionMapView::CreateView(const ezActionMap::TreeNode* pObject)
{
  for (auto pChild : pObject->GetChildren())
  {
    auto pDesc = m_pActionMap->GetDescriptor(pChild);
    QSharedPointer<ezQtProxy> pProxy = ezQtProxy::GetProxy(m_Context, pDesc->m_hAction);
    m_Proxies[pChild->GetGuid()] = pProxy;

    switch (pDesc->m_hAction.GetDescriptor()->m_Type)
    {
    case ezActionType::Action:
      {
        QAction* pQtAction = static_cast<ezQtActionProxy*>(pProxy.data())->GetQAction();
        addAction(pQtAction);
      }
      break;

    case ezActionType::Category:
      {
        if (!actions().isEmpty() && !actions().back()->isSeparator())
          addSeparator()->setParent(pProxy.data());

        CreateView(pChild);

        if (!actions().isEmpty() && !actions().back()->isSeparator())
          addSeparator()->setParent(pProxy.data());
      }
      break;

    case ezActionType::Menu:
      {
        ezNamedAction* pNamed = static_cast<ezNamedAction*>(pProxy->GetAction());

        QMenu* pQtMenu = static_cast<ezQtMenuProxy*>(pProxy.data())->GetQMenu();
        // TODO pButton leaks!
        QToolButton* pButton = new QToolButton(this);
        pButton->setMenu(pQtMenu);
        pButton->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
        pButton->setText(pQtMenu->title());
        pButton->setIcon(ezQtUiServices::GetCachedIconResource(pNamed->GetIconPath()));
        pButton->setToolTip(pQtMenu->title().toUtf8().data());

        // TODO addWidget return value of QAction leaks!
        QAction* pToolButtonAction = addWidget(pButton);
        pToolButtonAction->setParent(pQtMenu);

        ezQtMenuActionMapView::AddDocumentObjectToMenu(m_Proxies, m_Context, m_pActionMap, pQtMenu, pChild);
      }
      break;

    case ezActionType::ActionAndMenu:
      {
        ezNamedAction* pNamed = static_cast<ezNamedAction*>(pProxy->GetAction());

        QMenu* pQtMenu = static_cast<ezQtDynamicActionAndMenuProxy*>(pProxy.data())->GetQMenu();
        QAction* pQtAction = static_cast<ezQtDynamicActionAndMenuProxy*>(pProxy.data())->GetQAction();
        // TODO pButton leaks!
        QToolButton* pButton = new QToolButton(this);
        pButton->setDefaultAction(pQtAction);
        pButton->setMenu(pQtMenu);
        pButton->setPopupMode(QToolButton::ToolButtonPopupMode::MenuButtonPopup);

        // TODO addWidget return value of QAction leaks!
        QAction* pToolButtonAction = addWidget(pButton);
        pToolButtonAction->setParent(pQtMenu);

        ezQtMenuActionMapView::AddDocumentObjectToMenu(m_Proxies, m_Context, m_pActionMap, pQtMenu, pChild);
      }
      break;
    }
  }
}
