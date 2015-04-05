#include <GuiFoundation/PCH.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>
#include <Foundation/Configuration/Startup.h>
#include <QMenu>
#include <QAction>

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

EZ_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, QtProxies)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ToolsFoundation",
    "ActionManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
    ezRttiMappedObjectFactory<ezQtProxy>::RegisterCreator(ezGetStaticRTTI<ezMenuAction>(), QtMenuProxyCreator);
    ezRttiMappedObjectFactory<ezQtProxy>::RegisterCreator(ezGetStaticRTTI<ezCategoryAction>(), QtCategoryProxyCreator);
    ezRttiMappedObjectFactory<ezQtProxy>::RegisterCreator(ezGetStaticRTTI<ezLRUMenuAction>(), QtLRUMenuProxyCreator);
    ezRttiMappedObjectFactory<ezQtProxy>::RegisterCreator(ezGetStaticRTTI<ezButtonAction>(), QtButtonProxyCreator);
  }

  ON_CORE_SHUTDOWN
  {
  }

EZ_END_SUBSYSTEM_DECLARATION

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
  EZ_ASSERT_DEV(m_pQtAction != nullptr, "Something went horribly wrong!");

  m_pAction->m_StatusUpdateEvent.RemoveEventHandler(ezMakeDelegate(&ezQtButtonProxy::StatusUpdateEventHandler, this));

  m_pQtAction->deleteLater();
  m_pQtAction = nullptr;
}

void ezQtButtonProxy::Update(bool bSetShortcut)
{
  auto pButton = static_cast<ezButtonAction*>(m_pAction);

  if (bSetShortcut)
  {
    m_pQtAction->setShortcut(QKeySequence(QString::fromUtf8(m_pAction->GetDescriptorHandle().GetDescriptor()->m_sShortcut.GetData())));
    m_pQtAction->setShortcutContext(Qt::ShortcutContext::ApplicationShortcut);
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

  m_pQtAction = new QAction(nullptr);
  
  EZ_VERIFY(connect(m_pQtAction, SIGNAL(triggered(bool)), this, SLOT(OnTriggered())) != nullptr, "connection failed");
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

      auto pAction = m_pMenu->addAction(QString::fromUtf8(p.first.GetData()));
      pAction->setData(i);

      EZ_VERIFY(connect(pAction, SIGNAL(triggered()), this, SLOT(SlotMenuEntryTriggered())) != nullptr, "signal/slot connection failed");
    }
  }
}

void ezQtLRUMenuProxy::SlotMenuEntryTriggered()
{
  QAction* pAction = qobject_cast<QAction*>(sender());
  if (!pAction)
    return;

  ezUInt32 index = pAction->data().toUInt();
  m_pAction->Execute(m_Entries[index].second);

}

