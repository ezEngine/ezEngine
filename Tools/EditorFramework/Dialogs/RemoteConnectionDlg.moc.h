#pragma once

#include <EditorFramework/Plugin.h>
#include <QDialog>
#include <Tools/EditorFramework/ui_RemoteConnectionDlg.h>
#include <Foundation/Strings/String.h>

class EZ_EDITORFRAMEWORK_DLL ezQtRemoteConnectionDlg : public QDialog, public Ui_ezQtRemoteConnectionDlg
{
public:
  Q_OBJECT

  struct Address
  {
    ezUInt8 part[4];

    Address();
    void operator=(const Address& rhs);
    bool operator==(const Address& rhs) const;
    bool IsEmpty() const;
  };

public:
  ezQtRemoteConnectionDlg(QWidget* parent);
  ~ezQtRemoteConnectionDlg();

  Address m_UsedAddress;
  Address m_UsedFsAddress;

  QString GetResultingAddress() const;
  QString GetResultingFsAddress() const;

private slots:
  void on_ButtonConnect_clicked();
  void on_ButtonLaunchFS_clicked();
  void onRecentIPselected();
  void onRecentFsIPselected();

private:
  Address m_RecentAddresses[5];
  Address m_RecentFsAddresses[5];

  virtual void showEvent(QShowEvent* event) override;
  void AddToRecentAddresses(Address* pRecentAddresses, const Address& addr);
  void SetCurrentIP(const Address& addr);
  void SetCurrentFsIP(const Address& addr);
};


