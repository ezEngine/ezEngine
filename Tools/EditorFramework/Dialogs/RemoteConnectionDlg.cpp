#include <PCH.h>
#include <EditorFramework/Dialogs/RemoteConnectionDlg.moc.h>
#include <EditorFramework/Dialogs/LaunchFileserveDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QSettings>

ezQtRemoteConnectionDlg::Address::Address()
{
  part[0] = 0;
  part[1] = 0;
  part[2] = 0;
  part[3] = 0;
}

void ezQtRemoteConnectionDlg::Address::operator=(const Address& rhs)
{
  part[0] = rhs.part[0];
  part[1] = rhs.part[1];
  part[2] = rhs.part[2];
  part[3] = rhs.part[3];
}

bool ezQtRemoteConnectionDlg::Address::operator==(const Address& rhs) const
{
  return part[0] == rhs.part[0] &&
    part[1] == rhs.part[1] &&
    part[2] == rhs.part[2] &&
    part[3] == rhs.part[3];
}


bool ezQtRemoteConnectionDlg::Address::IsEmpty() const
{
  return part[0] == 0 &&
    part[1] == 0 &&
    part[2] == 0 &&
    part[3] == 0;
}

ezQtRemoteConnectionDlg::ezQtRemoteConnectionDlg(QWidget* parent) : QDialog(parent)
{
  setupUi(this);

  QSettings Settings;
  Settings.beginGroup(QLatin1String("RemoteConnection"));
  {
    for (ezInt32 i = 0; i < EZ_ARRAY_SIZE(m_RecentAddresses); ++i)
    {
      m_RecentAddresses[i].part[0] = Settings.value(QString("IP%1a").arg(i), 0).toInt();
      m_RecentAddresses[i].part[1] = Settings.value(QString("IP%1b").arg(i), 0).toInt();
      m_RecentAddresses[i].part[2] = Settings.value(QString("IP%1c").arg(i), 0).toInt();
      m_RecentAddresses[i].part[3] = Settings.value(QString("IP%1d").arg(i), 0).toInt();
    }

    for (ezInt32 i = 0; i < EZ_ARRAY_SIZE(m_RecentFsAddresses); ++i)
    {
      m_RecentFsAddresses[i].part[0] = Settings.value(QString("FsIP%1a").arg(i), 0).toInt();
      m_RecentFsAddresses[i].part[1] = Settings.value(QString("FsIP%1b").arg(i), 0).toInt();
      m_RecentFsAddresses[i].part[2] = Settings.value(QString("FsIP%1c").arg(i), 0).toInt();
      m_RecentFsAddresses[i].part[3] = Settings.value(QString("FsIP%1d").arg(i), 0).toInt();
    }
  }
  Settings.endGroup();

  m_UsedAddress = m_RecentAddresses[0];
  m_UsedFsAddress = m_RecentFsAddresses[0];
}

ezQtRemoteConnectionDlg::~ezQtRemoteConnectionDlg()
{
}

void ezQtRemoteConnectionDlg::SetCurrentIP(const Address& addr)
{
  IP1->setText(QString("%1").arg(addr.part[0]));
  IP2->setText(QString("%1").arg(addr.part[1]));
  IP3->setText(QString("%1").arg(addr.part[2]));
  IP4->setText(QString("%1").arg(addr.part[3]));
}

void ezQtRemoteConnectionDlg::SetCurrentFsIP(const Address& addr)
{
  FsIP1->setText(QString("%1").arg(addr.part[0]));
  FsIP2->setText(QString("%1").arg(addr.part[1]));
  FsIP3->setText(QString("%1").arg(addr.part[2]));
  FsIP4->setText(QString("%1").arg(addr.part[3]));
}

