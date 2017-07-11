#pragma once

#include <EditorFramework/Plugin.h>
#include <QDialog>
#include <Tools/EditorFramework/ui_RemoteConnectionDlg.h>

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

  bool m_bLaunchFileserve = true;
  ezString m_sFileserveCmdLine;
  Address m_UsedAddress;

  QString GetResultingAddress() const;

private slots:
  void on_ButtonConnect_clicked();
  void on_RecentIP_selected();

private:
  Address m_RecentAddresses[5];

  virtual void showEvent(QShowEvent* event) override;
  void AddToRecentAddresses(const Address& addr);
  void SetCurrentIP(const Address& addr);
};


