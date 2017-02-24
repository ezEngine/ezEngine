#pragma once

#include <EditorPluginFileserve/Plugin.h>
#include <Foundation/Containers/Deque.h>
#include <QAbstractListModel>

enum class ezFileserveActivityType
{
  StartServer,
  StopServer,
  ClientConnect,
  ClientDisconnect,
  Mount,
  MountFailed,
  Unmount,
  ReadFile,
  WriteFile,
  DeleteFile,
  Other
};

struct ezQtFileserveActivityItem
{
  QString m_Text;
  ezFileserveActivityType m_Type;
};

class EZ_EDITORPLUGINFILESERVE_DLL ezQtFileserveActivityModel : public QAbstractListModel
{
  Q_OBJECT

public:
  ezQtFileserveActivityModel(QWidget* parent);

  virtual int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

  ezQtFileserveActivityItem& AppendItem();
  void UpdateView();

  void Clear();
  private slots:
  void UpdateViewSlot();

private:
  bool m_bTimerRunning = false;
  ezUInt32 m_uiAddedItems = 0;
  ezDeque<ezQtFileserveActivityItem> m_Items;
};

