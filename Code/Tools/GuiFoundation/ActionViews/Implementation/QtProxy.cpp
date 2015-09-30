#include <GuiFoundation/PCH.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>
#include <Foundation/Configuration/Startup.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <QMenu>
#include <QAction>

ezRttiMappedObjectFactory<ezQtProxy> ezQtProxy::s_Factory;

static ezQtProxy* QtMenuProxyCreator(const ezRTTI* pRtti)
{
  return new(ezQtMenuProxy);
}

static ezQtProxy* QtCategoryProxyCreator(const ezRTTI* pRtti)
{
  return new(ezQtCategoryProxy);
}

static ezQtProxy* QtButtonProxyCreator(const ezRTTI* pRtti)
{
  return new(ezQtButtonProxy);
}

static ezQtProxy* QtLRUMenuProxyCreator(const ezRTTI* pRtti)
{
  return new(ezQtLRUMenuProxy);
}

ezMap<ezActionDescriptorHandle, QWeakPointer<ezQtProxy>> ezQtProxy::s_GlobalActions;
ezMap<ezUuid, ezMap<ezActionDescriptorHandle, QWeakPointer<ezQtProxy>> > ezQtProxy::s_DocumentActions;
ezMap<QWidget*, ezMap<ezActionDescriptorHandle, QWeakPointer<ezQtProxy>> > ezQtProxy::s_WindowActions;

EZ_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, QtProxies)

BEGIN_SUBSYSTEM_DEPENDENCIES
"ToolsFoundation",
"ActionManager"
END_SUBSYSTEM_DEPENDENCIES

ON_CORE_STARTUP
{
  ezQtProxy::GetFactory().RegisterCreator(ezGetStaticRTTI<ezMenuAction>(), QtMenuProxyCreator);
  ezQtProxy::GetFactory().RegisterCreator(ezGetStaticRTTI<ezCategoryAction>(), QtCategoryProxyCreator);
  ezQtProxy::GetFactory().RegisterCreator(ezGetStaticRTTI<ezLRUMenuAction>(), QtLRUMenuProxyCreator);
  ezQtProxy::GetFactory().RegisterCreator(ezGetStaticRTTI<ezButtonAction>(), QtButtonProxyCreator);
}

ON_CORE_SHUTDOWN
{
  ezQtProxy::s_GlobalActions.Clear();
  ezQtProxy::s_DocumentActions.Clear();
  ezQtProxy::s_WindowActions.Clear();
}

EZ_END_SUBSYSTEM_DECLARATION

ezRttiMappedObjectFactory<ezQtProxy>& ezQtProxy::GetFactory()
{
  return s_Factory;
}
QSharedPointer<ezQtProxy> ezQtProxy::GetProxy(ezActionContext& context, ezActionDescriptorHandle hDesc)
{
  QSharedPointer<ezQtProxy> pProxy;
  const ezActionDescriptor* pDesc = hDesc.GetDescriptor();
  if (pDesc->m_Type != ezActionType::Action)
  {
    auto pAction = pDesc->CreateAction(context);
    pProxy = QSharedPointer<ezQtProxy>(ezQtProxy::GetFactory().CreateObject(pAction->GetDynamicRTTI()));
    EZ_ASSERT_DEBUG(pProxy != nullptr, "No proxy assigned to action '%s'", pDesc->m_sActionName.GetData());
    pProxy->SetAction(pAction, true);
    return pProxy;
  }

  // ezActionType::Action will be cached to ensure only one QAction exist in its scope to prevent shortcut collisions.
  switch (pDesc->m_Scope)
  {
  case ezActionScope::Global:
    {
      QWeakPointer<ezQtProxy> pTemp = s_GlobalActions[hDesc];
      if (pTemp.isNull())
      {
        auto pAction = pDesc->CreateAction(context);
        pProxy = QSharedPointer<ezQtProxy>(ezQtProxy::GetFactory().CreateObject(pAction->GetDynamicRTTI()));
        EZ_ASSERT_DEBUG(pProxy != nullptr, "No proxy assigned to action '%s'", pDesc->m_sActionName.GetData());
        pProxy->SetAction(pAction, true);
        s_GlobalActions[hDesc] = pProxy.toWeakRef();
      }
      else
      {
        pProxy = pTemp.toStrongRef();
      }
    }
    break;
  case ezActionScope::Document:
    {
      QWeakPointer<ezQtProxy> pTemp = s_DocumentActions[context.m_pDocument->GetGuid()][hDesc];
      if (pTemp.isNull())
      {
        auto pAction = pDesc->CreateAction(context);
        pProxy = QSharedPointer<ezQtProxy>(ezQtProxy::GetFactory().CreateObject(pAction->GetDynamicRTTI()));
        EZ_ASSERT_DEBUG(pProxy != nullptr, "No proxy assigned to action '%s'", pDesc->m_sActionName.GetData());
        pProxy->SetAction(pAction, true);
        s_DocumentActions[context.m_pDocument->GetGuid()][hDesc] = pProxy;
      }
      else
      {
        pProxy = pTemp.toStrongRef();
      }
    }
    break;
  case ezActionScope::Window:
    {
      QWeakPointer<ezQtProxy> pTemp = s_WindowActions[context.m_pWindow][hDesc];
      if (pTemp.isNull())
      {
        auto pAction = pDesc->CreateAction(context);
        pProxy = QSharedPointer<ezQtProxy>(ezQtProxy::GetFactory().CreateObject(pAction->GetDynamicRTTI()));
        EZ_ASSERT_DEBUG(pProxy != nullptr, "No proxy assigned to action '%s'", pDesc->m_sActionName.GetData());
        pProxy->SetAction(pAction, true);
        s_WindowActions[pAction->GetContext().m_pWindow][hDesc] = pProxy;
      }
      else
      {
        pProxy = pTemp.toStrongRef();
      }
    }
    break;
  }
  return pProxy;
}