void ezQtRemoteConnectionDlg::AddToRecentAddresses(Address* pRecentAddresses, const Address& addr)
{
  Address prev = addr;
  for (ezInt32 i = 0; i < EZ_ARRAY_SIZE(m_RecentAddresses); ++i)
  {
    Address cur = pRecentAddresses[i];
    pRecentAddresses[i] = prev;

    if (cur == addr)
      break;

    prev = cur;
  }
}
void ezQtRemoteConnectionDlg::showEvent(QShowEvent* event)
{
  AddToRecentAddresses(m_RecentAddresses, m_UsedAddress);
  AddToRecentAddresses(m_RecentFsAddresses, m_UsedFsAddress);

  SetCurrentIP(m_UsedAddress);
  SetCurrentFsIP(m_UsedFsAddress);

  for (ezInt32 i = 0; i < EZ_ARRAY_SIZE(m_RecentAddresses); ++i)
  {
    if (!m_RecentAddresses[i].IsEmpty())
    {
      QAction* pAction = new QAction(this);
      pAction->setText(QString("%1.%2.%3.%4").arg(m_RecentAddresses[i].part[0]).arg(m_RecentAddresses[i].part[1]).arg(m_RecentAddresses[i].part[2]).arg(m_RecentAddresses[i].part[3]));
      pAction->setData(i);

      connect(pAction, &QAction::triggered, this, &ezQtRemoteConnectionDlg::onRecentIPselected);

      RecentIPs->addAction(pAction);
    }
  }

  for (ezInt32 i = 0; i < EZ_ARRAY_SIZE(m_RecentFsAddresses); ++i)
  {
    if (!m_RecentFsAddresses[i].IsEmpty())
    {
      QAction* pAction = new QAction(this);
      pAction->setText(QString("%1.%2.%3.%4").arg(m_RecentFsAddresses[i].part[0]).arg(m_RecentFsAddresses[i].part[1]).arg(m_RecentFsAddresses[i].part[2]).arg(m_RecentFsAddresses[i].part[3]));
      pAction->setData(i);

      connect(pAction, &QAction::triggered, this, &ezQtRemoteConnectionDlg::onRecentFsIPselected);

      RecentFsIPs->addAction(pAction);
    }
  }

  QDialog::showEvent(event);
}

void ezQtRemoteConnectionDlg::on_ButtonConnect_clicked()
{
  m_UsedAddress.part[0] = IP1->text().toInt();
  m_UsedAddress.part[1] = IP2->text().toInt();
  m_UsedAddress.part[2] = IP3->text().toInt();
  m_UsedAddress.part[3] = IP4->text().toInt();

  m_UsedFsAddress.part[0] = FsIP1->text().toInt();
  m_UsedFsAddress.part[1] = FsIP2->text().toInt();
  m_UsedFsAddress.part[2] = FsIP3->text().toInt();
  m_UsedFsAddress.part[3] = FsIP4->text().toInt();

  // store latest address in recent list, shift existing items back
  AddToRecentAddresses(m_RecentAddresses, m_UsedAddress);
  AddToRecentAddresses(m_RecentFsAddresses, m_UsedFsAddress);

  // store recent list
  {
    QSettings Settings;
    Settings.beginGroup(QLatin1String("RemoteConnection"));
    {
      for (ezInt32 i = 0; i < EZ_ARRAY_SIZE(m_RecentAddresses); ++i)
      {
        Settings.setValue(QString("IP%1b").arg(i), m_RecentAddresses[i].part[1]);
        Settings.setValue(QString("IP%1c").arg(i), m_RecentAddresses[i].part[2]);
        Settings.setValue(QString("IP%1a").arg(i), m_RecentAddresses[i].part[0]);
        Settings.setValue(QString("IP%1d").arg(i), m_RecentAddresses[i].part[3]);
      }

      for (ezInt32 i = 0; i < EZ_ARRAY_SIZE(m_RecentFsAddresses); ++i)
      {
        Settings.setValue(QString("FsIP%1b").arg(i), m_RecentFsAddresses[i].part[1]);
        Settings.setValue(QString("FsIP%1c").arg(i), m_RecentFsAddresses[i].part[2]);
        Settings.setValue(QString("FsIP%1a").arg(i), m_RecentFsAddresses[i].part[0]);
        Settings.setValue(QString("FsIP%1d").arg(i), m_RecentFsAddresses[i].part[3]);
      }
    }
    Settings.endGroup();
  }

  accept();
}


void ezQtRemoteConnectionDlg::on_ButtonLaunchFS_clicked()
{
  ezQtLaunchFileserveDlg dlg(this);
  dlg.exec();
}

QString ezQtRemoteConnectionDlg::GetResultingAddress() const
{
  return QString("%1.%2.%3.%4:1050").arg(m_UsedAddress.part[0]).arg(m_UsedAddress.part[1]).arg(m_UsedAddress.part[2]).arg(m_UsedAddress.part[3]);
}

QString ezQtRemoteConnectionDlg::GetResultingFsAddress() const
{
  return QString("%1.%2.%3.%4:1042").arg(m_UsedFsAddress.part[0]).arg(m_UsedFsAddress.part[1]).arg(m_UsedFsAddress.part[2]).arg(m_UsedFsAddress.part[3]);
}

void ezQtRemoteConnectionDlg::onRecentIPselected()
{
  QAction* pAction = qobject_cast<QAction*>(sender());
  int ip = pAction->data().toInt();

  SetCurrentIP(m_RecentAddresses[ip]);
}

void ezQtRemoteConnectionDlg::onRecentFsIPselected()
{
  QAction* pAction = qobject_cast<QAction*>(sender());
  int ip = pAction->data().toInt();

  SetCurrentFsIP(m_RecentFsAddresses[ip]);
}