ezQtProxy::ezQtProxy()
{
  m_pAction = nullptr;
}

ezQtProxy::~ezQtProxy()
{
  if (m_pAction != nullptr)
    ezActionManager::GetActionDescriptor(m_pAction->GetDescriptorHandle())->DeleteAction(m_pAction);
}

void ezQtProxy::SetAction(ezAction* pAction, bool bSetShortcut)
{
  m_pAction = pAction;
}

//////////////////// ezQtMenuProxy /////////////////////

ezQtMenuProxy::ezQtMenuProxy()
{
  m_pMenu = nullptr;
}

ezQtMenuProxy::~ezQtMenuProxy()
{
  m_pMenu->deleteLater();
  delete m_pMenu;
}

void ezQtMenuProxy::Update(bool bSetShortcut)
{
  auto pMenu = static_cast<ezMenuAction*>(m_pAction);

  m_pMenu->setIcon(QIcon(QString::fromUtf8(pMenu->GetIconPath())));
  m_pMenu->setTitle(QString::fromUtf8(pMenu->GetText()));
}

void ezQtMenuProxy::SetAction(ezAction* pAction, bool bSetShortcut)
{
  ezQtProxy::SetAction(pAction, bSetShortcut);

  m_pMenu = new QMenu();
  Update(bSetShortcut);
}

QMenu* ezQtMenuProxy::GetQMenu()
{
  return m_pMenu;
}

//////////////////// ezQtButtonProxy /////////////////////

ezQtButtonProxy::ezQtButtonProxy()
{
  m_pQtAction = nullptr;
}

ezQtButtonProxy::~ezQtButtonProxy()
{
  m_pAction->m_StatusUpdateEvent.RemoveEventHandler(ezMakeDelegate(&ezQtButtonProxy::StatusUpdateEventHandler, this));

  if (m_pQtAction != nullptr)
  {
    m_pQtAction->deleteLater();
  }
  m_pQtAction = nullptr;
}

void ezQtButtonProxy::Update(bool bSetShortcut)
{
  if (m_pQtAction == nullptr)
    return;

  auto pButton = static_cast<ezButtonAction*>(m_pAction);

  if (bSetShortcut)
  {
    const ezActionDescriptor* pDesc = m_pAction->GetDescriptorHandle().GetDescriptor();
    m_pQtAction->setShortcut(QKeySequence(QString::fromUtf8(pDesc->m_sShortcut.GetData())));
  }

  m_pQtAction->setIcon(QIcon(QString::fromUtf8(pButton->GetIconPath())));
  m_pQtAction->setText(QString::fromUtf8(pButton->GetText()));
  m_pQtAction->setCheckable(pButton->IsCheckable());
  m_pQtAction->setChecked(pButton->IsChecked());
  m_pQtAction->setEnabled(pButton->IsEnabled());
  m_pQtAction->setVisible(pButton->IsVisible());
}

void ezQtButtonProxy::SetAction(ezAction* pAction, bool bSetShortcut)
{
  EZ_ASSERT_DEV(m_pAction == nullptr, "Es darf nicht sein, es kann nicht sein!");

  ezQtProxy::SetAction(pAction, bSetShortcut);
  m_pAction->m_StatusUpdateEvent.AddEventHandler(ezMakeDelegate(&ezQtButtonProxy::StatusUpdateEventHandler, this));

  ezActionDescriptorHandle hDesc = m_pAction->GetDescriptorHandle();
  const ezActionDescriptor* pDesc = hDesc.GetDescriptor();

  if (m_pQtAction == nullptr)
  {
    m_pQtAction = new QAction(nullptr);
    EZ_VERIFY(connect(m_pQtAction, SIGNAL(triggered(bool)), this, SLOT(OnTriggered())) != nullptr, "connection failed");

    switch (pDesc->m_Scope)
    {
    case ezActionScope::Global:
      {
       // Parent is null so the global actions don't get deleted.
        m_pQtAction->setShortcutContext(Qt::ShortcutContext::ApplicationShortcut);
      }
      break;
    case ezActionScope::Document:
      {
        // Parent is set to the window belonging to the document.
        ezDocumentWindow* pWindow = ezDocumentWindow::FindWindowByDocument(pAction->GetContext().m_pDocument);
        EZ_ASSERT_DEBUG(pWindow != nullptr, "You can't map a ezActionScope::Document action without that document existing!");
        m_pQtAction->setParent(pWindow);
        m_pQtAction->setShortcutContext(Qt::ShortcutContext::WindowShortcut);
      }
      break;
    case ezActionScope::Window:
      {
        m_pQtAction->setParent(pAction->GetContext().m_pWindow);
        m_pQtAction->setShortcutContext(Qt::ShortcutContext::WidgetWithChildrenShortcut);
      }
      break;
    }
  }

  Update(bSetShortcut);
}

QAction* ezQtButtonProxy::GetQAction()
{
  return m_pQtAction;
}

void ezQtButtonProxy::StatusUpdateEventHandler(ezAction* pAction)
{
  Update(false);
}

void ezQtButtonProxy::OnTriggered()
{
  // make sure all focus is lost, to trigger pending changes
  if (QApplication::focusWidget())
    QApplication::focusWidget()->clearFocus();

  m_pAction->Execute(m_pQtAction->isChecked());
}

void ezQtLRUMenuProxy::SetAction(ezAction* pAction, bool bSetShortcut)
{
  ezQtMenuProxy::SetAction(pAction, bSetShortcut);

  EZ_VERIFY(connect(m_pMenu, SIGNAL(aboutToShow()), this, SLOT(SlotMenuAboutToShow())) != nullptr, "signal/slot connection failed");
}

void ezQtLRUMenuProxy::SlotMenuAboutToShow()
{
  m_pMenu->clear();

  static_cast<ezLRUMenuAction*>(m_pAction)->GetEntries(m_Entries);

  if (m_Entries.IsEmpty())
  {
    m_pMenu->addAction("<empty>")->setEnabled(false);
  }
  else
  {
    for (ezUInt32 i = 0; i < m_Entries.GetCount(); ++i)
    {
      const auto& p = m_Entries[i];

      auto pAction = m_pMenu->addAction(QString::fromUtf8(p.m_sDisplay.GetData()));
      pAction->setData(i);
      pAction->setIcon(p.m_Icon);
      pAction->setCheckable(p.m_CheckState != ezLRUMenuAction::Item::CheckMark::NotCheckable);
      pAction->setChecked(p.m_CheckState == ezLRUMenuAction::Item::CheckMark::Checked);

      EZ_VERIFY(connect(pAction, SIGNAL(triggered()), this, SLOT(SlotMenuEntryTriggered())) != nullptr, "signal/slot connection failed");
    }
  }
}

void ezQtLRUMenuProxy::SlotMenuEntryTriggered()
{
  QAction* pAction = qobject_cast<QAction*>(sender());
  if (!pAction)
    return;

  // make sure all focus is lost, to trigger pending changes
  if (QApplication::focusWidget())
    QApplication::focusWidget()->clearFocus();

  ezUInt32 index = pAction->data().toUInt();
  m_pAction->Execute(m_Entries[index].m_UserValue);

}

